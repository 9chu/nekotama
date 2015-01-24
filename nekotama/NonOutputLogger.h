#pragma once
#include <ILogger.h>

namespace nekotama
{
	class NonOutputLogger :
		public ILogger
	{
	public:
		static NonOutputLogger& GetInstance()
		{
			static NonOutputLogger s_Instance;
			return s_Instance;
		}
	public:
		void Log(const std::string& info, LogType type = LogType::Infomation)NKNOEXCEPT {}
	};
}
