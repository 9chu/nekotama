#include "ServerImplement.h"

#include <algorithm>

#include <Socket.h>

using namespace std;
using namespace nekotama;

////////////////////////////////////////////////////////////////////////////////

ServerImplement::ServerImplement(const std::string& server_name, uint16_t maxClient, uint16_t port)
	: Server(&SocketFactory::GetInstance(), &StdOutLogger::GetInstance(), server_name, maxClient, port),
	m_IpPool(0x0A000000, 0xFF000000)
{}

void ServerImplement::OnClientLeave(ClientSession* client)NKNOEXCEPT
{
	auto i = m_IpTable.begin();
	while (i != m_IpTable.end())
	{
		if (i->second == client)
		{
			m_IpPool.Free(VirtualIpPool::StringToIp(i->first));
			i = m_IpTable.erase(i);
		}
		else
			++i;
	}

	auto j = m_NickTable.begin();
	while (j != m_NickTable.end())
	{
		if (j->second == client)
			j = m_NickTable.erase(j);
		else
			++j;
	}

	auto k = m_GameList.find(client);
	if (k != m_GameList.end())
		m_GameList.erase(k);
}

bool ServerImplement::OnClientLogin(ClientSession* client, std::string& nick, std::string& addr, uint16_t& port)NKNOEXCEPT
{
	// 检查昵称是否合法
	if (nick.length() > 16 || nick.length() < 3)
		return false;

	// 获得一个ip地址
	uint32_t tIpAddr = m_IpPool.Alloc();
	if (tIpAddr == 0)
		return false;
	addr = VirtualIpPool::IpToString(tIpAddr);
	port = 10800;

	// 组合一个可用的昵称
	int i = 0;
	std::string tOrgNick = nick;
	while (m_NickTable.find(nick) != m_NickTable.end())
	{
		++i;
		nick = StringFormat("%s (%d)", tOrgNick.c_str(), i);
	}

	// 注册登记
	m_IpTable[addr] = client;
	m_NickTable[nick] = client;

	return true;
}

void ServerImplement::OnClientPackageReceived(ClientSession* client, const std::string& target, uint16_t target_port, uint16_t source_port, const std::string& data)NKNOEXCEPT
{
	// 找到接受方
	auto i = m_IpTable.find(target);
	if (i == m_IpTable.end())
		return;
	i->second->ForwardingPackage(client->GetVirtualAddr(), source_port, target_port, data);
}

void ServerImplement::OnClientHostCreated(ClientSession* client)NKNOEXCEPT
{
	if (m_GameList.find(client) == m_GameList.end())
		m_GameList.insert(client);
}

void ServerImplement::OnClientHostDestroyed(ClientSession* client)NKNOEXCEPT
{
	auto i = m_GameList.find(client);
	if (i != m_GameList.end())
		m_GameList.erase(i);
}

const std::set<ClientSession*>& ServerImplement::OnClientQueryGames(ClientSession* client)NKNOEXCEPT
{
	return m_GameList;
}
