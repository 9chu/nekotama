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
			cout << "[ÐÅÏ¢]" << info << endl;
			break;
		case LogType::Warning:
			cout << "[¾¯¸æ]" << info << endl;
			break;
		case LogType::Error:
			cout << "[´íÎó]" << info << endl;
			break;
		}
	}
} g_Logger;

int main()
{
	nekotama::Server tServer(&SocketFactory::GetInstance(), &g_Logger);
	tServer.Start();
	tServer.Wait();
	system("pause");
	return 0;
}
