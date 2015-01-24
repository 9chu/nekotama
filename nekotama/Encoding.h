#pragma once
#include <string>
#include <cstdint>

namespace nekotama
{
	/// @brief     string��wstring
	/// @param[in] Org ԭʼ�ַ���
	/// @param[in] CodePage ����ҳ���������MSDN
	/// @return    ���ر�ת�����ַ���
	std::wstring MultiByteToWideChar(const std::string& Org, uint32_t CodePage = 1);

	/// @brief     wstring��string
	/// @param[in] Org ԭʼ�ַ���
	/// @param[in] CodePage ����ҳ���������MSDN
	/// @return    ���ر�ת�����ַ���
	std::string WideCharToMultiByte(const std::wstring& Org, uint32_t CodePage = 1);
}
