#include "VirtualIpPool.h"

#include <StringFormat.h>

using namespace std;
using namespace nekotama;

////////////////////////////////////////////////////////////////////////////////

std::string VirtualIpPool::IpToString(uint32_t IpAddr)
{
	return StringFormat("%d.%d.%d.%d",
		(IpAddr & 0xFF000000) >> 24,
		(IpAddr & 0x00FF0000) >> 16,
		(IpAddr & 0x0000FF00) >> 8,
		IpAddr & 0x000000FF);
}

uint32_t VirtualIpPool::StringToIp(const std::string& str)
{
	uint32_t a = 0, b = 0, c = 0, d = 0;
	sscanf(str.c_str(), "%d.%d.%d.%d", &a, &b, &c, &d);
	return ((a & 0xFF) << 24) | ((b & 0xFF) << 16) | ((c & 0xFF) << 8) | (d & 0xFF);
}

VirtualIpPool::VirtualIpPool(uint32_t IpStart, uint32_t IpMask)
	: m_cLastIp(IpStart), m_IpMask(IpMask)
{
	m_subNetwork = m_cLastIp & m_IpMask;
}

uint32_t VirtualIpPool::Alloc()
{
	// 从池中分配
	if (!m_IpPool.empty())
	{
		uint32_t ret = m_IpPool.back();
		m_IpPool.pop_back();
		return ret;
	}

	// 分配一个新的IP
	if (m_cLastIp != 0)
	{
		++m_cLastIp;
		uint32_t d = m_cLastIp & 0xFF;
		if (d == 255)
		{
			++m_cLastIp;
			d = m_cLastIp & 0xFF;
		}
		if (d == 0)
			++m_cLastIp;
		if (d == 1)
			++m_cLastIp;
		if ((m_cLastIp & m_IpMask) != m_subNetwork)
		{
			// IP段耗尽
			m_cLastIp = 0;
			return 0;
		}
		return m_cLastIp;
	}
	
	return 0;
}

void VirtualIpPool::Free(uint32_t IpAddr)
{
	// 检查是否为有效ip
	if ((IpAddr & m_IpMask) != m_subNetwork)
		return;
	else
		m_IpPool.push_back(IpAddr);
}
