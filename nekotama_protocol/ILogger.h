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

	/// @brief 日志接口
	/// @note  该接口必须为线程安全的
	struct ILogger
	{
		virtual void Log(const std::string& info, LogType type = LogType::Infomation)NKNOEXCEPT = 0;
	};
}
