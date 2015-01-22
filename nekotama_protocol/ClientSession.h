#pragma once
#include <deque>
#include <chrono>

#include <Bencode.h>
#include <ISocket.h>

#include "Package.h"
#include "ILogger.h"

namespace nekotama
{
	class Server;
	class ClientSession;
	typedef std::shared_ptr<ClientSession> ClientSessionHandle;
	
	/// @brief 用户会话
	class ClientSession
	{
		friend class Server;
		enum class ClientSessionState
		{
			Invalid,

			CloseAfterSend,
			WaitForLogin,
			Logined
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

		std::string m_Nickname;
		std::string m_VirtualAddr;

		// === 计时器部分 ===
		bool m_bPingSent;
		std::chrono::milliseconds m_iLoginTimer;  // 距离login被收到经过的时间
		std::chrono::milliseconds m_iPingTimer;  // 距离ping被发送经过的时间
		std::chrono::milliseconds m_iPongTimer;  // 距离pong被收到经过的时间
		std::chrono::milliseconds m_iRecvTimer;  // 距离上次接受到数据包经过的时间
		std::chrono::milliseconds m_iDelay;  // 通信延迟
	public:
		SocketHandle GetSocket()NKNOEXCEPT { return m_cltSocket; }
		const std::string& GetIP()const NKNOEXCEPT { return m_sIP; }
		uint16_t GetPort()const NKNOEXCEPT { return m_uPort; }
		bool ShouldBeClosed()const NKNOEXCEPT{ return m_bShouldBeClosed; }
		bool HasData()const NKNOEXCEPT{ return m_iLastDataNotSent > 0 || !m_dataBuf.empty(); }
	protected:
		void sendWelcome(const std::string& server_name, uint32_t protocol_maj, uint32_t protocol_min);
		void sendKicked(KickReason reason);
		void sendLoginConfirm(const std::string& nick, const std::string& addr, uint16_t port);
		void sendPing();
	protected:
		void push(const Bencode::Value& v);  // 填报文
		void poll(const Bencode::Value& v);  // 处理报文
		void recv();  // 通知接收并处理数据
		void send();  // 通知发送数据（若有）
		void invalid();  // 通知会话已经无效
		void update(std::chrono::milliseconds tick);  // 通知更新时钟
	private:
		ClientSession& operator=(const ClientSession&);
		ClientSession(const ClientSession&);
	protected:
		ClientSession(Server* server, SocketHandle handle, const std::string& ip, uint16_t port, uint16_t count);
	public:
		~ClientSession();
	};
}
