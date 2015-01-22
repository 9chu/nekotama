#include <iostream>
#include <vector>
#include <mutex>

#include <Socket.h>
#include <Server.h>

#include "ConsoleColor.h"

#ifdef _DEBUG
#include "Client.h"
#endif

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
			printf("[信息] %s\n", info.c_str());
			break;
		case LogType::Warning:
			SetConsoleTextColor(ConsoleColors::Yellow);
			printf("[警告] %s\n", info.c_str());
			break;
		case LogType::Error:
			SetConsoleTextColor(ConsoleColors::RedOrange);
			printf("[错误] %s\n", info.c_str());
			break;
		}
	}
} g_Logger;

#ifdef _DEBUG
class TestServer :
	public Server
{
public:
	bool OnClientLogin(ClientSession* client, std::string& nick, std::string& addr, uint16_t port)NKNOEXCEPT
	{
		addr = "10.0.0.1";
		return true;
	}
public:
	TestServer(ISocketFactory* pFactory, ILogger* pLogger, const std::string& server_name, uint16_t maxClient = 16, uint16_t port = 12801)
		: Server(pFactory, pLogger, server_name, maxClient, port) {}
};

class TestClient :
	public Client
{
public:
	void OnConnectFailed()NKNOEXCEPT { g_Logger.Log("测试客户端: 连接失败。", LogType::Error); }
	void OnNotSupportedServerVersion()NKNOEXCEPT { g_Logger.Log("测试客户端: 不支持的版本。", LogType::Error); }
	void OnKicked(KickReason why)NKNOEXCEPT { g_Logger.Log("测试客户端: 被服务器踢出。", LogType::Error); }
	void OnLostConnection()NKNOEXCEPT { g_Logger.Log("测试客户端: 丢失连接。", LogType::Error); }
	void OnLoginSucceed(const std::string& server, const std::string& nickname, const std::string& addr, uint16_t gameport)NKNOEXCEPT{}
public:
	TestClient(ISocketFactory* pFactory, ILogger* pLogger, const std::string& serverip, const std::string& nickname, uint16_t port = 12801)
		: Client(pFactory, pLogger, serverip, nickname, port) {}
};
#endif

int main()
{
#ifdef _DEBUG
	// 测试
	TestServer tServer(&SocketFactory::GetInstance(), &g_Logger, "chu's server", 3);
	tServer.Start();
	TestClient t(&SocketFactory::GetInstance(), &g_Logger, "127.0.0.1", "chu");
	t.Start();
	this_thread::sleep_for(chrono::milliseconds(1000 * 5));
	t.GentlyStop();
	t.Wait();
	g_Logger.Log("客户端已关闭。", LogType::Infomation);
	tServer.Wait();
#endif
	system("pause");
	return 0;
}
