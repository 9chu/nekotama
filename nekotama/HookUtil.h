#pragma once
#include <Windows.h>

namespace nekotama
{
	/// @brief ��ȡ���߳�id
	DWORD GetMainThread();

	/// @brief �ָ����߳�����
	void ResumeMainThread();
}
