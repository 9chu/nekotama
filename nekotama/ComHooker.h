#pragma once
#include <cstdint>
#include <Windows.h>
#include <Dbghelp.h>

namespace nekotama
{
	class ComHooker
	{
	public:
		/// @brief     获取函数地址
		static void* GetFuncPtr(void* p, uint32_t func);
		/// @brief     修改COM指针虚表
		/// @param[in] p    待修改的COM指针
		/// @param[in] func 第几个虚函数（从IUnknown开始计算）
		/// @param[in] newf 新的函数指针
		/// @return    原始的函数指针
		static void* HookVptr(void* p, uint32_t func, void* newf);
	};
}
