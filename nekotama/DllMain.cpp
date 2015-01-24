#include "ComHooker.h"
#include "HookUtil.h"
#include "SocketHook.h"
#include "Dx9Hooker.h"
#include "ClientImplement.h"

#include <cstdint>
#include <sstream>
#include <memory>

using namespace std;
using namespace nekotama;

static std::wstring g_DllWorkPath;
static std::shared_ptr<ClientRenderer> g_ClientRenderer;
static std::shared_ptr<ClientImplement> g_ClientImplement;

// ===== ������Ϣ Hook���� =====
static LRESULT(CALLBACK *raw_WindowProc)(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK hook_WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_DESTROY)
	{
		if (g_ClientImplement)  // �ͷſͻ���ʵ�ֲ���
		{
			g_ClientImplement->GentlyStop();
			g_ClientImplement->Wait();
		}
		g_ClientRenderer = nullptr;  // �������ͷſͻ��˽���
		g_ClientImplement = nullptr;
	}	
	return CallWindowProc((WNDPROC)raw_WindowProc, hwnd, uMsg, wParam, lParam);
};

// ===== Direct3D Hook���� =====
static HRESULT(WINAPI *raw_IDirect3DDevice9_Present)(IDirect3DDevice9* pThis, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion);
static HRESULT(WINAPI *raw_IDirect3D9_CreateDevice)(IDirect3D9* pThis, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice9** ppReturnedDeviceInterface);

static HRESULT WINAPI hook_IDirect3DDevice9_Present(IDirect3DDevice9* pThis, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion)
{
	// ���ƿͻ��˽���
	if (g_ClientRenderer)
		g_ClientRenderer->Render();

	// ִ��ԭʼ����
	HRESULT tResult = raw_IDirect3DDevice9_Present(pThis, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);

	// ������Ķ�
	if (ComHooker::GetFuncPtr(pThis, 17) != hook_IDirect3DDevice9_Present)
		*(void**)&raw_IDirect3DDevice9_Present = ComHooker::HookVptr(pThis, 17, hook_IDirect3DDevice9_Present);

	return tResult;
}

static HRESULT WINAPI hook_IDirect3D9_CreateDevice(IDirect3D9* pThis, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice9** ppReturnedDeviceInterface)
{
	HRESULT tResult = raw_IDirect3D9_CreateDevice(pThis, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);
	if (SUCCEEDED(tResult))
	{
		if (!g_ClientRenderer)
		{
			// Hook��Ӧ����
			*(void**)&raw_IDirect3DDevice9_Present = ComHooker::HookVptr(*ppReturnedDeviceInterface, 17, hook_IDirect3DDevice9_Present);  // IDirect3DDevice9::Present

			// �����ʼ���ͻ��˽�����Ⱦ��
			g_ClientRenderer = make_shared<ClientRenderer>(g_DllWorkPath + L"\\assets", *ppReturnedDeviceInterface);

			// ����ͻ���
			// !TODO
			try
			{
				g_ClientImplement = make_shared<ClientImplement>(g_ClientRenderer, "127.0.0.1", "chu");
				g_ClientImplement->Start();
			}
			catch (...)
			{
				g_ClientImplement = nullptr;
			}
			
			// hook���ں���
			*(LONG*)&raw_WindowProc = SetWindowLong(hFocusWindow, GWL_WNDPROC, (LONG)hook_WindowProc);
		}
	}
	return tResult;
}

// ===== Socket Hook���� =====


BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		{
			APIHooker tMainModule;
			
			// ��ȡDLLִ��·��
			int i = 0;
			wchar_t tDllPath[MAX_PATH + 1];
			GetModuleFileName((HMODULE)hModule, tDllPath, MAX_PATH);
			while (tDllPath[i] != '\0') { ++i; }
			while (tDllPath[i] != '\\') { --i; }
			tDllPath[i] = '\0';
			g_DllWorkPath = tDllPath;

			// Direct3D Hook
			Dx9Hook(tMainModule);
			SetCallback_Direct3DCreate9([](UINT SDKVersion) -> IDirect3D9* {
				IDirect3D9* ret = Dx9_Direct3DCreate9(SDKVersion);
				if (raw_IDirect3D9_CreateDevice == nullptr)
					*(void**)&raw_IDirect3D9_CreateDevice = ComHooker::HookVptr(ret, 16, hook_IDirect3D9_CreateDevice);  // IDirect3D9::CreateDevice
				return ret;
			});

			// Socket Hook
			SocketHook(tMainModule);
			// !TODO

			// �ָ����߳�
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


/*
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
		sprintf(buf, "[Socket:%x] Bind �� %s:%u namelen=%d\n", s, ip.c_str(), port, namelen);
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
			sprintf(pbuf, "[Socket:%x] RecvFrom �� %s:%u len=%d fromlen=%d\n", s, ip.c_str(), port, len, datalen);
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
		sprintf(pbuf, "[Socket:%x] SendTo �� %s:%u len=%d tolen=%d\n", s, ip.c_str(), port, len, tolen);
		OutDbg(pbuf);
		return ISocketHooker::SendTo(s, buf, len, flags, to, tolen);
	}
};

TestListener g_test;

std::shared_ptr<HookRenderer> g_renderer;

static HRESULT(WINAPI *raw_Present)(IDirect3DDevice9* pThis, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion);

static HRESULT(WINAPI *raw_CreateDevice)(IDirect3D9* pThis, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice9** ppReturnedDeviceInterface);



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

			// DXHook
			Dx9Hook(mainModule);
			SetCallback_Direct3DCreate9([](UINT SDKVersion) -> IDirect3D9* {
				IDirect3D9* ret = Dx9_Direct3DCreate9(SDKVersion);
				OutDbg("Direct3DCreate9 called.\n");
				OutDbg("Hooking IDirect3D9::CreateDevice...\n");
				*(void**)&raw_CreateDevice = ComHooker::HookVptr(ret, 16, hook_CreateDevice);  // IDirect3D9::CreateDevice
				return ret;
			});

			// ��������̨
			AllocConsole();
			g_ConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
						
			// �ָ����߳�
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
*/
