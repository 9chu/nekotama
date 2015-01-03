#define _CRT_SECURE_NO_WARNINGS

#include "APIHooker.h"
#include "HookUtil.h"
#include "SocketHook.h"

#include <cstdint>
#include <sstream>

using namespace std;
using namespace nekotama;

HANDLE g_ConsoleOutput;

void OutDbg(const std::string& str)
{
	WriteConsoleA(g_ConsoleOutput, str.c_str(), str.length(), NULL, NULL);
}

class TestListener :
	public ISocketHooker
{
public:
	int Bind(SOCKET s, const struct sockaddr* addr, int namelen)
	{
		string ip = inet_ntoa(((sockaddr_in*)addr)->sin_addr);
		int16_t port = ntohs(((sockaddr_in*)addr)->sin_port);

		char buf[512];
		sprintf(buf, "[Socket:%x] Bind 于 %s:%u namelen=%d\n", s, ip.c_str(), port, namelen);
		OutDbg(buf);
		return ISocketHooker::Bind(s, addr, namelen);
	}
	int RecvFrom(SOCKET s, char* buf, int len, int flags, struct sockaddr* from, int* fromlen)
	{
		string ip = inet_ntoa(((sockaddr_in*)from)->sin_addr);
		uint16_t port = ntohs(((sockaddr_in*)from)->sin_port);

		int datalen = fromlen ? *fromlen : 0;
		int ret = ISocketHooker::RecvFrom(s, buf, len, flags, from, &datalen);
		stringstream ss;
		if (datalen > 0)
		{
			char pbuf[512];
			sprintf(pbuf, "[Socket:%x] RecvFrom 于 %s:%u len=%d fromlen=%d\n", s, ip.c_str(), port, len, datalen);
			OutDbg(pbuf);
		}
		if (fromlen)
			*fromlen = datalen;
		return ret;
	}
	int SendTo(SOCKET s, const char* buf, int len, int flags, const struct sockaddr* to, int tolen)
	{
		string ip = inet_ntoa(((sockaddr_in*)to)->sin_addr);
		uint16_t port = ntohs(((sockaddr_in*)to)->sin_port);

		char pbuf[512];
		sprintf(pbuf, "[Socket:%x] SendTo 于 %s:%u len=%d tolen=%d\n", s, ip.c_str(), port, len, tolen);
		OutDbg(pbuf);
		return ISocketHooker::SendTo(s, buf, len, flags, to, tolen);
	}
};

TestListener g_test;

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		{
			APIHooker mainModule;
			SocketHook(mainModule);
			SetCallback_Socket([](int af, int type, int protocol , ISocketHooker*& listener) -> SOCKET {
				listener = &g_test;
				return Socket_Socket(af, type, protocol);
			});

			// 创建控制台
			AllocConsole();
			g_ConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
						
			// 恢复主线程
			ResumeMainThread();
		}
		break;
	case DLL_PROCESS_DETACH:
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	}
	return (TRUE);
}
