#pragma once
#include <atomic>
#include <thread>
#include <stdexcept>

#include <ISocket.h>

#include "ILogger.h"
#include "ConcurrentQueue.h"
#include "ClientSession.h"

namespace nekotama
{
	class Server
	{
		/// @brief 消息类型
		enum class MessageType
		{
			None = 0,
			WorkStopped,
			ClientArrival,
			ClientRemoved,
			ClientPackageArrival
		};
		/// @brief 消息
		struct Message
		{
			MessageType Type;
			union
			{
				ClientSession* Session;
			};
			Message()
				: Type(MessageType::None) {}
			Message(MessageType type)
				: Type(type) {}
			Message(MessageType type, ClientSession* session)
				: Type(type), Session(session) {}
		};
	private:
		ISocketFactory* m_pFactory;
		ILogger* m_pLogger;
		uint16_t m_Port;
		uint16_t m_Clients;

		SocketHandle m_Socket;

		// 线程状态
		std::atomic<bool> m_stopFlag;  // 通知线程停止工作的标记
		std::atomic<bool> m_bRunning;  // 指示服务端是否在运行
		std::shared_ptr<std::thread> m_tSocket;  // Socket与计时器工作线程
		std::shared_ptr<std::thread> m_tMsgWork;  // 消息处理线程

		// 消息队列
		ConcurrentQueue<Message> m_MsgQueue;
	protected:
		void socketThreadLoop()NKNOEXCEPT;
		void messageThreadLoop()NKNOEXCEPT;
	public:
		/// @brief 启动服务
		/// @note  异步调用
		void Start();
		/// @brief 结束服务
		/// @note  异步调用
		void End();
		/// @brief 等待服务线程终止
		void Wait();
		/// @brief 是否在运行
		bool IsRunning()const throw();
	private:
		Server& operator=(const Server&);
		Server(const Server&);
	public:
		/// @brief 初始化服务
		Server(ISocketFactory* pFactory, ILogger* pLogger, uint16_t port = 12801, uint16_t maxClient = 32);
	};
}
