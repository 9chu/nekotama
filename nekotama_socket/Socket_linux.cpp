#include "Socket.h"

#include <atomic>
#include <algorithm>
#include <cstring>

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;
using namespace nekotama;

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define LASTWSAERR strerror(errno)

typedef int SOCKET;

////////////////////////////////////////////////////////////////////////////////

SocketFactory& SocketFactory::GetInstance()
{
	static std::unique_ptr<SocketFactory> s_Factory;
	if (!s_Factory)
		s_Factory.reset(new SocketFactory());
	return *(s_Factory.get());
}

SocketFactory::SocketFactory()
{
}

SocketFactory::~SocketFactory()
{
}

SocketHandle SocketFactory::Create(SocketType type)
{
	uint32_t tSocket;
	switch (type)
	{
	case SocketType::TCP:
		tSocket = socket(AF_INET, SOCK_STREAM, 0);
		break;
	case SocketType::UDP:
		tSocket = socket(AF_INET, SOCK_DGRAM, 0);
		break;
	default:
		throw invalid_argument("arg 'type' is invalid.");
	}
	if (tSocket == INVALID_SOCKET)
		throw logic_error(StringFormat("@%s: %s", "socket", LASTWSAERR));
	return static_pointer_cast<ISocket>(shared_ptr<Socket>(new Socket(type, tSocket)));
}

bool SocketFactory::Select(SocketHandleSet* read, SocketHandleSet* write, SocketHandleSet* error, uint32_t timeout)
{
	fd_set tRead, tWrite, tError;
	FD_ZERO(&tRead);
	FD_ZERO(&tWrite);
	FD_ZERO(&tError);
	size_t tMaxFd = 0;
	if (read)
	{
		if (read->size() >= FD_SETSIZE)
			throw logic_error("Select: socket handles are more than FD_SETSIZE.");
		for (auto p : *read)
		{
			FD_SET(p->GetNativeHandle(), &tRead);
			tMaxFd = max(tMaxFd, (size_t)p->GetNativeHandle());
		}
	}
	if (write)
	{
		if (write->size() >= FD_SETSIZE)
			throw logic_error("Select: socket handles are more than FD_SETSIZE.");
		for (auto p : *write)
		{
			FD_SET(p->GetNativeHandle(), &tWrite);
			tMaxFd = max(tMaxFd, (size_t)p->GetNativeHandle());
		}
	}
	if (error)
	{
		if (error->size() >= FD_SETSIZE)
			throw logic_error("Select: socket handles are more than FD_SETSIZE.");
		for (auto p : *error)
		{
			FD_SET(p->GetNativeHandle(), &tError);
			tMaxFd = max(tMaxFd, (size_t)p->GetNativeHandle());
		}
	}
	timeout *= 1000;
	timeval t = { timeout / 1000000, timeout % 1000000 };
	if (tMaxFd == 0)
		return false;
	int tRet = select(
		tMaxFd + 1,
		read && read->size() > 0 ? &tRead : nullptr,
		write && write->size() > 0 ? &tWrite : nullptr,
		error && error->size() > 0 ? &tError : nullptr,
		timeout == (uint32_t)-1 ? nullptr : &t
	);
	if (tRet == SOCKET_ERROR || tRet == 0)
	{
		if (read) read->clear();
		if (write) write->clear();
		if (error) error->clear();

		if (tRet == 0 || errno == EWOULDBLOCK || errno == EAGAIN)
			return false;
		else
			throw logic_error(StringFormat("@%s: %s", "select", LASTWSAERR));
	}

	if (read)
	{
		auto i = read->begin();
		while (i != read->end())
		{
			if (!FD_ISSET(i->get()->GetNativeHandle(), &tRead))
				i = read->erase(i);
			else
				++i;
		}
	}
	if (write)
	{
		auto i = write->begin();
		while (i != write->end())
		{
			if (!FD_ISSET(i->get()->GetNativeHandle(), &tWrite))
				i = write->erase(i);
			else
				++i;
		}
	}
	if (error)
	{
		auto i = error->begin();
		while (i != error->end())
		{
			if (!FD_ISSET(i->get()->GetNativeHandle(), &tError))
				i = error->erase(i);
			else
				++i;
		}
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////

Socket::Socket(SocketType type, uint32_t socket)
	: m_Type(type), m_Socket(socket)
{}

Socket::~Socket()
{
	close(m_Socket);
}

void Socket::IOControl(int32_t cmd, uint32_t* args)
{
	if (SOCKET_ERROR == ioctl(m_Socket, cmd, (u_long*)args))
		throw logic_error(StringFormat("@%s: %s", "ioctlsocket", LASTWSAERR));
}

void Socket::SetBlockingMode(bool blocking)
{
	int flags = fcntl(m_Socket, F_GETFL, 0);
    if (SOCKET_ERROR == fcntl(m_Socket, F_SETFL, blocking ? flags & ~O_NONBLOCK : flags|O_NONBLOCK))
		throw logic_error(StringFormat("@%s: %s", "fcntl", LASTWSAERR));
}

void Socket::Bind(const char* addr, uint16_t port)
{
	sockaddr_in tAddr;
	tAddr.sin_family = AF_INET;
	tAddr.sin_port = htons(port);
	tAddr.sin_addr.s_addr = inet_addr(addr);
	if (SOCKET_ERROR == bind(m_Socket, (sockaddr*)&tAddr, sizeof(tAddr)))
		throw logic_error(StringFormat("@%s: %s", "bind", LASTWSAERR));
}

void Socket::Listen(uint32_t queue)
{
	if (SOCKET_ERROR == listen(m_Socket, queue))
		throw logic_error(StringFormat("@%s: %s", "listen", LASTWSAERR));
}

bool Socket::Accept(std::string& ip, uint16_t& port, SocketHandle& handle)
{
	sockaddr_in tAddr;
	socklen_t size = sizeof(sockaddr_in);
	SOCKET tRet = accept(m_Socket, (sockaddr*)&tAddr, &size);
	if (tRet == INVALID_SOCKET)
	{
		ip.clear();
		port = 0;
		handle = nullptr;
		if (errno == EWOULDBLOCK || errno == EAGAIN)
			return false;
		throw logic_error(StringFormat("@%s: %s", "accept", LASTWSAERR));
	}
	if (size != sizeof(sockaddr_in))
	{
		close(tRet);
		return false;
	}
	ip = inet_ntoa(tAddr.sin_addr);
	port = ntohs(tAddr.sin_port);
	handle = static_pointer_cast<ISocket>(shared_ptr<Socket>(new Socket(SocketType::TCP, tRet)));
	return true;
}

void Socket::Shutdown(SocketShutdownType how)
{
	int tRet;
	switch (how)
	{
	case SocketShutdownType::CloseRead:
		tRet = shutdown(m_Socket, SHUT_RD);
		break;
	case SocketShutdownType::CloseWrite:
		tRet = shutdown(m_Socket, SHUT_WR);
		break;
	case SocketShutdownType::CloseReadWrite:
		tRet = shutdown(m_Socket, SHUT_RDWR);
		break;
	default:
		throw invalid_argument("arg 'how' is invalid.");
	}
	if (tRet == SOCKET_ERROR)
		throw logic_error(StringFormat("@%s: %s", "shutdown", LASTWSAERR));
}

void Socket::Connect(const char* addr, uint16_t port)
{
	sockaddr_in tAddr;
	tAddr.sin_family = AF_INET;
	tAddr.sin_port = htons(port);
	tAddr.sin_addr.s_addr = inet_addr(addr);
	int tRet = connect(m_Socket, (sockaddr*)&tAddr, sizeof(tAddr));
	if (tRet == SOCKET_ERROR && !(errno == EWOULDBLOCK || errno == EAGAIN))
		throw logic_error(StringFormat("@%s: %s", "connect", LASTWSAERR));
}

bool Socket::Recv(uint8_t* buffer, size_t length, size_t& readed, uint32_t flags)
{
	int tRet = recv(m_Socket, (char*)buffer, length, flags);
	if (tRet == 0)
	{
		readed = 0;
		return false;
	}
	else if (tRet == SOCKET_ERROR)
	{
		readed = 0;
		if (errno == EWOULDBLOCK || errno == EAGAIN)
			return false;
		throw logic_error(StringFormat("@%s: %s", "recv", LASTWSAERR));
	}
	readed = tRet;
	return true;
}

bool Socket::Send(const uint8_t* buffer, size_t length, size_t& wrote, uint32_t flags)
{
	int tRet = send(m_Socket, (char*)buffer, length, flags);
	if (tRet == 0)
	{
		wrote = 0;
		return false;
	}
	else if (tRet == SOCKET_ERROR)
	{
		wrote = 0;
		if (errno == EWOULDBLOCK || errno == EAGAIN)
			return false;
		throw logic_error(StringFormat("@%s: %s", "send", LASTWSAERR));
	}
	wrote = tRet;
	return true;
}

bool Socket::TestIfClosed()
{
	char c;
	int tRet = recv(m_Socket, (char*)&c, 1, MSG_PEEK);
	if (tRet == 0)
		return true;
	else if (tRet == SOCKET_ERROR)
	{
		if (errno == EWOULDBLOCK || errno == EAGAIN)
			return false;
		throw logic_error(StringFormat("@%s: %s", "recv", LASTWSAERR));
	}
	return false;
}
