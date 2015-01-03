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
		/// @brief  �ҹ�ģ��ĺ���
		/// @param  LibName  ������
		/// @param  FuncName �⺯��
		/// @param  NewAddr  �µĵ�ַ
		/// @return ԭ�е�ַ
		PROC HookFunction(LPCSTR LibName, LPCSTR FuncName, PROC NewAddr);
	public:
		/// @brief ����ģ��ҹ���
		/// @param ModuleName ģ������NULL���ڱ�ʾ��ģ��
		APIHooker(LPCWSTR ModuleName = NULL);
		~APIHooker();
	};
}
