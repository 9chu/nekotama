#include "Util.h"

#include <cstdarg>

using namespace std;
using namespace nekotama;

////////////////////////////////////////////////////////////////////////////////

std::string nekotama::StringFormat(const char* Format, ...)
{
	std::string tRet;
	va_list vaptr;

	va_start(vaptr, Format);
	while (*Format != '\0')
	{
		char c = *Format;
		if (c != '%')
			tRet.push_back(c);
		else
		{
			c = *(++Format);

			switch (c)
			{
			case '%':
				tRet.push_back('%');
				break;
			case 'd':
				tRet.append(std::to_string(va_arg(vaptr, int32_t)));
				break;
			case 'f':
				tRet.append(std::to_string(va_arg(vaptr, double)));
				break;
			case 'l':
				c = *(++Format);
				switch (c)
				{
				case 'f':
					tRet.append(std::to_string(va_arg(vaptr, double)));
					break;
				case 'd':
					tRet.append(std::to_string(va_arg(vaptr, int64_t)));
					break;
				case 'u':
					tRet.append(std::to_string(va_arg(vaptr, uint64_t)));
					break;
				default:
					tRet.append("%l");
					if (c == '\0')
						Format--;
					else
						tRet.push_back(c);
					break;
				}
				break;
			case 'u':
				tRet.append(std::to_string(va_arg(vaptr, uint32_t)));
				break;
			case 'c':
				tRet.push_back(va_arg(vaptr, int32_t));
				break;
			case 's':
			{
				const char* p = va_arg(vaptr, char*);
				if (p)
					tRet.append(p);
				else
					tRet.append("<null>");
			}
				break;
			default:
				tRet.push_back('%');
				if (c == '\0')
					Format--;
				else
					tRet.push_back(c);
				break;
			}
		}
		Format++;
	}
	va_end(vaptr);
	return tRet;
}

std::wstring nekotama::StringFormat(const wchar_t* Format, ...)NKNOEXCEPT
{
	std::wstring tRet;
	va_list vaptr;

	va_start(vaptr, Format);
	while (*Format != L'\0')
	{
		wchar_t c = *Format;
		if (c != L'%')
			tRet.push_back(c);
		else
		{
			c = *(++Format);

			switch (c)
			{
			case L'%':
				tRet.push_back(L'%');
				break;
			case L'd':
				tRet.append(std::to_wstring(va_arg(vaptr, int32_t)));
				break;
			case L'f':
				tRet.append(std::to_wstring(va_arg(vaptr, double)));
				break;
			case L'l':
				c = *(++Format);
				switch (c)
				{
				case L'f':
					tRet.append(std::to_wstring(va_arg(vaptr, double)));
					break;
				case L'd':
					tRet.append(std::to_wstring(va_arg(vaptr, int64_t)));
					break;
				case L'u':
					tRet.append(std::to_wstring(va_arg(vaptr, uint64_t)));
					break;
				default:
					tRet.append(L"%l");
					if (c == L'\0')
						Format--;
					else
						tRet.push_back(c);
					break;
				}
				break;
			case L'u':
				tRet.append(std::to_wstring(va_arg(vaptr, uint32_t)));
				break;
			case L'c':
				tRet.push_back(va_arg(vaptr, int32_t));
				break;
			case L's':
			{
				const wchar_t* p = va_arg(vaptr, wchar_t*);
				if (p)
					tRet.append(p);
				else
					tRet.append(L"<null>");
			}
				break;
			default:
				tRet.push_back(L'%');
				if (c == L'\0')
					Format--;
				else
					tRet.push_back(c);
				break;
			}
		}
		Format++;
	}
	va_end(vaptr);
	return tRet;
}
