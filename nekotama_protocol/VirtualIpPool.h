#pragma once
#include <cstdint>
#include <vector>
#include <string>

namespace nekotama
{
	/// @brief ����IP��
	class VirtualIpPool
	{
	public:
		/// @brief ת��ip��ַ���ַ���
		static std::string IpToString(uint32_t IpAddr);
		/// @brief ת���ַ�����ip��ַ
		static uint32_t StringToIp(const std::string& str);
	private:
		uint32_t m_subNetwork;  ///< @brief ����
		uint32_t m_cLastIp;  ///< @brief ���һ���������ip��ַ
		uint32_t m_IpMask;  ///< @brief ip����
		std::vector<uint32_t> m_IpPool;  ///< @brief ip��ַ��
	public:
		/// @brief  ����һ��ip��ַ
		/// @return ����0ָʾ����ʧ��
		uint32_t Alloc();
		/// @brief �ͷ�һ��ip��ַ
		void Free(uint32_t IpAddr);
	public:
		/// @param[in] IpStart ��ʼ����ĵ�ַ��������
		/// @param[in] IpMask  ��������
		VirtualIpPool(uint32_t IpStart, uint32_t IpMask);
	};
}
