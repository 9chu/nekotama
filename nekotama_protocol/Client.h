#pragma once
#include <cstdint>
#include <string>
#include <atomic>
#include <thread>

#include <ISocket.h>
#include <Bencode.h>

#include "ConcurrentQueue.h"
#include "ILogger.h"
#include "Package.h"

namespace nekotama
{
	/// @brief �ͻ���
	class Client
	{
	public:
		/// @brief ��Ϸ��Ϣ
		struct GameInfo
		{
			std::string Nickname;
			std::string VirtualAddr;
			uint16_t VirtualPort;
			uint32_t Delay;

			GameInfo() {}
			GameInfo(const GameInfo& org)
				: Nickname(org.Nickname), VirtualAddr(org.VirtualAddr), VirtualPort(org.VirtualPort), Delay(org.Delay) {}
		};
	private:
		ISocketFactory* m_pFactory;
		ILogger* m_pLogger;
		std::string m_sIP;
		uint16_t m_iPort;

		SocketHandle m_pSocket;
		
		// �߳�״̬
		std::atomic<bool> m_stopFlag;  // ֪ͨ�߳�ֹͣ�����ı��
		std::atomic<bool> m_bRunning;  // ָʾ������Ƿ�������
		std::shared_ptr<std::thread> m_threadMain;  // �������߳�
		ConcurrentQueue<Bencode::Value> m_sendQueue;  // ����������

		// �ͻ�����
		std::string m_sServerName;
		std::string m_sNickname;
		std::string m_sVirtualAddr;
		uint16_t m_iGamePort;
		uint32_t m_iDelay;
	private:
		bool poll(const Bencode::Value& v);
		void mainThreadLoop()NKNOEXCEPT;
		void sendThreadLoop()NKNOEXCEPT;
	public:
		/// @brief ��ȡ����������
		const std::string& GetServerName()const NKNOEXCEPT { return m_sServerName; }
		/// @brief ��ȡ�ǳ�
		const std::string& GetNickname()const NKNOEXCEPT { return m_sNickname; }
		/// @brief ��ȡ�����ַ
		const std::string& GetVirtualAddr()const NKNOEXCEPT { return m_sVirtualAddr; }
		/// @brief ��ȡ��Ϸ�˿�
		uint16_t GetGamePort()const NKNOEXCEPT { return m_iGamePort; }
		/// @brief ��ȡ�ӳ�
		uint32_t GetDelay()const NKNOEXCEPT { return m_iDelay; }

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
		/// @brief ���ŵضϿ�����
		/// @note  �첽����
		void GentlyStop();

		/// @brief ֪ͨ������Ϸ
		void NotifyGameCreated();
		/// @brief ֪ͨ������Ϸ
		void NotifyGameDestroyed();
		/// @brief ֪ͨ��ѯ������Ϣ
		void QueryGameInfo();
		/// @brief ת�����ݰ�
		void ForwardingPackage(const std::string& target_addr, uint16_t target_port, uint16_t source_port, const std::string& data);
	protected:
		virtual void OnConnectFailed()NKNOEXCEPT {}
		virtual void OnNotSupportedServerVersion()NKNOEXCEPT {}
		virtual void OnKicked(KickReason why)NKNOEXCEPT {}
		virtual void OnLostConnection()NKNOEXCEPT {}
		virtual void OnLoginSucceed(const std::string& server, const std::string& nickname, const std::string& addr, uint16_t gameport)NKNOEXCEPT {}
		virtual void OnDataArrival(const std::string& source_ip, uint16_t source_port, uint16_t target_port, const std::string& data)NKNOEXCEPT {}
		virtual void OnRefreshGameInfo(const std::vector<GameInfo>& info)NKNOEXCEPT {}
		virtual void OnDelayRefreshed()NKNOEXCEPT {}
	public:
		Client(ISocketFactory* pFactory, ILogger* pLogger, const std::string& serverip, const std::string& nickname, uint16_t port = 12801);
		~Client();
	};
}
