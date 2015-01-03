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

	/// @brief Socket����
	enum class SocketType
	{
		TCP,
		UDP
	};

	/// @brief Socket�ر�����
	enum class SocketShutdownType
	{
		CloseRead,
		CloseWrite,
		CloseReadWrite
	};

	/// @brief Socket������
	struct ISocketFactory
	{
		virtual SocketHandle Create(SocketType type) = 0;
		/// @brief  Select������װ
		/// @note   ��socket����ɲ��������ڼ�����
		/// @param  read    ��ѡ�ġ���Ҫ���ɶ��Ե�socket�ӿ�
		/// @param  write   ��ѡ�ġ���Ҫ����д�Ե�socket�ӿ�
		/// @param  error   ��ѡ�ġ���Ҫ�������socket�ӿ�
		/// @param  timeout ��ʱʱ�䣨���룩
		/// @return ���ж���ɲ����򷵻�true
		virtual bool Select(SocketHandleSet* read, SocketHandleSet* write, SocketHandleSet* error, uint32_t timeout) = 0;
	};

	/// @brief Socket������װ
	/// @note  ��ֱ��ʹ��socket�еĺ�����ͨ���ӿ�Ҫ���ⲿʵ���ṩ�ײ�ʵ��\
	///        ������ӦHook���Hook�����µ�Socket���á�
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
