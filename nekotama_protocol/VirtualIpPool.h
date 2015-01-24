#pragma once
#include <cstdint>
#include <vector>
#include <string>

namespace nekotama
{
	/// @brief 虚拟IP池
	class VirtualIpPool
	{
	public:
		/// @brief 转换ip地址到字符串
		static std::string IpToString(uint32_t IpAddr);
		/// @brief 转换字符串到ip地址
		static uint32_t StringToIp(const std::string& str);
	private:
		uint32_t m_subNetwork;  ///< @brief 子网
		uint32_t m_cLastIp;  ///< @brief 最后一个被分配的ip地址
		uint32_t m_IpMask;  ///< @brief ip掩码
		std::vector<uint32_t> m_IpPool;  ///< @brief ip地址池
	public:
		/// @brief  分配一个ip地址
		/// @return 返回0指示分配失败
		uint32_t Alloc();
		/// @brief 释放一个ip地址
		void Free(uint32_t IpAddr);
	public:
		/// @param[in] IpStart 起始分配的地址（不含）
		/// @param[in] IpMask  子网掩码
		VirtualIpPool(uint32_t IpStart, uint32_t IpMask);
	};
}
