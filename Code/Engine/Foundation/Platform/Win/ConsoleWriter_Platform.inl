#include <Foundation/Platform/Win/Utils/IncludeWindows.h>

void SetConsoleColor(ezUInt8 uiColor)
{
  SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (WORD)uiColor);
}
