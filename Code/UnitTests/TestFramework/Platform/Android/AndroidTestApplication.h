#pragma once

#include <TestFramework/TestFrameworkDLL.h>

struct android_app;

int ezAndroidMain(int argc, char** argv);

// \brief A small wrapper class around the android message loop to wait for window creation before starting tests.
class EZ_TEST_DLL ezAndroidTestApplication
{
public:
  ezAndroidTestApplication(struct android_app* pApp);
  void HandleCmd(int32_t cmd);
  void AndroidRun();

  static void ezAndroidHandleCmd(struct android_app* pApp, int32_t cmd);

private:
  struct android_app* m_pApp = nullptr;
  bool m_bStarted = false;
};
