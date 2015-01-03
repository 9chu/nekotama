#include "ClientSession.h"

using namespace std;
using namespace nekotama;

////////////////////////////////////////////////////////////////////////////////

ClientSession::ClientSession(SocketHandle handle, const std::string& ip, uint16_t port, ILogger* pLogger)
	: m_pLogger(pLogger), m_Socket(handle), m_IP(ip), m_Port(port)
{}

bool ClientSession::RecvData()
{
	bool bEndOfPackage = false;
	uint8_t tByte;
	size_t tReaded;
	while (m_Socket->Recv((uint8_t*)&tByte, sizeof(uint8_t), tReaded))
	{
		switch (m_Checker.Update(tByte))
		{
		case PackageValidChecker::UpdateState::InvalidByte:
			throw logic_error("invalid package data.");
		case PackageValidChecker::UpdateState::EndOfPackage:
			bEndOfPackage = true;
		default:
			m_RecvBuf.push_back(tByte);
			break;
		}
		if (bEndOfPackage)
			break;
	}
	return bEndOfPackage;
}

void ClientSession::SendData()
{

}

void ClientSession::Tick(uint32_t TickCount)
{

}
