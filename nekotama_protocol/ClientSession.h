#pragma once
#include <deque>

#include <ISocket.h>

#include "ILogger.h"
#include "Package.h"

namespace nekotama
{
	class Server;
	class ClientSession;
	typedef std::shared_ptr<ClientSession> ClientSessionHandle;

	/// @brief �û��Ự
	class ClientSession
	{
		friend class Server;
	public:
		typedef std::deque<uint8_t> BufferType;
	private:
		ILogger* m_pLogger;
		SocketHandle m_Socket;
		std::string m_IP;
		uint16_t m_Port;

		// ���ݻ�����
		PackageValidChecker m_Checker;
		BufferType m_RecvBuf;
		BufferType m_SendBuf;
	public:
		const std::string& GetIP()const { return m_IP; }
		uint16_t GetPort()const { return m_Port; }

	protected:  // �ײ����ݴ�����ƺ���
		/// @brief  ��������ֱ�����һ�����
		/// @return �������������һ������򷵻�true
		bool RecvData();
		/// @brief  ��������δ���͵�����
		void SendData();
		/// @brief  ��ʱ������
		void Tick(uint32_t TickCount);
		/// @brief  ��ȡ��ȡ������
		const BufferType& GetReadBuffer()const { return m_RecvBuf; }
		/// @brief  ��ȡд�뻺����
		BufferType& GetWriteBuffer() { return m_SendBuf; }
	public:
		ClientSession(SocketHandle handle, const std::string& ip, uint16_t port, ILogger* pLogger);
	};
}
