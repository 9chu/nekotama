#pragma once
#include <atomic>
#include <thread>
#include <stdexcept>
#include <unordered_map>

#include "ClientSession.h"

namespace nekotama
{
	/// @brief 服务
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

		// 线程状态
		std::atomic<bool> m_stopFlag;  // 通知线程停止工作的标记
		std::atomic<bool> m_bRunning;  // 指示服务端是否在运行
		std::shared_ptr<std::thread> m_threadMain;  // 主工作线程

		// 会话列表
		std::unordered_map<SocketHandle, ClientSessionHandle> m_mpClients;
	private:
		void mainThreadLoop()NKNOEXCEPT;
	public:
		ISocketFactory* GetSocketFactory()NKNOEXCEPT { return m_pFactory; }
		ILogger* GetLogger()NKNOEXCEPT { return m_pLogger; }
		uint16_t GetMaxClients()NKNOEXCEPT { return m_uMaxClients; }
		const std::string& GetServerName()NKNOEXCEPT { return m_sServerName; }

		/// @brief 启动服务
		/// @note  异步调用
		void Start();
		/// @brief 结束服务
		/// @note  异步调用
		void Stop();
		/// @brief 等待服务线程终止
		void Wait();
		/// @brief 是否在运行
		bool IsRunning()const NKNOEXCEPT;
	protected:  // 可以实现的函数
		virtual void OnClientArrival(ClientSession* client, bool kicked_for_full)NKNOEXCEPT {}
		virtual void OnClientLeave(ClientSession* client)NKNOEXCEPT {}
		virtual void OnClientInvalid(ClientSession* client)NKNOEXCEPT {}
		virtual bool OnClientLogin(ClientSession* client, std::string& nick, std::string& addr, uint16_t port)NKNOEXCEPT { return false; }
		virtual void OnClientLogout(ClientSession* client)NKNOEXCEPT {}
	private:
		Server& operator=(const Server&);
		Server(const Server&);
	public:
		/// @brief 初始化服务
		Server(ISocketFactory* pFactory, ILogger* pLogger, const std::string& server_name, uint16_t maxClient = 16, uint16_t port = 12801);
	};
}
