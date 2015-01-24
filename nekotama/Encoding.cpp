#include "Encoding.h"

#include <Windows.h>

using namespace std;

std::wstring nekotama::MultiByteToWideChar(const std::string& Org, uint32_t CodePage)
{
	uint32_t dwNum = ::MultiByteToWideChar(CodePage, 0, Org.c_str(), -1, NULL, 0); // 获得长度
	wchar_t *pwText = new wchar_t[dwNum];
	::MultiByteToWideChar(CodePage, 0, Org.c_str(), -1, pwText, dwNum);		   // 获得数据
	wstring retStr(pwText);
	delete[] pwText;
	return retStr;
}

std::string nekotama::WideCharToMultiByte(const std::wstring& Org, uint32_t CodePage)
{
	DWORD tCount = ::WideCharToMultiByte(CodePage, NULL, Org.c_str(), -1, NULL, 0, NULL, FALSE);
	char *tText = NULL;
	tText = new char[tCount];
	::WideCharToMultiByte(CodePage, NULL, Org.c_str(), -1, tText, tCount, NULL, FALSE);
	string tRet = tText;
	delete[]tText;
	return tRet;
}
