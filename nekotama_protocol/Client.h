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
	private:
		bool poll(const Bencode::Value& v);
		void mainThreadLoop()NKNOEXCEPT;
		void sendThreadLoop()NKNOEXCEPT;
	public:
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
	protected:
		virtual void OnConnectFailed()NKNOEXCEPT {}
		virtual void OnNotSupportedServerVersion()NKNOEXCEPT {}
		virtual void OnKicked(KickReason why)NKNOEXCEPT {}
		virtual void OnLostConnection()NKNOEXCEPT {}
		virtual void OnLoginSucceed(const std::string& server, const std::string& nickname, const std::string& addr, uint16_t gameport)NKNOEXCEPT {}
	public:
		Client(ISocketFactory* pFactory, ILogger* pLogger, const std::string& serverip, const std::string& nickname, uint16_t port = 12801);
	};
}
