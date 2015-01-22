#include "ClientSession.h"

#include "Server.h"

#define MAXPACKAGESIZE 4096

using namespace std;
using namespace Bencode;
using namespace nekotama;

////////////////////////////////////////////////////////////////////////////////

static const chrono::milliseconds sc_RecvTimeout(10000);  // 10秒收发延迟
static const chrono::milliseconds sc_LoginTimeout(5000);  // 5秒登陆延迟
static const chrono::milliseconds sc_PingPeriod(3000);  // 3秒发送1ping
static const chrono::milliseconds sc_PongTimeout(3000);  // 3秒pong延迟

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

void ClientSession::sendLoginConfirm(const std::string& nick, const std::string& addr, uint16_t port)
{
	Value tPackage(ValueType::Dictionary);
	tPackage.VDict["type"] = make_shared<Value>((IntType)PackageType::LoginConfirm);
	tPackage.VDict["nick"] = make_shared<Value>(nick);
	tPackage.VDict["addr"] = make_shared<Value>(addr);
	tPackage.VDict["port"] = make_shared<Value>((IntType)port);
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
	PackageType tPakType = (PackageType)PackageHelper::GetPackageField<int>(v, "type");
	switch (tPakType)
	{
	case PackageType::Login:
		if (m_iState != ClientSessionState::WaitForLogin)
			throw logic_error("invalid 'login' package.");
		else
		{
			std::string tNickname = PackageHelper::GetPackageField<const string&>(v, "nick");
			std::string tAddr;
			uint16_t tPort = 10800;  // 非想天则的默认游戏端口
			if (m_pServer->OnClientLogin(this, tNickname, tAddr, tPort))
			{
				m_Nickname = tNickname;
				m_VirtualAddr = tAddr;
				
				sendLoginConfirm(tNickname, tAddr, tPort);  // 登陆成功
				m_iState = ClientSessionState::Logined;
				m_bPingSent = false;
				m_iPongTimer = chrono::milliseconds::zero();
				m_iPingTimer = chrono::milliseconds::zero();
				m_iRecvTimer = chrono::milliseconds::zero();
				m_iDelay = chrono::milliseconds::zero();

				m_pLogger->Log(StringFormat("客户端登陆成功，昵称: %s，虚拟ip: %s。(%s:%u)", tNickname.c_str(), tAddr.c_str(), m_sIP.c_str(), m_uPort));
			}
			else
			{
				m_pLogger->Log(StringFormat("客户端登陆失败。(%s:%u)", m_sIP.c_str(), m_uPort));

				sendKicked(KickReason::LoginFailed);  // 登陆失败
				m_iState = ClientSessionState::CloseAfterSend;
			}	
		}
		break;
	case PackageType::Pong:
		if (m_iState == ClientSessionState::Logined && m_bPingSent)
		{
			m_bPingSent = false;
			m_iDelay = m_iPongTimer;
			m_iPongTimer = chrono::milliseconds::zero();
		}
		break;
	case PackageType::Logout:
		m_iState = ClientSessionState::CloseAfterSend;
		m_bShouldBeClosed = true;
		m_pLogger->Log(StringFormat("客户端登出，昵称: %s，虚拟ip: %s。(%s:%u)", m_Nickname.c_str(), m_VirtualAddr.c_str(), m_sIP.c_str(), m_uPort));
		m_pServer->OnClientLogout(this);
		break;
	default:
		throw logic_error("unexpected package type.");
	}
}

void ClientSession::recv()
{
	m_iRecvTimer = chrono::milliseconds::zero();

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
		} while (tReaded >= sizeof(tBuf));  // 当且仅当tReaded == sizeof(tBuf)时说明还有数据没读完
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
		m_iLoginTimer += tick;
		if (m_iLoginTimer > sc_LoginTimeout)
			throw logic_error(StringFormat("login timeout in %d ms.", sc_LoginTimeout));
		break;
	case ClientSessionState::Logined:
		if (m_bPingSent)
		{
			m_iPongTimer += tick;
			if (m_iPongTimer > sc_PongTimeout)
				throw logic_error(StringFormat("pong timeout in %d ms.", sc_PongTimeout));
		}
		else
		{
			m_iPingTimer += tick;
			if (m_iPingTimer > sc_PingPeriod)
			{
				sendPing();
				m_bPingSent = true;
				m_iPingTimer = chrono::milliseconds::zero();
			}
		}
		break;
	}

	m_iRecvTimer += tick;
	if (m_iRecvTimer > sc_RecvTimeout)
		throw logic_error(StringFormat("no data transfer in %d ms.", sc_RecvTimeout.count()));
}
