#pragma once
#include <functional>

#include <d3d9.h>

#include "APIHooker.h"

namespace nekotama
{
	// ԭʼ��������
	IDirect3D9* WINAPI Dx9_Direct3DCreate9(UINT SDKVersion);

	// HOOK�ҹ�����
	void Dx9Hook(APIHooker& target);
	void SetCallback_Direct3DCreate9(const std::function<IDirect3D9*(UINT)>& Callback);
}
