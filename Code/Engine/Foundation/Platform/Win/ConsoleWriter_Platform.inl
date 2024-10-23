#include <Foundation/Platform/Win/Utils/IncludeWindows.h>

void SetConsoleColor(ezUInt8 ui)
{
  SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (WORD)ui);
}
