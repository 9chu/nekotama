#include "ClientSession.h"

#include "Server.h"

#define MAXPACKAGESIZE 4096

using namespace std;
using namespace Bencode;
using namespace nekotama;

////////////////////////////////////////////////////////////////////////////////

static const chrono::milliseconds sc_RecvTimeout(10000);    // 10秒收发延迟
static const chrono::milliseconds sc_LoginTimeout(5000);    // 5秒登陆延迟
static const chrono::milliseconds sc_PingPongTimeout(5000); // 5秒pingpong延迟

////////////////////////////////////////////////////////////////////////////////

template<typename T>
static T GetPackageField(const Bencode::Value& v, const std::string& field);

template<>
static int GetPackageField(const Bencode::Value& v, const std::string& field)
{
	if (v.Type != ValueType::Dictionary)
		throw logic_error("invalid package format.");
	auto tField = v.VDict.find(field);
	if (tField == v.VDict.end())
		throw logic_error(StringFormat("invalid package format, field '%s' required.", field.c_str()));
	if (tField->second->Type != ValueType::Int)
		throw logic_error(StringFormat("invalid package format, field '%s' should be an integer.", field.c_str()));
	return tField->second->VInt;
}

template<>
static const std::string& GetPackageField(const Bencode::Value& v, const std::string& field)
{
	if (v.Type != ValueType::Dictionary)
		throw logic_error("invalid package format.");
	auto tField = v.VDict.find(field);
	if (tField == v.VDict.end())
		throw logic_error(StringFormat("invalid package format, field '%s' required.", field.c_str()));
	if (tField->second->Type != ValueType::String)
		throw logic_error(StringFormat("invalid package format, field '%s' should be a string.", field.c_str()));
	return tField->second->VString;
}

////////////////////////////////////////////////////////////////////////////////

ClientSession::ClientSession(Server* server, SocketHandle handle, const std::string& ip, uint16_t port, uint16_t count)
	: m_pServer(server), m_pLogger(server->GetLogger()), m_cltSocket(handle), m_sIP(ip), m_uPort(port),
	m_iState(ClientSessionState::Invalid), m_bShouldBeClosed(false), m_iBytesRecved(0), m_iLastDataNotSent(0),
	m_bPingSent(false)
{
	// 检查会话上限
	if (count >= server->GetMaxClients())
	{
		m_pLogger->Log(StringFormat("连接人数达到服务器上限，已拒绝连接。(%s:%u)", m_sIP.c_str(), m_uPort), LogType::Error);
		sendKicked(KickReason::ServerIsFull);
		m_iState = ClientSessionState::CloseAfterSend;
		m_pServer->OnClientArrival(this, true);
	}
	else
	{
		m_pLogger->Log(StringFormat("已建立到客户端的连接，等待登陆响应。(%s:%u)", m_sIP.c_str(), m_uPort));
		sendWelcome(m_pServer->GetServerName(), NK_PROTOCOL_MAJOR, NK_PROTOCOL_MINOR);
		m_iState = ClientSessionState::WaitForLogin;
		m_pServer->OnClientArrival(this, false);
	}
}

ClientSession::~ClientSession()
{
	try
	{
		m_pServer->OnClientLeave(this);
	}
	catch (const std::exception& e)
	{
		m_pLogger->Log(StringFormat("响应客户端断开事件时发生异常。(%s:%u: %s)", m_sIP.c_str(), m_uPort, e.what()));
	}
	m_pLogger->Log(StringFormat("已断开到客户端的连接。(%s:%u)", m_sIP.c_str(), m_uPort));
}

void ClientSession::sendWelcome(const std::string& server_name, uint32_t protocol_maj, uint32_t protocol_min)
{
	Value tPackage(ValueType::Dictionary);
	tPackage.VDict["type"] = make_shared<Value>((IntType)PackageType::Welcome);
	tPackage.VDict["name"] = make_shared<Value>(server_name);
	tPackage.VDict["pmaj"] = make_shared<Value>((IntType)protocol_maj);
	tPackage.VDict["pmin"] = make_shared<Value>((IntType)protocol_min);
	push(tPackage);
}

void ClientSession::sendKicked(KickReason reason)
{
	Value tPackage(ValueType::Dictionary);
	tPackage.VDict["type"] = make_shared<Value>((IntType)PackageType::Kicked);
	tPackage.VDict["why"] = make_shared<Value>((IntType)reason);
	push(tPackage);
}

void ClientSession::sendLoginConfirm(const std::string& nick, const std::string& addr)
{
	Value tPackage(ValueType::Dictionary);
	tPackage.VDict["type"] = make_shared<Value>((IntType)PackageType::LoginConfirm);
	tPackage.VDict["nick"] = make_shared<Value>(nick);
	tPackage.VDict["addr"] = make_shared<Value>(addr);
	push(tPackage);
}

void ClientSession::sendPing()
{
	Value tPackage(ValueType::Dictionary);
	tPackage.VDict["type"] = make_shared<Value>((IntType)PackageType::Ping);
	push(tPackage);
}

void ClientSession::push(const Bencode::Value& v)
{
	if (m_iLastDataNotSent == 0)
	{
		m_sLastData = std::move(m_dEncoder << v);
		m_iLastDataNotSent = m_sLastData.length();
	}
	else
		m_dataBuf.emplace_back(m_dEncoder << v);
}

void ClientSession::poll(const Bencode::Value& v)
{
	PackageType tPakType = (PackageType)GetPackageField<int>(v, "type");
	switch (tPakType)
	{
	case PackageType::Login:
		if (m_iState != ClientSessionState::WaitForLogin)
			throw logic_error("invalid 'login' package.");
		else
		{
			std::string tNickname = GetPackageField<const string&>(v, "nick");
			std::string tAddr;
			if (m_pServer->OnClientLogin(this, tNickname, tAddr))
			{
				sendLoginConfirm(tNickname, tAddr);  // 登陆成功
				m_iState = ClientSessionState::Logined;
				m_bPingSent = false;
				m_iPongTimeout = chrono::milliseconds::zero();
				m_iRecvTimeout = chrono::milliseconds::zero();
				m_iDelay = chrono::milliseconds::zero();
			}
			else
			{
				sendKicked(KickReason::LoginFailed);  // 登陆失败
				m_iState = ClientSessionState::CloseAfterSend;
			}	
		}
		break;
	case PackageType::Pong:
		if (m_iState == ClientSessionState::Logined && m_bPingSent)
		{
			m_bPingSent = false;
			m_iDelay = m_iPongTimeout;
			m_iPongTimeout = chrono::milliseconds::zero();
		}
		break;
	}
}

void ClientSession::recv()
{
	m_iRecvTimeout = chrono::milliseconds::zero();

	if (!m_bShouldBeClosed && m_iState != ClientSessionState::Invalid)
	{
		uint8_t tBuf[512];
		size_t tReaded = 0;
		do
		{
			if (m_cltSocket->Recv(tBuf, sizeof(tBuf), tReaded))
			{
				for (size_t i = 0; i < tReaded; ++i)
				{
					++m_iBytesRecved;
					if (m_dDecoder << tBuf[i])
					{
						poll(*m_dDecoder);
						m_iBytesRecved = 0;
					}
					if (m_iBytesRecved >= MAXPACKAGESIZE)
						throw logic_error("package is too big.");
				}
			}
			else
				break;
		} while (tReaded >= sizeof(tBuf));
	}
}

void ClientSession::send()
{
	if (!m_bShouldBeClosed && m_iState != ClientSessionState::Invalid && m_iLastDataNotSent > 0)
	{
		while (m_iLastDataNotSent)
		{
			size_t wrote = 0;
			if (m_cltSocket->Send((uint8_t*)&m_sLastData.data()[m_sLastData.length() - m_iLastDataNotSent], m_iLastDataNotSent, wrote))
				m_iLastDataNotSent -= wrote;
			else
				return;
		}
		
		while (!m_dataBuf.empty())
		{
			m_sLastData = m_dataBuf.front();
			m_iLastDataNotSent = m_sLastData.length();
			m_dataBuf.pop_front();
		}

		if (m_dataBuf.empty() && m_iLastDataNotSent == 0 && m_iState == ClientSessionState::CloseAfterSend)
			m_bShouldBeClosed = true;
	}
}

void ClientSession::invalid()
{
	if (m_iState != ClientSessionState::Invalid)
	{
		m_iState = ClientSessionState::Invalid;
		m_pServer->OnClientInvalid(this);
	}
}

void ClientSession::update(std::chrono::milliseconds tick)
{
	switch (m_iState)
	{
	case ClientSessionState::WaitForLogin:
		m_iLoginTimeout += tick;
		if (m_iLoginTimeout > sc_LoginTimeout)
			throw logic_error(StringFormat("login timeout in %d ms.", sc_LoginTimeout));
		break;
	case ClientSessionState::Logined:
		m_iPongTimeout += tick;
		if (m_iPongTimeout > sc_PingPongTimeout)
			throw logic_error(StringFormat("pong timeout in %d ms.", sc_PingPongTimeout));
		break;
	}

	m_iRecvTimeout += tick;
	if (m_iRecvTimeout > sc_RecvTimeout)
		throw logic_error(StringFormat("no data transfer in %d ms.", sc_RecvTimeout.count()));
}
