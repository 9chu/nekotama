#include "ServerImplement.h"
#include "StdOutLogger.h"

#include <ConfigFile.h>

using namespace std;
using namespace nekotama;

#ifndef SERVER_CONFIG_PATH
#define SERVER_CONFIG_PATH "nekotama_server.conf"
#endif

int main()
{
	ConfigFile tConfig;
	tConfig["name"] = "nekotama";
	tConfig["port"] = "12801";
	tConfig["max"] = "16";
	if (!tConfig.Load(SERVER_CONFIG_PATH, true))
		 StdOutLogger::GetInstance().Log("未找到配置文件，使用默认配置。", LogType::Warning);
	int tMaxClient = atoi(tConfig["max"].c_str());
	if (tMaxClient < 1)
	{
		StdOutLogger::GetInstance().Log("无效的最大客户数，使用默认配置。", LogType::Warning);
		tMaxClient = 16;
	}
	int tPort = atoi(tConfig["port"].c_str());
	if (tPort <= 1024 || tPort > 65535)
	{
		StdOutLogger::GetInstance().Log("无效的端口，使用默认配置。", LogType::Warning);
		tPort = 12801;
	}

	ServerImplement tServer(tConfig["name"], tMaxClient, (uint16_t)tPort);
	tServer.Start();
	tServer.Wait();
	return 0;
}
