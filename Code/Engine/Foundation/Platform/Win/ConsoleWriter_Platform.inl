#include <Foundation/Platform/Win/Utils/IncludeWindows.h>

static void SetConsoleColor(WORD ui)
{
  SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), ui);
}
