#pragma once
#include <mutex>
#include <iostream>

#include <ILogger.h>

namespace nekotama
{
	/// @brief StdOut日志输出接口
	class StdOutLogger :
		public ILogger
	{
	public:
		static StdOutLogger& GetInstance();
	private:
		std::mutex m_Lock;
	public:
		void Log(const std::string& info, LogType type)NKNOEXCEPT;
	};
}
