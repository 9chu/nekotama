#include "ComHooker.h"

using namespace nekotama;

struct VTable
{
	void* methods[1];
};

struct InterfaceABI
{
	VTable* vptr;
};

void* ComHooker::GetFuncPtr(void* p, uint32_t func)
{
	// ��ȡvptr
	VTable* vptr = reinterpret_cast<InterfaceABI*>(p)->vptr;

	return vptr->methods[func];
}

void* ComHooker::HookVptr(void* p, uint32_t func, void* newf)
{
	// ��ȡvptr
	VTable* vptr = reinterpret_cast<InterfaceABI*>(p)->vptr;

	// �޸Ŀɶ�д��
	MEMORY_BASIC_INFORMATION mbi;
	VirtualQuery(vptr, &mbi, sizeof(MEMORY_BASIC_INFORMATION));
	VirtualProtect(mbi.BaseAddress, mbi.RegionSize, PAGE_EXECUTE_READWRITE, &mbi.Protect);

	// �޸�ָ��
	void* old = vptr->methods[func];
	vptr->methods[func] = newf;

	// ��ԭ�ɶ�д��
	DWORD dwOldProtect;
	VirtualProtect(mbi.BaseAddress, mbi.RegionSize, mbi.Protect, &dwOldProtect);
	return old;
}
