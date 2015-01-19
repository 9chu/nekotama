#include "ConsoleColor.h"

#include <Windows.h>

void SetConsoleTextColor(ConsoleColors color)
{
#ifdef _WIN32  
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (WORD)color);
#else  
	printf("/033[1;40;%dm", color);
#endif  
}
