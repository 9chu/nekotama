#pragma once
#include <string>
#include <stdexcept>

#include "ISocket.h"

namespace nekotama
{
	/// @brief Socket工厂
	class SocketFactory :
		public ISocketFactory
	{
	public:
		static SocketFactory& GetInstance();
	public:
		SocketHandle Create(SocketType type);
		bool Select(SocketHandleSet* read, SocketHandleSet* write, SocketHandleSet* error, uint32_t timeout);
	private:
		SocketFactory& operator=(const SocketFactory&);
		SocketFactory(const SocketFactory&);
	protected:
		SocketFactory();
	public:
		~SocketFactory();
	};

	/// @brief Socket接口实现
	class Socket :
		public ISocket
	{
		friend class SocketFactory;
	private:
		SocketType m_Type;
		uint32_t m_Socket;
	public:
		uint32_t GetNativeHandle()NKNOEXCEPT { return m_Socket; }

		SocketType GetType()const NKNOEXCEPT { return m_Type; }
		void IOControl(int32_t cmd, uint32_t* args);
		void SetBlockingMode(bool blocking);

		void Bind(const char* addr, uint16_t port);
		void Listen(uint32_t queue);
		bool Accept(std::string& ip, uint16_t& port, SocketHandle& handle);
		void Shutdown(SocketShutdownType how);
		void Connect(const char* addr, uint16_t port);
		
		bool Recv(uint8_t* buffer, size_t length, size_t& readed, uint32_t flags);
		bool Send(const uint8_t* buffer, size_t length, size_t& wrote, uint32_t flags);
		bool TestIfClosed();
	private:
		Socket(const Socket&);
		Socket& operator=(const Socket&);
	protected:
		Socket(SocketType type, uint32_t socket);
	public:
		~Socket();
	};
}
