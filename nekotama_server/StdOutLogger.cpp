﻿#include "StdOutLogger.h"

#include "ConsoleColor.h"

using namespace std;
using namespace nekotama;

////////////////////////////////////////////////////////////////////////////////

StdOutLogger& StdOutLogger::GetInstance()
{
	static StdOutLogger s_Instance;
	return s_Instance;
}

void StdOutLogger::Log(const std::string& info, LogType type)NKNOEXCEPT
{
	lock_guard<mutex> scope(m_Lock);
	switch (type)
	{
	case LogType::Infomation:
		SetConsoleTextColor(ConsoleColors::Grey);
		printf("[信息] %s\n", info.c_str());
		break;
	case LogType::Warning:
		SetConsoleTextColor(ConsoleColors::Yellow);
		printf("[警告] %s\n", info.c_str());
		break;
	case LogType::Error:
		SetConsoleTextColor(ConsoleColors::RedOrange);
		printf("[错误] %s\n", info.c_str());
		break;
	}
}
