#include "ServerImplement.h"

/*
#include <thread>
#include <Socket.h>
#include <Client.h>
*/

using namespace std;
using namespace nekotama;

int main()
{
	ServerImplement tServer("chu's server");
	tServer.Start();
	
	/*
	Client clt1(&SocketFactory::GetInstance(), &StdOutLogger::GetInstance(), "127.0.0.1", "chu");
	Client clt2(&SocketFactory::GetInstance(), &StdOutLogger::GetInstance(), "127.0.0.1", "chu");
	Client clt3(&SocketFactory::GetInstance(), &StdOutLogger::GetInstance(), "127.0.0.1", "chu");
	clt1.Start();
	clt2.Start();
	clt3.Start();
	this_thread::sleep_for(chrono::milliseconds(1000));
	clt1.NotifyGameCreated();
	clt2.NotifyGameCreated();
	this_thread::sleep_for(chrono::milliseconds(4000));
	clt1.GentlyStop();
	clt2.GentlyStop();
	clt3.GentlyStop();
	clt1.Wait();
	clt2.Wait();
	clt3.Wait();
	*/

	tServer.Wait();
	return 0;
}
