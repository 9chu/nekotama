#include "SocketHook.h"
#include <unordered_map>
#include <memory>

using namespace nekotama;

static int(WINAPI *raw_bind)(SOCKET, const struct sockaddr*, int) = NULL;
static int(WINAPI *raw_closesocket)(SOCKET) = NULL;
static int(WINAPI *raw_recvfrom)(SOCKET, char*, int, int, struct sockaddr*, int*) = NULL;
static int(WINAPI *raw_sendto)(SOCKET, const char*, int, int, const struct sockaddr*, int) = NULL;
static int(WINAPI *raw_shutdown)(SOCKET, int) = NULL;
static SOCKET(WINAPI *raw_socket)(int, int, int) = NULL;
static struct hostent*(WINAPI *raw_gethostbyname)(const char*) = NULL;

static int WINAPI hook_bind(SOCKET s, const struct sockaddr* addr, int namelen);
static int WINAPI hook_closesocket(SOCKET s);
static int WINAPI hook_recvfrom(SOCKET s, char* buf, int len, int flags, struct sockaddr* from, int* fromlen);
static int WINAPI hook_sendto(SOCKET s, const char* buf, int len, int flags, const struct sockaddr* to, int tolen);
static int WINAPI hook_shutdown(SOCKET s, int how);
static SOCKET WINAPI hook_socket(int af, int type, int protocol);
static struct hostent* WINAPI hook_gethostbyname(const char* name);

int WINAPI nekotama::Socket_Bind(SOCKET s, const struct sockaddr* addr, int namelen)
{
	if (raw_bind)
		return raw_bind(s, addr, namelen);
	else
		return bind(s, addr, namelen);
}

int WINAPI nekotama::Socket_CloseSocket(SOCKET s)
{
	if (raw_closesocket)
		return raw_closesocket(s);
	else
		return closesocket(s);
}

int WINAPI nekotama::Socket_RecvFrom(SOCKET s, char* buf, int len, int flags, struct sockaddr* from, int* fromlen)
{
	if (raw_recvfrom)
		return raw_recvfrom(s, buf, len, flags, from, fromlen);
	else
		return recvfrom(s, buf, len, flags, from, fromlen);
}

int WINAPI nekotama::Socket_SendTo(SOCKET s, const char* buf, int len, int flags, const struct sockaddr* to, int tolen)
{
	if (raw_sendto)
		return raw_sendto(s, buf, len, flags, to, tolen);
	else
		return sendto(s, buf, len, flags, to, tolen);
}

int WINAPI nekotama::Socket_Shutdown(SOCKET s, int how)
{
	if (raw_shutdown)
		return raw_shutdown(s, how);
	else
		return shutdown(s, how);
}

SOCKET WINAPI nekotama::Socket_Socket(int af, int type, int protocol)
{
	if (raw_socket)
		return raw_socket(af, type, protocol);
	else
		return socket(af, type, protocol);
}

struct hostent* WINAPI nekotama::Socket_GetHostByName(const char* name)
{
	if (raw_gethostbyname)
		return gethostbyname(name);
	else
		return gethostbyname(name);
}

////////////////////////////////////////////////////////////////////////////////

struct HookTable
{
	const char* LibName;
	const char* FuncName;
	PROC Target;
	PROC* RawFunc;
};

static std::unordered_map<SOCKET, std::shared_ptr<ISocketHooker>> callbackTable;
static CRITICAL_SECTION lockSection;
static bool socketHooked = false;
static std::function<SOCKET(int, int, int, std::shared_ptr<ISocketHooker>&)> callback_socket;
static std::function<struct hostent*(const char*)> callback_gethostbyname;

static HookTable socketHookTable[] = {
	{ "WS2_32.dll", "bind", (PROC)hook_bind, (PROC*)&raw_bind },
	{ "WS2_32.dll", "closesocket", (PROC)hook_closesocket, (PROC*)&raw_closesocket },
	{ "WS2_32.dll", "recvfrom", (PROC)hook_recvfrom, (PROC*)&raw_recvfrom },
	{ "WS2_32.dll", "sendto", (PROC)hook_sendto, (PROC*)&raw_sendto },
	{ "WS2_32.dll", "shutdown", (PROC)hook_shutdown, (PROC*)&raw_shutdown },
	{ "WS2_32.dll", "socket", (PROC)hook_socket, (PROC*)&raw_socket },
	{ "WS2_32.dll", "gethostbyname", (PROC)hook_gethostbyname, (PROC*)&raw_gethostbyname }
};

static ISocketHooker* GetHooker(SOCKET s)
{
	EnterCriticalSection(&lockSection);
	auto i = callbackTable.find(s);
	if (i == callbackTable.end())
	{
		LeaveCriticalSection(&lockSection);
		return NULL;
	}
	else
	{
		ISocketHooker* p = i->second.get();
		LeaveCriticalSection(&lockSection);
		return p;
	}
}

static void RemoveHooker(SOCKET s)
{
	EnterCriticalSection(&lockSection);
	auto i = callbackTable.find(s);
	if (i == callbackTable.end())
		LeaveCriticalSection(&lockSection);
	else
	{
		callbackTable.erase(i);
		LeaveCriticalSection(&lockSection);
	}
}

static void SetHooker(SOCKET s, const std::shared_ptr<ISocketHooker>& pHooker)
{
	EnterCriticalSection(&lockSection);
	callbackTable[s] = pHooker;
	LeaveCriticalSection(&lockSection);
}

void nekotama::SocketHook(APIHooker& target)
{
	if (!socketHooked)
	{
		socketHooked = true;
		for (HookTable t : socketHookTable)
		{
			*t.RawFunc = target.HookFunction(t.LibName, t.FuncName, t.Target);
		}
		InitializeCriticalSection(&lockSection);
	}
}

void nekotama::SetCallback_Socket(const std::function<SOCKET(int af, int type, int protocol, std::shared_ptr<ISocketHooker>& listener)>& Callback)
{
	callback_socket = Callback;
}

void nekotama::SetCallback_GetHostByName(const std::function<struct hostent*(const char* name)>& Callback)
{
	callback_gethostbyname = Callback;
}

////////////////////////////////////////////////////////////////////////////////

static int WINAPI hook_bind(SOCKET s, const struct sockaddr* addr, int namelen)
{
	ISocketHooker* p = GetHooker(s);
	if (p)
		return p->Bind(s, addr, namelen);
	else
		return Socket_Bind(s, addr, namelen);
}

static int WINAPI hook_closesocket(SOCKET s)
{
	ISocketHooker* p = GetHooker(s);
	if (p)
	{
		int ret = p->CloseSocket(s);
		RemoveHooker(s);
		return ret;
	}	
	else
		return Socket_CloseSocket(s);
}

static int WINAPI hook_recvfrom(SOCKET s, char* buf, int len, int flags, struct sockaddr* from, int* fromlen)
{
	ISocketHooker* p = GetHooker(s);
	if (p)
		return p->RecvFrom(s, buf, len, flags, from, fromlen);
	else
		return Socket_RecvFrom(s, buf, len, flags, from, fromlen);
}

static int WINAPI hook_sendto(SOCKET s, const char* buf, int len, int flags, const struct sockaddr* to, int tolen)
{
	ISocketHooker* p = GetHooker(s);
	if (p)
		return p->SendTo(s, buf, len, flags, to, tolen);
	else
		return Socket_SendTo(s, buf, len, flags, to, tolen);
}

static int WINAPI hook_shutdown(SOCKET s, int how)
{
	ISocketHooker* p = GetHooker(s);
	if (p)
		return p->Shutdown(s, how);
	else
		return Socket_Shutdown(s, how);
}

static SOCKET WINAPI hook_socket(int af, int type, int protocol)
{
	if (callback_socket)
	{
		std::shared_ptr<ISocketHooker> p = NULL;
		SOCKET ret = callback_socket(af, type, protocol, p);
		if (p)
			SetHooker(ret, p);
		return ret;
	}
	else
		return Socket_Socket(af, type, protocol);
}

static struct hostent* WINAPI hook_gethostbyname(const char* name)
{
	if (callback_gethostbyname)
		return callback_gethostbyname(name);
	else
		return Socket_GetHostByName(name);
}
