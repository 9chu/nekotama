#include <iostream>
#include <vector>
#include <mutex>

#include <Socket.h>
#include <Server.h>

#include "ConsoleColor.h"

using namespace std;
using namespace nekotama;

class StdOutLogger :
	public ILogger
{
private:
	std::mutex m_Lock;
public:
	void Log(const std::string& info, LogType type)NKNOEXCEPT
	{
		std::lock_guard<std::mutex> scope(m_Lock);
		switch (type)
		{
		case LogType::Infomation:
			SetConsoleTextColor(ConsoleColors::Grey);
			printf("[ÐÅÏ¢] %s\n", info.c_str());
			break;
		case LogType::Warning:
			SetConsoleTextColor(ConsoleColors::Yellow);
			printf("[¾¯¸æ] %s\n", info.c_str());
			break;
		case LogType::Error:
			SetConsoleTextColor(ConsoleColors::RedOrange);
			printf("[´íÎó] %s\n", info.c_str());
			break;
		}
	}
} g_Logger;

class ServerListener :
	public Server
{
public:
};

int main()
{
	nekotama::Server tServer(&SocketFactory::GetInstance(), &g_Logger, "chu's server", 3);
	tServer.Start();
	tServer.Wait();
	system("pause");
	return 0;
}
