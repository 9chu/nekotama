#include "Injecter.h"

using namespace nekotama;

Injecter::Injecter(DWORD ProcessId, LPCWSTR DllPath)
{
	// 打开进程
	m_hRemoteProcess = OpenProcess( 
		PROCESS_CREATE_THREAD |        // 允许远程创建线程
		PROCESS_QUERY_INFORMATION |
		PROCESS_VM_OPERATION  |        // 允许远程VM操作
		PROCESS_VM_READ       |
		PROCESS_VM_WRITE,		       // 允许远程VM写
		FALSE, ProcessId);

	m_lpDllPath = NULL;
	// 在远端写入DLL地址
	{
		// 计算DLL路径名需要的内存空间
		m_cDllPathSize = (1 + wcslen(DllPath)) * sizeof(wchar_t);
		// 使用VirtualAllocEx函数在远程进程的内存地址空间分配DLL文件名缓冲区
		m_lpDllPath = (wchar_t*)VirtualAllocEx( m_hRemoteProcess, NULL, m_cDllPathSize, MEM_COMMIT, PAGE_READWRITE);
		// 使用WriteProcessMemory函数将DLL的路径名复制到远程进程的内存空间
		WriteProcessMemory(m_hRemoteProcess, m_lpDllPath, (PVOID) DllPath, m_cDllPathSize, NULL); 
	}

	// 获得LoadLibraryW的地址
	PTHREAD_START_ROUTINE pfnStartAddr = (PTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle(L"Kernel32"), "LoadLibraryW");
	// 创建远端线程
	m_hThread = CreateRemoteThread(m_hRemoteProcess, NULL, 0, pfnStartAddr, m_lpDllPath, 0, NULL);
}

Injecter::~Injecter(void)
{
	VirtualFreeEx(m_hRemoteProcess, m_lpDllPath, m_cDllPathSize, MEM_DECOMMIT);
	CloseHandle(m_hThread);
	CloseHandle(m_hRemoteProcess);
}
