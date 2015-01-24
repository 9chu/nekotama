#pragma once
#include <string>
#include <cstdint>

namespace nekotama
{
	/// @brief     string到wstring
	/// @param[in] Org 原始字符串
	/// @param[in] CodePage 代码页，具体查阅MSDN
	/// @return    返回被转换的字符串
	std::wstring MultiByteToWideChar(const std::string& Org, uint32_t CodePage = 1);

	/// @brief     wstring到string
	/// @param[in] Org 原始字符串
	/// @param[in] CodePage 代码页，具体查阅MSDN
	/// @return    返回被转换的字符串
	std::string WideCharToMultiByte(const std::wstring& Org, uint32_t CodePage = 1);
}
