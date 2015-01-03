#pragma once
#include <functional>
#include "APIHooker.h"

namespace nekotama
{
	// 原始函数调用
	int WINAPI Socket_Bind(SOCKET s, const struct sockaddr* addr, int namelen);
	int WINAPI Socket_CloseSocket(SOCKET s);
	int WINAPI Socket_RecvFrom(SOCKET s, char* buf, int len, int flags, struct sockaddr* from, int* fromlen);
	int WINAPI Socket_SendTo(SOCKET s, const char* buf, int len, int flags, const struct sockaddr* to, int tolen);
	int WINAPI Socket_Shutdown(SOCKET s, int how);
	SOCKET WINAPI Socket_Socket(int af, int type, int protocol);
	struct hostent* WINAPI Socket_GetHostByName(const char* name);

	// 调用Hook接口
	struct ISocketHooker
	{
		virtual int Bind(SOCKET s, const struct sockaddr* addr, int namelen) { return Socket_Bind(s, addr, namelen); }
		virtual int CloseSocket(SOCKET s) { return Socket_CloseSocket(s); }
		virtual int RecvFrom(SOCKET s, char* buf, int len, int flags, struct sockaddr* from, int* fromlen) { return Socket_RecvFrom(s, buf, len, flags, from, fromlen); }
		virtual int SendTo(SOCKET s, const char* buf, int len, int flags, const struct sockaddr* to, int tolen) { return Socket_SendTo(s, buf, len, flags, to, tolen); }
		virtual int Shutdown(SOCKET s, int how) { return Socket_Shutdown(s, how); }
	};

	// HOOK挂钩函数
	void SocketHook(APIHooker& target);
	void SetCallback_Socket(const std::function<SOCKET(int, int, int, ISocketHooker*&)>& Callback);
	void SetCallback_GetHostByName(const std::function<struct hostent*(const char*)>& Callback);
}
