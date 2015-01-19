#pragma once
#include <string>

namespace nekotama
{
	enum class LogType
	{
		Infomation,
		Warning,
		Error
	};

	/// @brief ��־�ӿ�
	/// @note  �ýӿڱ���Ϊ�̰߳�ȫ��
	struct ILogger
	{
		virtual void Log(const std::string& info, LogType type = LogType::Infomation)NKNOEXCEPT = 0;
	};
}
