#pragma once
#include <memory>

#include <ConcurrentQueue.h>

#include "SocketHook.h"

namespace nekotama
{
	class ClientImplement;

	/// @brief ÐéÄâµÄUDPÁ¬½Ó
	class VirtualUDPConnection :
		public ISocketHooker
	{
		struct DataPackage
		{
			std::string ip;
			uint16_t port;
			std::string data;

			DataPackage()
				: port(0) {}
			DataPackage(const std::string& i_ip, uint16_t i_port, const std::string& idata)
				: ip(i_ip), port(i_port), data(idata) {}
			DataPackage(const DataPackage& org)
				: ip(org.ip), port(org.port), data(org.data) {}
			DataPackage(DataPackage&& org)
				: ip(org.ip), port(org.port), data(org.data) {}
		};
	private:
		std::weak_ptr<ClientImplement> m_pImplement;
		uint16_t m_iBindedPort;

		bool m_bRecvShuted;
		ConcurrentQueue<DataPackage> m_Packages;
	private:
		void closeRecv();
	public:
		void ReceiveData(const std::string& ip, uint16_t port, const std::string& data);
	public:
		int Bind(SOCKET s, const struct sockaddr* addr, int namelen);
		int CloseSocket(SOCKET s);
		int RecvFrom(SOCKET s, char* buf, int len, int flags, struct sockaddr* from, int* fromlen);
		int SendTo(SOCKET s, const char* buf, int len, int flags, const struct sockaddr* to, int tolen);
		int Shutdown(SOCKET s, int how);
	public:
		VirtualUDPConnection(const std::weak_ptr<ClientImplement>& pImplement);
		~VirtualUDPConnection();
	};
}
