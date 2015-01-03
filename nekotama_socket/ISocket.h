#pragma once
#include <memory>
#include <cstdint>
#include <unordered_set>
#include <string>

#include "Util.h"

namespace nekotama
{
	struct ISocket;
	typedef std::shared_ptr<ISocket> SocketHandle;
	typedef std::unordered_set<SocketHandle> SocketHandleSet;

	/// @brief Socket类型
	enum class SocketType
	{
		TCP,
		UDP
	};

	/// @brief Socket关闭类型
	enum class SocketShutdownType
	{
		CloseRead,
		CloseWrite,
		CloseReadWrite
	};

	/// @brief Socket工厂类
	struct ISocketFactory
	{
		virtual SocketHandle Create(SocketType type) = 0;
		/// @brief  Select操作封装
		/// @note   若socket对象可操作则留在集合中
		/// @param  read    可选的、需要检查可读性的socket接口
		/// @param  write   可选的、需要检查可写性的socket接口
		/// @param  error   可选的、需要检查错误的socket接口
		/// @param  timeout 超时时间（毫秒）
		/// @return 若有对象可操作则返回true
		virtual bool Select(SocketHandleSet* read, SocketHandleSet* write, SocketHandleSet* error, uint32_t timeout) = 0;
	};

	/// @brief Socket操作封装
	/// @note  不直接使用socket中的函数，通过接口要求外部实现提供底层实现\
	///        用于适应Hook与非Hook环境下的Socket调用。
	struct ISocket
	{
		virtual uint32_t GetNativeHandle()NKNOEXCEPT = 0;

		virtual SocketType GetType()const NKNOEXCEPT = 0;
		virtual void IOControl(int32_t cmd, uint32_t* args) = 0;
		virtual void SetBlockingMode(bool blocking) = 0;

		virtual void Bind(const char* addr, uint16_t port) = 0;
		virtual void Listen(uint32_t queue = 5) = 0;
		virtual bool Accept(std::string& ip, uint16_t& port, SocketHandle& handle) = 0;
		virtual void Shutdown(SocketShutdownType how) = 0;
		virtual void Connect(const char* addr, uint16_t port) = 0;

		virtual bool Recv(uint8_t* buffer, size_t length, size_t& readed, uint32_t flags = 0) = 0;
		virtual bool Send(const uint8_t* buffer, size_t length, size_t& wrote, uint32_t flags = 0) = 0;
		virtual bool TestIfClosed() = 0;
	};
}
