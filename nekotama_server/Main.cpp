#include <iostream>
#include <vector>

#include <Socket.h>
#include <Server.h>

using namespace std;
using namespace nekotama;

class StdOutLogger :
	public ILogger
{
private:
	std::mutex m_Lock;
public:
	void Log(const std::string& info, LogType type)
	{
		std::lock_guard<std::mutex> scope(m_Lock);
		switch (type)
		{
		case LogType::Infomation:
			cout << "[信息]" << info << endl;
			break;
		case LogType::Warning:
			cout << "[警告]" << info << endl;
			break;
		case LogType::Error:
			cout << "[错误]" << info << endl;
			break;
		}
	}
} g_Logger;

class ServerListener :
	public IServerListener
{
public:
	void OnCreateSession(ClientSession* pSession)
	{
		g_Logger.Log("客户端会话建立", LogType::Infomation);
	}
	void OnCloseSession(ClientSession* pSession)
	{
		g_Logger.Log("客户端会话关闭", LogType::Infomation);
	}
};

int main()
{
	nekotama::Server tServer(&SocketFactory::GetInstance(), &g_Logger);
	tServer.Start();
	tServer.Wait();
	system("pause");
	return 0;
}
