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
			printf("[��Ϣ] %s\n", info.c_str());
			break;
		case LogType::Warning:
			SetConsoleTextColor(ConsoleColors::Yellow);
			printf("[����] %s\n", info.c_str());
			break;
		case LogType::Error:
			SetConsoleTextColor(ConsoleColors::RedOrange);
			printf("[����] %s\n", info.c_str());
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
	void OnConnectFailed()NKNOEXCEPT { g_Logger.Log("���Կͻ���: ����ʧ�ܡ�", LogType::Error); }
	void OnNotSupportedServerVersion()NKNOEXCEPT { g_Logger.Log("���Կͻ���: ��֧�ֵİ汾��", LogType::Error); }
	void OnKicked(KickReason why)NKNOEXCEPT { g_Logger.Log("���Կͻ���: ���������߳���", LogType::Error); }
	void OnLostConnection()NKNOEXCEPT { g_Logger.Log("���Կͻ���: ��ʧ���ӡ�", LogType::Error); }
	void OnLoginSucceed(const std::string& server, const std::string& nickname, const std::string& addr, uint16_t gameport)NKNOEXCEPT{}
public:
	TestClient(ISocketFactory* pFactory, ILogger* pLogger, const std::string& serverip, const std::string& nickname, uint16_t port = 12801)
		: Client(pFactory, pLogger, serverip, nickname, port) {}
};
#endif

int main()
{
#ifdef _DEBUG
	// ����
	TestServer tServer(&SocketFactory::GetInstance(), &g_Logger, "chu's server", 3);
	tServer.Start();
	TestClient t(&SocketFactory::GetInstance(), &g_Logger, "127.0.0.1", "chu");
	t.Start();
	this_thread::sleep_for(chrono::milliseconds(1000 * 5));
	t.GentlyStop();
	t.Wait();
	g_Logger.Log("�ͻ����ѹرա�", LogType::Infomation);
	tServer.Wait();
#endif
	system("pause");
	return 0;
}
