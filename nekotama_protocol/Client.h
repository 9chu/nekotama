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
	/// @brief 客户端
	class Client
	{
	private:
		ISocketFactory* m_pFactory;
		ILogger* m_pLogger;
		std::string m_sIP;
		uint16_t m_iPort;

		SocketHandle m_pSocket;
		
		// 线程状态
		std::atomic<bool> m_stopFlag;  // 通知线程停止工作的标记
		std::atomic<bool> m_bRunning;  // 指示服务端是否在运行
		std::shared_ptr<std::thread> m_threadMain;  // 主工作线程
		ConcurrentQueue<Bencode::Value> m_sendQueue;  // 待发送数据

		// 客户数据
		std::string m_sServerName;
		std::string m_sNickname;
		std::string m_sVirtualAddr;
		uint16_t m_iGamePort;
	private:
		bool poll(const Bencode::Value& v);
		void mainThreadLoop()NKNOEXCEPT;
		void sendThreadLoop()NKNOEXCEPT;
	public:
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
		/// @brief 优雅地断开连接
		/// @note  异步调用
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
