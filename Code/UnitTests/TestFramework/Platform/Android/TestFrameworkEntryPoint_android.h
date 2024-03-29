#pragma once

#include <TestFramework/Platform/Android/AndroidTestApplication.h>
#include <android/log.h>

#define EZ_TESTFRAMEWORK_ENTRY_POINT_BEGIN(szTestName, szNiceTestName)                                                \
  int ezAndroidMain(int argc, char** argv);                                                                           \
  EZ_APPLICATION_ENTRY_POINT_CODE_INJECTION                                                                           \
  extern "C" void android_main(struct android_app* app)                                                               \
  {                                                                                                                   \
    ezAndroidTestApplication androidApp(app);                                                                         \
    androidApp.AndroidRun();                                                                                          \
    const ezInt32 iFailedTests = ezTestSetup::GetFailedTestCount();                                                   \
    ezTestSetup::DeInitTestFramework();                                                                               \
    __android_log_print(ANDROID_LOG_ERROR, "ezEngine", "Test framework exited with return code: '%d'", iFailedTests); \
  }                                                                                                                   \
                                                                                                                      \
  int ezAndroidMain(int argc, char** argv)                                                                            \
  {                                                                                                                   \
    ezTestSetup::InitTestFramework(szTestName, szNiceTestName, 0, nullptr);                                           \
    /* Execute custom init code here by using the BEGIN/END macros directly */

#define EZ_TESTFRAMEWORK_ENTRY_POINT_END() \
  return 0;                                \
  }
