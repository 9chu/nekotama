#include "HookUtil.h"

#include <Tlhelp32.h>

using namespace nekotama;

DWORD nekotama::GetMainThread()
{
	DWORD dwThreadID = 0;
	HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	DWORD dwProcessID = GetCurrentProcessId();
	THREADENTRY32 te32 = { sizeof(te32) };
	if (Thread32First(hThreadSnap, &te32))
	{
		do
		{
			if (dwProcessID == te32.th32OwnerProcessID)
			{
				dwThreadID = te32.th32ThreadID;
				break;
			}
		} while (Thread32Next(hThreadSnap, &te32));
	}
	return dwThreadID;
}

void nekotama::ResumeMainThread()
{
	DWORD hThreadID = GetMainThread();
	HANDLE hHandle = OpenThread(THREAD_ALL_ACCESS, FALSE, hThreadID);
	ResumeThread(hHandle);
	CloseHandle(hHandle);
}
