#pragma once

#include <Foundation/Basics.h>

#if EZ_ENABLED(EZ_PLATFORM_ANDROID)

#  include <TestFramework/Platform/Android/TestFrameworkEntryPoint_android.h>

#else

#  ifdef EZ_NV_OPTIMUS
#    undef EZ_NV_OPTIMUS
#  endif

#  if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#    include <Foundation/Platform/Win/Utils/MinWindows.h>
#    define EZ_NV_OPTIMUS                                                                           \
      extern "C"                                                                                    \
      {                                                                                             \
        _declspec(dllexport) ezMinWindows::DWORD NvOptimusEnablement = 0x00000001;                  \
        _declspec(dllexport) ezMinWindows::DWORD AmdPowerXpressRequestHighPerformance = 0x00000001; \
      }
#  else
#    define EZ_NV_OPTIMUS
#  endif

/// \brief Macro to define the application entry point for all test applications
#  define EZ_TESTFRAMEWORK_ENTRY_POINT_BEGIN(szTestName, szNiceTestName)                    \
    /* Enables that on machines with multiple GPUs the NVIDIA GPU is preferred */           \
    EZ_NV_OPTIMUS                                                                           \
    EZ_APPLICATION_ENTRY_POINT_CODE_INJECTION                                               \
    int main(int argc, char** argv)                                                         \
    {                                                                                       \
      ezTestSetup::InitTestFramework(szTestName, szNiceTestName, argc, (const char**)argv); \
      /* Execute custom init code here by using the BEGIN/END macros directly */

#  define EZ_TESTFRAMEWORK_ENTRY_POINT_END()                        \
    while (ezTestSetup::RunTests() == ezTestAppRun::Continue)       \
    {                                                               \
    }                                                               \
    const ezInt32 iFailedTests = ezTestSetup::GetFailedTestCount(); \
    ezTestSetup::DeInitTestFramework();                             \
    return iFailedTests;                                            \
    }

#endif

#define EZ_TESTFRAMEWORK_ENTRY_POINT(szTestName, szNiceTestName)             \
  EZ_TESTFRAMEWORK_ENTRY_POINT_BEGIN(szTestName, szNiceTestName)             \
  /* Execute custom init code here by using the BEGIN/END macros directly */ \
  EZ_TESTFRAMEWORK_ENTRY_POINT_END()
