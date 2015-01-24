#pragma once
#include <Server.h>
#include <VirtualIpPool.h>

#include "StdOutLogger.h"

namespace nekotama
{
	/// @brief 完整服务端实现
	class ServerImplement :
		public Server
	{
	protected:
		VirtualIpPool m_IpPool;
		std::unordered_map<std::string, ClientSession*> m_IpTable;
		std::unordered_map<std::string, ClientSession*> m_NickTable;
		std::set<ClientSession*> m_GameList;
	protected:
		void OnClientLeave(ClientSession* client)NKNOEXCEPT;
		bool OnClientLogin(ClientSession* client, std::string& nick, std::string& addr, uint16_t& port)NKNOEXCEPT;
		void OnClientPackageReceived(ClientSession* client, const std::string& target, uint16_t target_port, uint16_t source_port, const std::string& data)NKNOEXCEPT;
		void OnClientHostCreated(ClientSession* client)NKNOEXCEPT;
		void OnClientHostDestroyed(ClientSession* client)NKNOEXCEPT;
		const std::set<ClientSession*>& OnClientQueryGames(ClientSession* client)NKNOEXCEPT;
	public:
		ServerImplement(const std::string& server_name, uint16_t maxClient = 16, uint16_t port = 12801);
	};
}
