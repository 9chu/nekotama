#include "Dx9Hooker.h"

using namespace nekotama;

////////////////////////////////////////////////////////////////////////////////

static IDirect3D9* (WINAPI *raw_Direct3DCreate9)(UINT SDKVersion) = NULL;

static IDirect3D9* WINAPI hook_Direct3DCreate9(UINT SDKVersion);

IDirect3D9* WINAPI nekotama::Dx9_Direct3DCreate9(UINT SDKVersion)
{
	if (raw_Direct3DCreate9 == NULL)
		return Direct3DCreate9(SDKVersion);
	else
		return raw_Direct3DCreate9(SDKVersion);
}

////////////////////////////////////////////////////////////////////////////////

static bool dx9Hooked = false;
static std::function<IDirect3D9*(UINT)> callback_create;

void nekotama::Dx9Hook(APIHooker& target)
{
	if (!dx9Hooked)
	{
		dx9Hooked = true;
		*(PROC*)&raw_Direct3DCreate9 = target.HookFunction("D3D9.dll", "Direct3DCreate9", (PROC)hook_Direct3DCreate9);
	}
}

void nekotama::SetCallback_Direct3DCreate9(const std::function<IDirect3D9*(UINT)>& Callback)
{
	callback_create = Callback;
}

////////////////////////////////////////////////////////////////////////////////

static IDirect3D9* WINAPI hook_Direct3DCreate9(UINT SDKVersion)
{
	if (callback_create)
		return callback_create(SDKVersion);
	else
		return raw_Direct3DCreate9(SDKVersion);
}
