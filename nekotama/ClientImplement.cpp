#include "ClientImplement.h"

#include "NonOutputLogger.h"
#include "HookedSocket.h"
#include "Encoding.h"

using namespace nekotama;

ClientImplement::ClientImplement(const std::shared_ptr<ClientRenderer>& renderer, const std::string& serverip, const std::string& nickname, uint16_t port)
	: Client(&HookedSocketFactory::GetInstance(), &NonOutputLogger::GetInstance(), serverip, nickname, port), m_ClientRenderer(renderer)
{}

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
}

void ClientImplement::OnRefreshGameInfo(const std::vector<GameInfo>& info)NKNOEXCEPT
{
}

void ClientImplement::OnDelayRefreshed()NKNOEXCEPT
{
	m_ClientRenderer->SetDelay(GetDelay());
}
