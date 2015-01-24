#include "ClientImplement.h"

#include "NonOutputLogger.h"
#include "HookedSocket.h"
#include "Encoding.h"

using namespace nekotama;

ClientImplement::ClientImplement(const std::shared_ptr<ClientRenderer>& renderer, const std::string& serverip, const std::string& nickname, uint16_t port)
	: Client(&HookedSocketFactory::GetInstance(), &NonOutputLogger::GetInstance(), serverip, nickname, port), m_ClientRenderer(renderer)
{}

bool ClientImplement::RegisterConnection(uint16_t port, VirtualUDPConnection* p)NKNOEXCEPT
{
	std::unique_lock<std::mutex> lock(m_ConnectionMutex);

	auto i = m_pConnections.find(port);
	if (i != m_pConnections.end() || p == nullptr)
		return false;
	m_pConnections[port] = p;
	if (port == GetGamePort())
		NotifyGameCreated();
	m_ClientRenderer->ShowHint(L"连接建立");
	return true;
}

void ClientImplement::UnregisterConnection(uint16_t port)NKNOEXCEPT
{
	std::unique_lock<std::mutex> lock(m_ConnectionMutex);

	auto i = m_pConnections.find(port);
	if (i != m_pConnections.end())
	{
		if (port == GetGamePort())
			NotifyGameDestroyed();
		m_pConnections.erase(i);
		m_ClientRenderer->ShowHint(L"连接关闭");
	}	
}

uint16_t ClientImplement::AllocFreePort()
{
	std::unique_lock<std::mutex> lock(m_ConnectionMutex);

	uint16_t tPort = 50000 + rand() % 10000;
	while (m_pConnections.find(tPort) != m_pConnections.end())
		tPort = 50000 + rand() % 10000;
	return tPort;
}

void ClientImplement::OnConnectFailed()NKNOEXCEPT
{
	m_ClientRenderer->ShowHint(L"服务器连接失败");
}

void ClientImplement::OnNotSupportedServerVersion()NKNOEXCEPT
{
	m_ClientRenderer->ShowHint(L"服务器版本不符");
}

void ClientImplement::OnKicked(KickReason why)NKNOEXCEPT
{
	switch (why)
	{
	case KickReason::LoginFailed:
		m_ClientRenderer->ShowHint(L"被服务器踢出\n原因：登陆失败");
		break;
	case KickReason::ServerClosed:
		m_ClientRenderer->ShowHint(L"被服务器踢出\n原因：服务器关闭");
		break;
	case KickReason::ServerIsFull:
		m_ClientRenderer->ShowHint(L"被服务器踢出\n原因：服务器已满");
		break;
	default:
		m_ClientRenderer->ShowHint(L"被服务器踢出");
		break;
	}
}

void ClientImplement::OnLostConnection()NKNOEXCEPT
{
	m_ClientRenderer->ShowHint(L"已从服务器断开");
	m_ClientRenderer->SetDelay();
}

void ClientImplement::OnLoginSucceed(const std::string& server, const std::string& nickname, const std::string& addr, uint16_t gameport)NKNOEXCEPT
{
	m_ClientRenderer->ShowHint(
		StringFormat(
			L"服务器连接成功\n昵称：%s\n%s:%d", 
			MultiByteToWideChar(nickname.c_str()).c_str(), 
			MultiByteToWideChar(addr.c_str()).c_str(),
			gameport
			)
		);
}

void ClientImplement::OnDataArrival(const std::string& source_ip, uint16_t source_port, uint16_t target_port, const std::string& data)NKNOEXCEPT
{
	std::unique_lock<std::mutex> lock(m_ConnectionMutex);

	auto i = m_pConnections.find(target_port);
	if (i != m_pConnections.end())
	{
		i->second->ReceiveData(source_ip, source_port, data);
	}
}

void ClientImplement::OnRefreshGameInfo(const std::vector<GameInfo>& info)NKNOEXCEPT
{
	/// !TODO
}

void ClientImplement::OnDelayRefreshed()NKNOEXCEPT
{
	m_ClientRenderer->SetDelay(GetDelay());
}
