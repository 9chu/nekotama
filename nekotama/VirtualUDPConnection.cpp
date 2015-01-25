#include "VirtualUDPConnection.h"

#include <VirtualIpPool.h>

#include "ClientImplement.h"

using namespace std;
using namespace nekotama;

VirtualUDPConnection::VirtualUDPConnection(const std::weak_ptr<ClientImplement>& pImplement)
	: m_pImplement(pImplement), m_iBindedPort(0), m_bRecvShuted(false)
{}

VirtualUDPConnection::~VirtualUDPConnection()
{
	if (!m_bRecvShuted)
	{
		m_Packages.Push(DataPackage(std::string(), 0, std::string()));
		m_bRecvShuted = true;
		unique_lock<mutex> lock(m_Mutex);
	}	
}

void VirtualUDPConnection::ReceiveData(const std::string& ip, uint16_t port, const std::string& data)
{
	m_Packages.Push(DataPackage(ip, port, data));
}

int VirtualUDPConnection::Bind(SOCKET s, const struct sockaddr* addr, int namelen)
{
	if (namelen != sizeof(sockaddr_in))
		return -1;
	sockaddr_in* addr_in = (sockaddr_in*)addr;
	if (addr_in->sin_family != AF_INET)
		return -1;

	// 不对addr_in的绑定ip做处理，全部视为0.0.0.0
	auto p = m_pImplement.lock();
	if (!p)
		return -1;
	if (addr_in->sin_port == 0)
		addr_in->sin_port = htons(p->AllocFreePort());

	if (m_iBindedPort)
		p->UnregisterConnection(m_iBindedPort);
	m_iBindedPort = ntohs(addr_in->sin_port);
	if (!p->RegisterConnection(m_iBindedPort, this))
		return -1;
	return 0;
}

int VirtualUDPConnection::CloseSocket(SOCKET s)
{
	if (!m_bRecvShuted)
	{
		m_Packages.Push(DataPackage(std::string(), 0, std::string()));
		m_bRecvShuted = true;
		unique_lock<mutex> lock(m_Mutex);
	}

	auto p = m_pImplement.lock();
	if (!p)
		return -1;
	if (m_iBindedPort)
		p->UnregisterConnection(m_iBindedPort);
	return 0;
}

int VirtualUDPConnection::RecvFrom(SOCKET s, char* buf, int len, int flags, struct sockaddr* from, int* fromlen)
{
	unique_lock<mutex> lock(m_Mutex);

	// 忽略flags参数
	if (!((fromlen == 0 && from == 0) || (*fromlen == sizeof(sockaddr_in) && from != 0)))
		return -1;
	if (m_bRecvShuted)
		return -1;
	
	// 抽取数据
	DataPackage t;
	if ((flags & MSG_PEEK) == MSG_PEEK)
		m_Packages.Peek(t);
	else
		m_Packages.Pop(t);
	if (t.port == 0)
	{
		m_bRecvShuted = true;
		return 0;
	}

	// 填充数据
	len = min(len, (int)t.data.size());
	if (len > 0)
		memcpy(buf, t.data.data(), len);

	// 填充地址
	if (from)
	{
		sockaddr_in* pfrom = (sockaddr_in*)from;
		pfrom->sin_family = AF_INET;
		pfrom->sin_port = htons(t.port);
		pfrom->sin_addr.S_un.S_addr = inet_addr(t.ip.c_str());
	}

	return len;
}

int VirtualUDPConnection::SendTo(SOCKET s, const char* buf, int len, int flags, const struct sockaddr* to, int tolen)
{
	// 忽略flags参数
	if (tolen != sizeof(sockaddr_in))
		return -1;

	auto p = m_pImplement.lock();
	if (!p)
		return -1;
	std::string tBuf;
	if (len > 0)
	{
		tBuf.resize(len);
		memcpy((void*)tBuf.data(), buf, len);
	}
	sockaddr_in* pto = (sockaddr_in*)to;
	p->ForwardingPackage(inet_ntoa(pto->sin_addr), ntohs(pto->sin_port), m_iBindedPort, tBuf);
	return len;
}

int VirtualUDPConnection::Shutdown(SOCKET s, int how)
{
	if (how == 0 || how == 2)
	{
		if (!m_bRecvShuted)
		{
			m_Packages.Push(DataPackage(std::string(), 0, std::string()));
			m_bRecvShuted = true;
			unique_lock<mutex> lock(m_Mutex);
		}
	}
		
	return 0;  // 忽略该操作
}
