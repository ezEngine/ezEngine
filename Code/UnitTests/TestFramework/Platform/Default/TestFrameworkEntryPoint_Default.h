#pragma once

#include <Foundation/Basics.h>

#ifndef EZ_TESTFRAMEWORK_ENTRY_POINT_CODE_INJECTION
#  define EZ_TESTFRAMEWORK_ENTRY_POINT_CODE_INJECTION
#endif

/// \brief Macro to define the application entry point for all test applications
#define EZ_TESTFRAMEWORK_ENTRY_POINT_BEGIN(szTestName, szNiceTestName)                    \
  /* Enables that on machines with multiple GPUs the NVIDIA GPU is preferred */           \
  EZ_TESTFRAMEWORK_ENTRY_POINT_CODE_INJECTION                                             \
  EZ_APPLICATION_ENTRY_POINT_CODE_INJECTION                                               \
  int main(int argc, char** argv)                                                         \
  {                                                                                       \
    ezTestSetup::InitTestFramework(szTestName, szNiceTestName, argc, (const char**)argv); \
    /* Execute custom init code here by using the BEGIN/END macros directly */

#define EZ_TESTFRAMEWORK_ENTRY_POINT_END()                        \
  while (ezTestSetup::RunTests() == ezTestAppRun::Continue)       \
  {                                                               \
  }                                                               \
  const ezInt32 iFailedTests = ezTestSetup::GetFailedTestCount(); \
  ezTestSetup::DeInitTestFramework();                             \
  return iFailedTests;                                            \
  }

#define EZ_TESTFRAMEWORK_ENTRY_POINT(szTestName, szNiceTestName)             \
  EZ_TESTFRAMEWORK_ENTRY_POINT_BEGIN(szTestName, szNiceTestName)             \
  /* Execute custom init code here by using the BEGIN/END macros directly */ \
  EZ_TESTFRAMEWORK_ENTRY_POINT_END()
