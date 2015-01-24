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
	public:
		/// @brief 游戏信息
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
		uint32_t m_iDelay;
	private:
		bool poll(const Bencode::Value& v);
		void mainThreadLoop()NKNOEXCEPT;
		void sendThreadLoop()NKNOEXCEPT;
	public:
		/// @brief 获取服务器名称
		const std::string& GetServerName()const NKNOEXCEPT { return m_sServerName; }
		/// @brief 获取昵称
		const std::string& GetNickname()const NKNOEXCEPT { return m_sNickname; }
		/// @brief 获取虚拟地址
		const std::string& GetVirtualAddr()const NKNOEXCEPT { return m_sVirtualAddr; }
		/// @brief 获取游戏端口
		uint16_t GetGamePort()const NKNOEXCEPT { return m_iGamePort; }
		/// @brief 获取延迟
		uint32_t GetDelay()const NKNOEXCEPT { return m_iDelay; }

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

		/// @brief 通知开启游戏
		void NotifyGameCreated();
		/// @brief 通知结束游戏
		void NotifyGameDestroyed();
		/// @brief 通知查询房间信息
		void QueryGameInfo();
		/// @brief 转发数据包
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
