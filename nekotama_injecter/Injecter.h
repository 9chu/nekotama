#pragma once
#include <string>
#include <Windows.h>

namespace nekotama
{
	class Injecter
	{
	private:
		HANDLE   m_hRemoteProcess;
		HANDLE   m_hThread;
		wchar_t* m_lpDllPath;
		UINT     m_cDllPathSize;
	public:
		/// @brief 注入一个进程
		/// @param ProcessId 进程ID
		/// @param DllPath   要注入的Dll路径（绝对路径）
		Injecter(DWORD ProcessId, LPCWSTR DllPath);
		~Injecter(void);
	};
}
