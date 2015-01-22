#pragma once
#include <atomic>
#include <thread>
#include <stdexcept>
#include <unordered_map>

#include "ClientSession.h"

namespace nekotama
{
	/// @brief ����
	class Server
	{
		friend class ClientSession;
	private:
		ISocketFactory* m_pFactory;
		ILogger* m_pLogger;
		uint16_t m_uPort;
		uint16_t m_uMaxClients;
		std::string m_sServerName;

		SocketHandle m_srvSocket;

		// �߳�״̬
		std::atomic<bool> m_stopFlag;  // ֪ͨ�߳�ֹͣ�����ı��
		std::atomic<bool> m_bRunning;  // ָʾ������Ƿ�������
		std::shared_ptr<std::thread> m_threadMain;  // �������߳�

		// �Ự�б�
		std::unordered_map<SocketHandle, ClientSessionHandle> m_mpClients;
	private:
		void mainThreadLoop()NKNOEXCEPT;
	public:
		ISocketFactory* GetSocketFactory()NKNOEXCEPT { return m_pFactory; }
		ILogger* GetLogger()NKNOEXCEPT { return m_pLogger; }
		uint16_t GetMaxClients()NKNOEXCEPT { return m_uMaxClients; }
		const std::string& GetServerName()NKNOEXCEPT { return m_sServerName; }

		/// @brief ��������
		/// @note  �첽����
		void Start();
		/// @brief ��������
		/// @note  �첽����
		void Stop();
		/// @brief �ȴ������߳���ֹ
		void Wait();
		/// @brief �Ƿ�������
		bool IsRunning()const NKNOEXCEPT;
	protected:  // ����ʵ�ֵĺ���
		virtual void OnClientArrival(ClientSession* client, bool kicked_for_full)NKNOEXCEPT {}
		virtual void OnClientLeave(ClientSession* client)NKNOEXCEPT {}
		virtual void OnClientInvalid(ClientSession* client)NKNOEXCEPT {}
		virtual bool OnClientLogin(ClientSession* client, std::string& nick, std::string& addr, uint16_t port)NKNOEXCEPT { return false; }
		virtual void OnClientLogout(ClientSession* client)NKNOEXCEPT {}
	private:
		Server& operator=(const Server&);
		Server(const Server&);
	public:
		/// @brief ��ʼ������
		Server(ISocketFactory* pFactory, ILogger* pLogger, const std::string& server_name, uint16_t maxClient = 16, uint16_t port = 12801);
	};
}
