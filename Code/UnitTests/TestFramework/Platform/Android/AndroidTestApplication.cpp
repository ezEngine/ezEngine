#include <TestFramework/TestFrameworkPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_ANDROID)

#  include <Foundation/Basics/Platform/Android/AndroidUtils.h>
#  include <Foundation/Logging/Log.h>
#  include <TestFramework/Platform/Android/AndroidTestApplication.h>
#  include <TestFramework/Utilities/TestSetup.h>
#  include <android/native_activity.h>
#  include <android_native_app_glue.h>

ezAndroidTestApplication::ezAndroidTestApplication(struct android_app* pApp)
  : m_pApp(pApp)
{
  pApp->userData = this;
  pApp->onAppCmd = ezAndroidHandleCmd;
  ezAndroidUtils::SetAndroidApp(pApp);
}

void ezAndroidTestApplication::HandleCmd(int32_t cmd)
{
  switch (cmd)
  {
    case APP_CMD_INIT_WINDOW:
      if (m_pApp->window != nullptr)
      {
        ezAndroidMain(0, nullptr);
        m_bStarted = true;

        int width = ANativeWindow_getWidth(m_pApp->window);
        int height = ANativeWindow_getHeight(m_pApp->window);
        ezLog::Info("Init Window: {}x{}", width, height);
      }
      break;
    default:
      break;
  }
}
void ezAndroidTestApplication::AndroidRun()
{
  bool bRun = true;
  while (true)
  {
    struct android_poll_source* pSource = nullptr;
    int iIdent = 0;
    int iEvents = 0;
    while ((iIdent = ALooper_pollAll(0, nullptr, &iEvents, (void**)&pSource)) >= 0)
    {
      if (pSource != nullptr)
        pSource->process(m_pApp, pSource);
    }

    // APP_CMD_INIT_WINDOW has not triggered yet. Engine is not yet started.
    if (!m_bStarted)
      continue;

    if (bRun && ezTestSetup::RunTests() != ezTestAppRun::Continue)
    {
      bRun = false;
      ANativeActivity_finish(m_pApp->activity);
    }
    if (m_pApp->destroyRequested)
    {
      break;
    }
  }
}

void ezAndroidTestApplication::ezAndroidHandleCmd(struct android_app* pApp, int32_t cmd)
{
  ezAndroidTestApplication* pAndroidApp = static_cast<ezAndroidTestApplication*>(pApp->userData);
  pAndroidApp->HandleCmd(cmd);
}

#endif


