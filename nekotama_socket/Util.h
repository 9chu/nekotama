#pragma once
#include <string>
#include <cstdint>

#define NKNOEXCEPT throw()

namespace nekotama
{
	/// @brief 字符串格式化
	/// @note  支持 %f %d %u %lf %ld %lu %c %s，不支持精度处理，自带容错处理
	std::string StringFormat(const char* Format, ...)NKNOEXCEPT;
}
