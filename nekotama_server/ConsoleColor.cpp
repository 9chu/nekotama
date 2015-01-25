#include "ConsoleColor.h"

#include <cstdio>

#ifdef _WIN32
#include <Windows.h>
#endif

void SetConsoleTextColor(ConsoleColors color)
{
#ifdef _WIN32
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (WORD)color);
#else
	printf("\033[01;49;%02dm", (int)color);
#endif
}
