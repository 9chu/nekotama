#pragma once
#include <string>
#include <Windows.h>

namespace nekotama
{
	class Process
	{
	public:
		static std::wstring GetGurrentDirectory();
	private:
		PROCESS_INFORMATION m_Info;
	public:
		DWORD GetProcessId();
		void Resume();
		void Suspend();
		void Terminate();
		UINT GetExitCode();
		void Wait();
	public:
		Process(LPCWSTR Path, LPCWSTR CommandLine, LPCWSTR Environment);
		~Process(void);
	};
}
