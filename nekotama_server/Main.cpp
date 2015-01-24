#include "ServerImplement.h"

using namespace std;
using namespace nekotama;

int main()
{
	ServerImplement tServer("chu's server");
	tServer.Start();
	tServer.Wait();
	return 0;
}
