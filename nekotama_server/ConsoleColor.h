#pragma once

#ifdef _WIN32
enum class ConsoleColors
{
	Black = 0,
	Blue = 1,
	DarkGreen = 2,
	LightBlue = 3,
	Red = 4,
	Purple = 5,
	Orange = 6,
	Grey = 7,
	DarkerGrey = 8,
	MediumBlue = 9,
	LightGreen = 10,
	Teal = 11,
	RedOrange = 12,
	LightPurple = 13,
	Yellow = 14,
	White = 15,
};
#else
enum class ConsoleColors
{
	Black = 30,
	Red = 31,
	LightGreen = 32,
	Yellow = 33,
	Blue = 34,
	Purple = 35,
	MediumBlue = 36,
	White = 37,
	DarkGreen = 32,
	LightBlue = 34,
	Orange = 33,
	Grey = 37,
	DarkerGrey = 37,
	Teal = 30,
	RedOrange = 33,
	LightPurple = 31,
};
#endif

extern void SetConsoleTextColor(ConsoleColors color);
