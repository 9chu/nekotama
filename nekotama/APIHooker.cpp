#include "APIHooker.h"

using namespace nekotama;

APIHooker::APIHooker(LPCWSTR ModuleName)
{
	m_hModule = GetModuleHandle(ModuleName);

	// 获得导入表
    ULONG ulSize;
    m_pImportDesc =
		(PIMAGE_IMPORT_DESCRIPTOR)ImageDirectoryEntryToData(
		m_hModule,
		TRUE,
		IMAGE_DIRECTORY_ENTRY_IMPORT,
		&ulSize
		);
}

APIHooker::~APIHooker()
{}

PROC APIHooker::HookFunction(LPCSTR LibName, LPCSTR FuncName, PROC NewAddr)
{
	HMODULE tLibHandle = GetModuleHandleA(LibName);

	// 在导入表中寻找目标模块
	PIMAGE_IMPORT_DESCRIPTOR pImportDesc = m_pImportDesc;
	while (pImportDesc->Name)
    {
		PSTR pszModName = (PSTR)((PBYTE)m_hModule + pImportDesc->Name);
		HMODULE tLib = GetModuleHandleA(pszModName);
		if(tLibHandle==tLib)
			break;
        pImportDesc++;
    }
	if(!pImportDesc->Name) return NULL;

	// 寻找函数块
    PIMAGE_THUNK_DATA pThunk =
		(PIMAGE_THUNK_DATA)((PBYTE) m_hModule + pImportDesc->FirstThunk);

	PROC pfnHookAPIAddr = GetProcAddress(tLibHandle, FuncName);
    while (pThunk->u1.Function)
    {
        PROC* ppfn = (PROC*) &pThunk->u1.Function;

        if (*ppfn == pfnHookAPIAddr)
        {
            MEMORY_BASIC_INFORMATION mbi;
            VirtualQuery(ppfn, &mbi, sizeof(MEMORY_BASIC_INFORMATION));
            VirtualProtect(mbi.BaseAddress, mbi.RegionSize, PAGE_READWRITE, &mbi.Protect);
			
			*ppfn = *NewAddr;

            DWORD dwOldProtect;
            VirtualProtect(mbi.BaseAddress, mbi.RegionSize, mbi.Protect, &dwOldProtect);
            break;
        }
        pThunk++;
    }

	return pfnHookAPIAddr;
}
