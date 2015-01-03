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
		/// @brief ע��һ������
		/// @param ProcessId ����ID
		/// @param DllPath   Ҫע���Dll·��������·����
		Injecter(DWORD ProcessId, LPCWSTR DllPath);
		~Injecter(void);
	};
}
