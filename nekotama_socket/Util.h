#pragma once
#include <string>
#include <cstdint>

#define NKNOEXCEPT throw()

namespace nekotama
{
	/// @brief �ַ�����ʽ��
	/// @note  ֧�� %f %d %u %lf %ld %lu %c %s����֧�־��ȴ����Դ��ݴ���
	std::string StringFormat(const char* Format, ...)NKNOEXCEPT;
}
