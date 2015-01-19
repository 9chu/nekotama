#pragma once
#include <deque>

#include <Bencode.h>
#include <ISocket.h>

#include "ILogger.h"

namespace nekotama
{
	class Server;
	class ClientSession;
	typedef std::shared_ptr<ClientSession> ClientSessionHandle;
	
	/// @brief ���ݰ�����
	enum class ClientSessionPackageType
	{
		Welcome = 1,
		Kicked = 2,
		// Login = 3,
		LoginConfirm = 4,
		Ping = 5,
		// Pong = 6,
		// Logout = 7
	};

	/// @brief �ͻ��˻Ự�߳�ԭ��
	enum class ClientSessionKickReason
	{
		ServerIsFull,
		Timeout,
		WrongUsername,
		WrongPasswd
	};

	/// @brief �û��Ự
	class ClientSession
	{
		friend class Server;
		enum class ClientSessionState
		{
			Invalid,

			CloseAfterSend,
			WaitForLogin,
		};
	private:
		Server* m_pServer;
		ILogger* m_pLogger;
		SocketHandle m_cltSocket;
		std::string m_sIP;
		uint16_t m_uPort;
		ClientSessionState m_iState;
		bool m_bShouldBeClosed;

		size_t m_iBytesRecved;
		Bencode::Decoder m_dDecoder;
		Bencode::Encoder m_dEncoder;
		std::deque<std::string> m_dataBuf;
		std::string m_sLastData;
		size_t m_iLastDataNotSent;
	public:
		SocketHandle GetSocket()NKNOEXCEPT { return m_cltSocket; }
		const std::string& GetIP()const NKNOEXCEPT { return m_sIP; }
		uint16_t GetPort()const NKNOEXCEPT { return m_uPort; }
		bool ShouldBeClosed()const NKNOEXCEPT{ return m_bShouldBeClosed; }
		bool HasData()const NKNOEXCEPT{ return m_iLastDataNotSent > 0 || !m_dataBuf.empty(); }
	public:  // ���ݰ�����
		void SendWelcome(const std::string& server_name, uint32_t protocol_maj, uint32_t protocol_min);
		void SendKicked(ClientSessionKickReason reason);
		void SendLoginConfirm();
		void SendPing();
	protected:
		void push(const Bencode::Value& v);  // ���
		void poll(const Bencode::Value& v);  // ������
		void recv();  // ֪ͨ���ղ���������
		void send();  // ֪ͨ�������ݣ����У�
		void invalid();  // ֪ͨ�Ự�Ѿ���Ч
		void update(uint32_t tick);  // ֪ͨ����ʱ��
	private:
		ClientSession& operator=(const ClientSession&);
		ClientSession(const ClientSession&);
	protected:
		ClientSession(Server* server, SocketHandle handle, const std::string& ip, uint16_t port, uint16_t count);
	public:
		~ClientSession();
	};
}
