#pragma once
#include <Windows.h>

namespace nekotama
{
	/// @brief 获取主线程id
	DWORD GetMainThread();

	/// @brief 恢复主线程运行
	void ResumeMainThread();
}
