#include "ClientSession.h"

#include "Server.h"
#include "Version.h"

#define MAXPACKAGESIZE 4096

using namespace std;
using namespace Bencode;
using namespace nekotama;

////////////////////////////////////////////////////////////////////////////////

ClientSession::ClientSession(Server* server, SocketHandle handle, const std::string& ip, uint16_t port, uint16_t count)
	: m_pServer(server), m_pLogger(server->GetLogger()), m_cltSocket(handle), m_sIP(ip), m_uPort(port),
	m_iState(ClientSessionState::Invalid), m_bShouldBeClosed(false), m_iBytesRecved(0), m_iLastDataNotSent(0)
{
	// 检查会话上限
	if (count >= server->GetMaxClients())
	{
		m_pLogger->Log(StringFormat("连接人数达到服务器上限，已拒绝连接。(%s:%u)", m_sIP.c_str(), m_uPort), LogType::Error);
		SendKicked(ClientSessionKickReason::ServerIsFull);
		m_iState = ClientSessionState::CloseAfterSend;
		m_pServer->OnClientArrival(this, true);
	}
	else
	{
		m_pLogger->Log(StringFormat("已建立到客户端的连接，等待登陆响应。(%s:%u)", m_sIP.c_str(), m_uPort));
		SendWelcome(m_pServer->GetServerName(), NK_PROTOCOL_MAJOR, NK_PROTOCOL_MINOR);
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

void ClientSession::SendWelcome(const std::string& server_name, uint32_t protocol_maj, uint32_t protocol_min)
{
	Value tPackage(ValueType::Dictionary);
	tPackage.VDict["type"] = make_shared<Value>((IntType)ClientSessionPackageType::Welcome);
	tPackage.VDict["name"] = make_shared<Value>(server_name);
	tPackage.VDict["pmaj"] = make_shared<Value>((IntType)protocol_maj);
	tPackage.VDict["pmin"] = make_shared<Value>((IntType)protocol_min);
	push(tPackage);
}

void ClientSession::SendKicked(ClientSessionKickReason reason)
{
	Value tPackage(ValueType::Dictionary);
	tPackage.VDict["type"] = make_shared<Value>((IntType)ClientSessionPackageType::Kicked);
	tPackage.VDict["why"] = make_shared<Value>((IntType)reason);
	push(tPackage);
}

void ClientSession::SendLoginConfirm()
{
	Value tPackage(ValueType::Dictionary);
	tPackage.VDict["type"] = make_shared<Value>((IntType)ClientSessionPackageType::LoginConfirm);
	push(tPackage);
}

void ClientSession::SendPing()
{
	Value tPackage(ValueType::Dictionary);
	tPackage.VDict["type"] = make_shared<Value>((IntType)ClientSessionPackageType::Ping);
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
	// TODO
}

void ClientSession::recv()
{
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

void ClientSession::update(uint32_t tick)
{
	// TODO
}
