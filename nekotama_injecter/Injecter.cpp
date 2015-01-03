#include "Injecter.h"

using namespace nekotama;

Injecter::Injecter(DWORD ProcessId, LPCWSTR DllPath)
{
	// �򿪽���
	m_hRemoteProcess = OpenProcess( 
		PROCESS_CREATE_THREAD |        // ����Զ�̴����߳�
		PROCESS_QUERY_INFORMATION |
		PROCESS_VM_OPERATION  |        // ����Զ��VM����
		PROCESS_VM_READ       |
		PROCESS_VM_WRITE,		       // ����Զ��VMд
		FALSE, ProcessId);

	m_lpDllPath = NULL;
	// ��Զ��д��DLL��ַ
	{
		// ����DLL·������Ҫ���ڴ�ռ�
		m_cDllPathSize = (1 + wcslen(DllPath)) * sizeof(wchar_t);
		// ʹ��VirtualAllocEx������Զ�̽��̵��ڴ��ַ�ռ����DLL�ļ���������
		m_lpDllPath = (wchar_t*)VirtualAllocEx( m_hRemoteProcess, NULL, m_cDllPathSize, MEM_COMMIT, PAGE_READWRITE);
		// ʹ��WriteProcessMemory������DLL��·�������Ƶ�Զ�̽��̵��ڴ�ռ�
		WriteProcessMemory(m_hRemoteProcess, m_lpDllPath, (PVOID) DllPath, m_cDllPathSize, NULL); 
	}

	// ���LoadLibraryW�ĵ�ַ
	PTHREAD_START_ROUTINE pfnStartAddr = (PTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle(L"Kernel32"), "LoadLibraryW");
	// ����Զ���߳�
	m_hThread = CreateRemoteThread(m_hRemoteProcess, NULL, 0, pfnStartAddr, m_lpDllPath, 0, NULL);
}

Injecter::~Injecter(void)
{
	VirtualFreeEx(m_hRemoteProcess, m_lpDllPath, m_cDllPathSize, MEM_DECOMMIT);
	CloseHandle(m_hThread);
	CloseHandle(m_hRemoteProcess);
}
