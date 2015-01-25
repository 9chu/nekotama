#include "Process.h"

#include <stdexcept>

#include <StringFormat.h>

using namespace std;
using namespace nekotama;

std::wstring Process::GetGurrentDirectory()
{
	wchar_t buf[MAX_PATH + 1] = { 0 };
	GetCurrentDirectory(MAX_PATH, buf);
	return buf;
}

Process::Process(LPCWSTR Path, LPCWSTR CommandLine, LPCWSTR Environment)
{
	STARTUPINFO tStartupInfo;
	memset(&tStartupInfo,0,sizeof(tStartupInfo));
	tStartupInfo.cb = sizeof(tStartupInfo);
	tStartupInfo.dwFlags = STARTF_USESHOWWINDOW;
	tStartupInfo.wShowWindow = SW_SHOWNORMAL;

	if(!CreateProcess(Path, (LPWSTR)CommandLine, NULL, NULL, FALSE, CREATE_SUSPENDED|CREATE_NEW_CONSOLE|CREATE_UNICODE_ENVIRONMENT|CREATE_DEFAULT_ERROR_MODE, NULL, Environment, &tStartupInfo, &m_Info))
	{
		UINT tRet = GetLastError();
		throw logic_error(StringFormat("CreateProcess failed. (LastError=%d)", tRet));
	}
}

Process::~Process(void)
{
	CloseHandle(m_Info.hProcess);
	CloseHandle(m_Info.hThread);
}

DWORD Process::GetProcessId()
{
	return m_Info.dwProcessId;
}

void Process::Resume()
{
	ResumeThread(m_Info.hThread);
}

void Process::Suspend()
{
	SuspendThread(m_Info.hThread);
}

void Process::Terminate()
{
	TerminateProcess(m_Info.hProcess, (UINT)-1);
}

UINT Process::GetExitCode()
{
	DWORD tRet = (DWORD)-1;
	GetExitCodeProcess(m_Info.hProcess, &tRet);
	return tRet;
}

void Process::Wait()
{
	WaitForSingleObject(m_Info.hProcess, (DWORD)-1);
}
