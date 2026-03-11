#pragma once

#include <Foundation/Platform/Win/Utils/MinWindows.h>
#define EZ_TESTFRAMEWORK_ENTRY_POINT_CODE_INJECTION                                             \
  extern "C"                                                                                    \
  {                                                                                             \
    _declspec(dllexport) ezMinWindows::DWORD NvOptimusEnablement = 0x00000001;                  \
    _declspec(dllexport) ezMinWindows::DWORD AmdPowerXpressRequestHighPerformance = 0x00000001; \
  }

#include <TestFramework/Platform/Default/TestFrameworkEntryPoint_Default.h>
