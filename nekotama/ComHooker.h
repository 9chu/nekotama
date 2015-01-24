#pragma once
#include <cstdint>
#include <Windows.h>
#include <Dbghelp.h>

namespace nekotama
{
	class ComHooker
	{
	public:
		/// @brief     ��ȡ������ַ
		static void* GetFuncPtr(void* p, uint32_t func);
		/// @brief     �޸�COMָ�����
		/// @param[in] p    ���޸ĵ�COMָ��
		/// @param[in] func �ڼ����麯������IUnknown��ʼ���㣩
		/// @param[in] newf �µĺ���ָ��
		/// @return    ԭʼ�ĺ���ָ��
		static void* HookVptr(void* p, uint32_t func, void* newf);
	};
}
