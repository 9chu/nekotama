#pragma once
#include <Windows.h>
#include <Dbghelp.h>

namespace nekotama
{
	class APIHooker
	{
	private:
		HMODULE m_hModule;
		PIMAGE_IMPORT_DESCRIPTOR m_pImportDesc;
	public:
		/// @brief  挂钩模块的函数
		/// @param  LibName  库名称
		/// @param  FuncName 库函数
		/// @param  NewAddr  新的地址
		/// @return 原有地址
		PROC HookFunction(LPCSTR LibName, LPCSTR FuncName, PROC NewAddr);
	public:
		/// @brief 创建模块挂钩器
		/// @param ModuleName 模块名，NULL用于表示主模块
		APIHooker(LPCWSTR ModuleName = NULL);
		~APIHooker();
	};
}
