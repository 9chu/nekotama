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
	// 获取vptr
	VTable* vptr = reinterpret_cast<InterfaceABI*>(p)->vptr;

	return vptr->methods[func];
}

void* ComHooker::HookVptr(void* p, uint32_t func, void* newf)
{
	// 获取vptr
	VTable* vptr = reinterpret_cast<InterfaceABI*>(p)->vptr;

	// 修改可读写性
	MEMORY_BASIC_INFORMATION mbi;
	VirtualQuery(vptr, &mbi, sizeof(MEMORY_BASIC_INFORMATION));
	VirtualProtect(mbi.BaseAddress, mbi.RegionSize, PAGE_EXECUTE_READWRITE, &mbi.Protect);

	// 修改指针
	void* old = vptr->methods[func];
	vptr->methods[func] = newf;

	// 还原可读写性
	DWORD dwOldProtect;
	VirtualProtect(mbi.BaseAddress, mbi.RegionSize, mbi.Protect, &dwOldProtect);
	return old;
}
