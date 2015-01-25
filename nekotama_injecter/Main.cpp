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
	// ���������ļ�
	ConfigFile tCfgFile;
	if (!tCfgFile.Load(CLIENT_CONFIG_PATH))
	{
		MessageBox(0, L"δ�ҵ������ļ����봴�����༭nekotama.conf��", L"����", MB_ICONERROR);
		return -1;
	}

	if (!tCfgFile.ContainsKey("game"))
	{
		MessageBox(0, L"����������ļ���������game��Ϊ��Ϸ·����", L"����", MB_ICONERROR);
		return -1;
	}
	if (!tCfgFile.ContainsKey("args"))
	{
		MessageBox(0, L"����������ļ���������args��Ϊ���������ա�", L"����", MB_ICONERROR);
		return -1;
	}
	if (!tCfgFile.ContainsKey("workdir"))
	{
		MessageBox(0, L"����������ļ���������workdir��Ϊ����Ŀ¼��", L"����", MB_ICONERROR);
		return -1;
	}
	if (!tCfgFile.ContainsKey("server"))
	{
		MessageBox(0, L"����������ļ���������server��Ϊ������ip��", L"����", MB_ICONERROR);
		return -1;
	}
	if (!tCfgFile.ContainsKey("port"))
	{
		MessageBox(0, L"����������ļ���������port��Ϊ�������˿ڡ�", L"����", MB_ICONERROR);
		return -1;
	}
	if (!tCfgFile.ContainsKey("nickname"))
	{
		MessageBox(0, L"����������ļ���������nickname��Ϊ�ǳơ�", L"����", MB_ICONERROR);
		return -1;
	}
	else
	{
		if (tCfgFile["nickname"].length() > 16 || tCfgFile["nickname"].length() < 3)
		{
			MessageBox(0, L"����������ļ����ǳƲ��ó���16�ַ������3�ַ���", L"����", MB_ICONERROR);
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
		MessageBox(0, MultiByteToWideChar(e.what()).c_str(), L"����ʧ��", MB_ICONERROR);
		return -2;
	}
	return 0;
}
