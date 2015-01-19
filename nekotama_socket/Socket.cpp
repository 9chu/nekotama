// 先包含Windows.h以去除宏污染
#include <winsock2.h>
#undef max
#undef min

#include "Socket.h"

#include <atomic>
#include <algorithm>

using namespace std;
using namespace nekotama;

////////////////////////////////////////////////////////////////////////////////

static const char* GetErrorDesc(int id)NKNOEXCEPT
{
	switch (id)
	{
	case WSANOTINITIALISED:
		return "WSANOTINITIALISED: A successful WSAStartup call must occur before using this function.";
	case WSAENETDOWN:
		return "WSAENETDOWN: The network subsystem has failed.";
	case WSAEACCES:
		return "WSAEACCES: An attempt was made to access a socket in a way forbidden by its access permissions.";
	case WSAEADDRINUSE:
		return "WSAEADDRINUSE: Only one usage of each socket address(protocol / network address / port) is normally permitted.";
	case WSAEADDRNOTAVAIL:
		return "WSAEADDRNOTAVAIL: The requested address is not valid in its context.";
	case WSAEFAULT:
		return "WSAEFAULT: The system detected an invalid pointer address in attempting to use a pointer argument in a call.";
	case WSAEINPROGRESS:
		return "WSAEINPROGRESS: A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function.";
	case WSAEINVAL:
		return "WSAEINVAL: An invalid argument was supplied.";
	case WSAENOBUFS:
		return "WSAENOBUFS: An operation on a socket could not be performed because the system lacked sufficient buffer space or because a queue was full.";
	case WSAENOTSOCK:
		return "WSAENOTSOCK: An operation was attempted on something that is not a socket.";
	case WSASYSNOTREADY:
		return "WSASYSNOTREADY: The underlying network subsystem is not ready for network communication.";
	case WSAVERNOTSUPPORTED:
		return "WSAVERNOTSUPPORTED: The version of Windows Sockets support requested is not provided by this particular Windows Sockets implementation.";
	case WSAEPROCLIM:
		return "WSAEPROCLIM: A limit on the number of tasks supported by the Windows Sockets implementation has been reached.";
	case WSAEMFILE:
		return "WSAEMFILE: The queue is nonempty upon entry to accept and there are no descriptors available.";
	case WSAEOPNOTSUPP:
		return "WSAEOPNOTSUPP: The referenced socket is not a type that supports connection - oriented service.";
	case WSAEWOULDBLOCK:
		return "WSAEWOULDBLOCK: The socket is marked as nonblocking and no connections are present to be accepted.";
	case WSAEISCONN:
		return "WSAEISCONN: The socket is already connected.";
	case WSAECONNRESET:
		return "WSAECONNRESET: The virtual circuit was reset by the remote side executing a hard or abortive close.";
	case WSAETIMEDOUT:
		return "WSAETIMEDOUT: The connection has been dropped, because of a network failure or because the system on the other end went down without notice.";
	case WSAENETUNREACH:
		return "WSAENETUNREACH: The network cannot be reached from this host at this time.";
	case WSAEHOSTUNREACH:
		return "WSAEHOSTUNREACH: A socket operation was attempted to an unreachable host.";
	default:
		return "?: Unknown error.";
	}
}

#define LASTWSAERR GetErrorDesc(WSAGetLastError())

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
	WSADATA tData = { 0 };
	int tRet = WSAStartup(MAKEWORD(2, 2), &tData);
	if (tRet != 0)
		throw logic_error(StringFormat("@%s: %s", "WSAStartup", GetErrorDesc(tRet)));
}

SocketFactory::~SocketFactory()
{
	WSACleanup();
}

SocketHandle SocketFactory::Create(SocketType type)
{
	uint32_t tSocket;
	switch (type)
	{
	case SocketType::TCP:
		tSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		break;
	case SocketType::UDP:
		tSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
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
	fd_set tRead = { 0 }, tWrite = { 0 }, tError = { 0 };
	size_t tCount = 0;
	if (read)
	{
		if (read->size() >= FD_SETSIZE)
			throw logic_error("Select: socket handles are more than FD_SETSIZE.");
		else
			tCount = max(tCount, read->size());
		for (auto p : *read)
		{
			FD_SET(p->GetNativeHandle(), &tRead);
		}	
	}
	if (write)
	{
		if (write->size() >= FD_SETSIZE)
			throw logic_error("Select: socket handles are more than FD_SETSIZE.");
		else
			tCount = max(tCount, write->size());
		for (auto p : *write)
		{
			FD_SET(p->GetNativeHandle(), &tWrite);
		}	
	}
	if (error)
	{
		if (error->size() >= FD_SETSIZE)
			throw logic_error("Select: socket handles are more than FD_SETSIZE.");
		else
			tCount = max(tCount, error->size());
		for (auto p : *error)
		{
			FD_SET(p->GetNativeHandle(), &tError);
		}	
	}
	timeval t = { timeout / 1000, timeout % 1000 };
	if (tCount == 0)
		return false;
	int tRet = select(
		tCount,
		read->size() > 0 ? &tRead : nullptr,
		write->size() > 0 ? &tWrite : nullptr,
		error->size() > 0 ? &tError : nullptr,
		timeout == (uint32_t)-1 ? nullptr : &t
	);
	if (tRet == SOCKET_ERROR || tRet == 0)
	{
		if (read) read->clear();
		if (write) write->clear();
		if (error) error->clear();

		if (tRet == 0 || WSAGetLastError() == WSAEWOULDBLOCK)
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
	closesocket(m_Socket);
}

void Socket::IOControl(int32_t cmd, uint32_t* args)
{
	if (SOCKET_ERROR == ioctlsocket(m_Socket, cmd, (u_long*)args))
		throw logic_error(StringFormat("@%s: %s", "ioctlsocket", LASTWSAERR));
}

void Socket::SetBlockingMode(bool blocking)
{
	uint32_t v = blocking ? 0 : 1;
	if (SOCKET_ERROR == ioctlsocket(m_Socket, FIONBIO, (u_long*)&v))
		throw logic_error(StringFormat("@%s: %s", "ioctlsocket", LASTWSAERR));
}

void Socket::Bind(const char* addr, uint16_t port)
{
	sockaddr_in tAddr;
	tAddr.sin_family = AF_INET;
	tAddr.sin_port = htons(port);
	tAddr.sin_addr.S_un.S_addr = inet_addr(addr);
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
	int size = sizeof(sockaddr_in);
	SOCKET tRet = accept(m_Socket, (sockaddr*)&tAddr, &size);
	if (tRet == INVALID_SOCKET)
	{
		ip.clear();
		port = 0;
		handle = nullptr;
		if (WSAGetLastError() == WSAEWOULDBLOCK)
			return false;
		throw logic_error(StringFormat("@%s: %s", "accept", LASTWSAERR));
	}
	if (size != sizeof(sockaddr_in))
	{
		closesocket(tRet);
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
		tRet = shutdown(m_Socket, SD_RECEIVE);
		break;
	case SocketShutdownType::CloseWrite:
		tRet = shutdown(m_Socket, SD_SEND);
		break;
	case SocketShutdownType::CloseReadWrite:
		tRet = shutdown(m_Socket, SD_BOTH);
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
	tAddr.sin_addr.S_un.S_addr = inet_addr(addr);
	int tRet = connect(m_Socket, (sockaddr*)&tAddr, sizeof(tAddr));
	if (tRet == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
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
		if (WSAGetLastError() == WSAEWOULDBLOCK)
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
		if (WSAGetLastError() == WSAEWOULDBLOCK)
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
		if (WSAGetLastError() == WSAEWOULDBLOCK)
			return false;
		throw logic_error(StringFormat("@%s: %s", "recv", LASTWSAERR));
	}
	return false;
}
