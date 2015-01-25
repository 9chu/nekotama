#include "Injecter.h"
#include "Process.h"

#include "Encoding.h"
#include <ConfigFile.h>

using namespace std;
using namespace nekotama;

#ifndef CLIENT_CONFIG_PATH
#define CLIENT_CONFIG_PATH "nekotama.conf"
#endif

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// 载入配置文件
	ConfigFile tCfgFile;
	if (!tCfgFile.Load(CLIENT_CONFIG_PATH))
	{
		MessageBox(0, L"未找到配置文件，请创建并编辑nekotama.conf。", L"错误", MB_ICONERROR);
		return -1;
	}

	if (!tCfgFile.ContainsKey("game"))
	{
		MessageBox(0, L"错误的配置文件，请设置game项为游戏路径。", L"错误", MB_ICONERROR);
		return -1;
	}
	if (!tCfgFile.ContainsKey("args"))
	{
		MessageBox(0, L"错误的配置文件，请设置args项为参数或留空。", L"错误", MB_ICONERROR);
		return -1;
	}
	if (!tCfgFile.ContainsKey("workdir"))
	{
		MessageBox(0, L"错误的配置文件，请设置workdir项为工作目录。", L"错误", MB_ICONERROR);
		return -1;
	}
	if (!tCfgFile.ContainsKey("server"))
	{
		MessageBox(0, L"错误的配置文件，请设置server项为服务器ip。", L"错误", MB_ICONERROR);
		return -1;
	}
	if (!tCfgFile.ContainsKey("port"))
	{
		MessageBox(0, L"错误的配置文件，请设置port项为服务器端口。", L"错误", MB_ICONERROR);
		return -1;
	}
	if (!tCfgFile.ContainsKey("nickname"))
	{
		MessageBox(0, L"错误的配置文件，请设置nickname项为昵称。", L"错误", MB_ICONERROR);
		return -1;
	}
	else
	{
		if (tCfgFile["nickname"].length() > 16 || tCfgFile["nickname"].length() < 3)
		{
			MessageBox(0, L"错误的配置文件，昵称不得长于16字符或短于3字符。", L"错误", MB_ICONERROR);
			return -1;
		}
	}

	wstring tGamePath = MultiByteToWideChar(tCfgFile["game"]);
	wstring tArgs = tGamePath + L" " + MultiByteToWideChar(tCfgFile["args"]);
	wstring tWorkDir = MultiByteToWideChar(tCfgFile["workdir"]);
	try
	{
		Process  tProg(tGamePath.c_str(), tArgs.c_str(), tWorkDir.c_str());
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
	}
	catch (const std::exception& e)
	{
		MessageBox(0, MultiByteToWideChar(e.what()).c_str(), L"启动失败", MB_ICONERROR);
		return -2;
	}
	return 0;
}
