#include "ComHooker.h"
#include "HookUtil.h"
#include "SocketHook.h"
#include "Dx9Hooker.h"
#include "ClientImplement.h"
#include "VirtualUDPConnection.h"

#include <cstdint>
#include <sstream>
#include <memory>

using namespace std;
using namespace nekotama;

static std::wstring g_DllWorkPath;
static std::shared_ptr<ClientRenderer> g_ClientRenderer;
static std::shared_ptr<ClientImplement> g_ClientImplement;

// ===== 窗口消息 Hook部分 =====
static LRESULT(CALLBACK *raw_WindowProc)(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK hook_WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_KEYDOWN)
	{
		if (wParam == VK_TAB && g_ClientImplement && g_ClientRenderer &&
			g_ClientRenderer->IsGameInfoListVisible() == false)
		{
			g_ClientImplement->QueryGameInfo();
			g_ClientRenderer->ShowGameInfoList();
		}
	}
	else if (uMsg == WM_KEYUP)
	{
		if (wParam == VK_TAB && g_ClientImplement && g_ClientRenderer &&
			g_ClientRenderer->IsGameInfoListVisible() == true)
			g_ClientRenderer->HideGameInfoList();
	}
	else if (uMsg == WM_CLOSE || uMsg == WM_DESTROY || (uMsg == WM_SHOWWINDOW && wParam == FALSE))
	{
		if (g_ClientImplement)  // 释放客户端实现部分
		{
			g_ClientImplement->GentlyStop();
			g_ClientImplement->Wait();
		}
		g_ClientRenderer = nullptr;  // 在这里释放客户端界面
		g_ClientImplement = nullptr;
	}
	
	return CallWindowProc((WNDPROC)raw_WindowProc, hwnd, uMsg, wParam, lParam);
};

// ===== Direct3D Hook部分 =====
static HRESULT(WINAPI *raw_IDirect3DSwapChain9_Present)(IDirect3DSwapChain9* pThis, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion, DWORD dwFlags);
static HRESULT(WINAPI *raw_IDirect3DDevice9_Reset)(IDirect3DDevice9* pThis, D3DPRESENT_PARAMETERS* pPresentationParameters);
static HRESULT(WINAPI *raw_IDirect3DDevice9_GetSwapChain)(IDirect3DDevice9* pThis, UINT iSwapChain, IDirect3DSwapChain9** pSwapChain);
static HRESULT(WINAPI *raw_IDirect3DDevice9_Present)(IDirect3DDevice9* pThis, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion);
static HRESULT(WINAPI *raw_IDirect3D9_CreateDevice)(IDirect3D9* pThis, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice9** ppReturnedDeviceInterface);

static HRESULT WINAPI hook_IDirect3DDevice9_Reset(IDirect3DDevice9* pThis, D3DPRESENT_PARAMETERS* pPresentationParameters)
{
	if (g_ClientRenderer)
		g_ClientRenderer->DoDeviceLost();

	// 执行原始函数
	HRESULT tRet = raw_IDirect3DDevice9_Reset(pThis, pPresentationParameters);

	if (SUCCEEDED(tRet))
	{
		if (g_ClientRenderer)
			g_ClientRenderer->DoDeviceReset();
	}	
	return tRet;
}

static HRESULT WINAPI hook_IDirect3DSwapChain9_Present(IDirect3DSwapChain9* pThis, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion, DWORD dwFlags)
{
	// 绘制客户端界面
	if (g_ClientRenderer)
		g_ClientRenderer->Render();

	// 执行原始函数
	HRESULT tRet = raw_IDirect3DSwapChain9_Present(pThis, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags);

	// 检测虚表改动
	IDirect3DDevice9* pDev;
	pThis->GetDevice(&pDev);
	if (ComHooker::GetFuncPtr(pDev, 16) != hook_IDirect3DDevice9_Reset)
		*(void**)&raw_IDirect3DDevice9_Reset = ComHooker::HookVptr(pDev, 16, hook_IDirect3DDevice9_Reset);  // IDirect3DDevice9::Reset
	pDev->Release();

	if (tRet == D3DERR_DEVICELOST)
	{
		if (g_ClientRenderer)
			g_ClientRenderer->DoDeviceLost();
	}	
	return tRet;
}

static HRESULT WINAPI hook_IDirect3DDevice9_GetSwapChain(IDirect3DDevice9* pThis, UINT iSwapChain, IDirect3DSwapChain9** pSwapChain)
{
	HRESULT tRet = raw_IDirect3DDevice9_GetSwapChain(pThis, iSwapChain, pSwapChain);
	if (SUCCEEDED(tRet))
	{
		// hook函数
		if (ComHooker::GetFuncPtr(*pSwapChain, 3) != hook_IDirect3DSwapChain9_Present)
			*(void**)&raw_IDirect3DSwapChain9_Present = ComHooker::HookVptr(*pSwapChain, 3, hook_IDirect3DSwapChain9_Present);  // IDirect3DSwapChain9::Present
	}
	return tRet;
}

static HRESULT WINAPI hook_IDirect3DDevice9_Present(IDirect3DDevice9* pThis, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion)
{
	// 绘制客户端界面
	if (g_ClientRenderer)
		g_ClientRenderer->Render();

	// 执行原始函数
	HRESULT tRet = raw_IDirect3DDevice9_Present(pThis, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);

	if (tRet == D3DERR_DEVICELOST)
	{
		if (g_ClientRenderer)
			g_ClientRenderer->DoDeviceLost();
	}

	// 检查虚表改动
	if (ComHooker::GetFuncPtr(pThis, 17) != hook_IDirect3DDevice9_Present)
		*(void**)&raw_IDirect3DDevice9_Present = ComHooker::HookVptr(pThis, 17, hook_IDirect3DDevice9_Present);  // IDirect3DDevice9::Present
	if (ComHooker::GetFuncPtr(pThis, 16) != hook_IDirect3DDevice9_Reset)
		*(void**)&raw_IDirect3DDevice9_Reset = ComHooker::HookVptr(pThis, 16, hook_IDirect3DDevice9_Reset);  // IDirect3DDevice9::Reset

	return tRet;
}

static HRESULT WINAPI hook_IDirect3D9_CreateDevice(IDirect3D9* pThis, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice9** ppReturnedDeviceInterface)
{
	HRESULT tResult = raw_IDirect3D9_CreateDevice(pThis, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);
	if (SUCCEEDED(tResult))
	{
		if (!g_ClientRenderer)
		{
			// Hook相应函数
			*(void**)&raw_IDirect3DDevice9_Present = ComHooker::HookVptr(*ppReturnedDeviceInterface, 17, hook_IDirect3DDevice9_Present);  // IDirect3DDevice9::Present
			*(void**)&raw_IDirect3DDevice9_GetSwapChain = ComHooker::HookVptr(*ppReturnedDeviceInterface, 14, hook_IDirect3DDevice9_GetSwapChain);  // IDirect3DDevice9::GetSwapChain
			*(void**)&raw_IDirect3DDevice9_Reset = ComHooker::HookVptr(*ppReturnedDeviceInterface, 16, hook_IDirect3DDevice9_Reset);  // IDirect3DDevice9::Reset

			// 构造初始化客户端界面渲染器
			g_ClientRenderer = make_shared<ClientRenderer>(g_DllWorkPath + L"\\assets", *ppReturnedDeviceInterface);

			// 构造客户端
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
			
			// hook窗口函数
			*(LONG*)&raw_WindowProc = SetWindowLong(hFocusWindow, GWL_WNDPROC, (LONG)hook_WindowProc);
		}
	}
	return tResult;
}

// ===== DLL入口 =====
BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		{
			APIHooker tMainModule;
			
			// 获取DLL执行路径
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
			SetCallback_Socket([](int af, int type, int protocol, std::shared_ptr<ISocketHooker>& pListener) -> SOCKET {
				if (af == AF_INET && type == SOCK_DGRAM && (protocol == 0 || protocol == IPPROTO_UDP) && g_ClientImplement)
				{
					pListener = make_shared<VirtualUDPConnection>(g_ClientImplement);
					return (SOCKET)pListener.get();
				}	
				else
					return Socket_Socket(af, type, protocol);
			});

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
