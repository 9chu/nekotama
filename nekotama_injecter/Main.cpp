#include "Injecter.h"
#include "Process.h"

using namespace nekotama;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	Process  tProg(L"D:\\Touhou\\th123\\th123.exe", L"D:\\Touhou\\th123\\th123.exe", L"D:\\Touhou\\th123\\");
#ifdef _DEBUG
	Injecter tInject(
		tProg.GetProcessId(), 
		(Process::GetGurrentDirectory() + L"\\nekotama_d.dll").c_str());
#else
	Injecter tInject(
		tProg.GetProcessId(),
		(Process::GetGurrentDirectory() + L"\\nekotama.dll").c_str());
#endif
	tProg.Wait();

	return 0;
}
