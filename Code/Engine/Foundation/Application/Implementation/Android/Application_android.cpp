#include <FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_ANDROID)

#include <Foundation/Application/Application.h>
#include <Foundation/Application/Implementation/Android/Application_android.h>
#  include <android/log.h>
#  include <android_native_app_glue.h>

static void ezAndroidHandleCmd(struct android_app* pApp, int32_t cmd)
{
  ezAndroidApplication* pAndroidApp = static_cast<ezAndroidApplication*>(pApp->userData);
  pAndroidApp->HandleCmd(cmd);
}

static int32_t ezAndroidHandleInput(struct android_app* pApp, AInputEvent* pEvent)
{
  ezAndroidApplication* pAndroidApp = static_cast<ezAndroidApplication*>(pApp->userData);
  return pAndroidApp->HandleInput(pEvent);
}

ezAndroidApplication::ezAndroidApplication(struct android_app* pApp, ezApplication* pEzApp)
  : m_pApp(pApp)
  , m_pEzApp(pEzApp)
{
  pApp->userData = this;
  pApp->onAppCmd = ezAndroidHandleCmd;
  pApp->onInputEvent = ezAndroidHandleInput;
  //#TODO: acquire sensors, set app->onAppCmd, set app->onInputEvent
}

ezAndroidApplication::~ezAndroidApplication()
{
}

void ezAndroidApplication::AndroidRun()
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

      HandleIdent(iIdent);
    }
    if (bRun && m_pEzApp->Run() != ezApplication::Continue)
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

void ezAndroidApplication::HandleCmd(int32_t cmd)
{
  //#TODO:
}

int32_t ezAndroidApplication::HandleInput(AInputEvent* pEvent)
{
  //#TODO:
  return 0;
}

void ezAndroidApplication::HandleIdent(ezInt32 iIdent)
{
  //#TODO:
}

EZ_FOUNDATION_DLL void ezAndroidRun(struct android_app* pApp, ezApplication* pEzApp)
{
  ezAndroidApplication androidApp(pApp, pEzApp);

  if (ezRun_Startup(pEzApp).Succeeded())
  {
    androidApp.AndroidRun();
  }
  ezRun_Shutdown(pEzApp);

  const int iReturnCode = pEzApp->GetReturnCode();
  if (iReturnCode != 0)
  {
    const char* szReturnCode = pEzApp->TranslateReturnCode();
    if (szReturnCode != nullptr && szReturnCode[0] != '\0')
      __android_log_print(ANDROID_LOG_ERROR, "ezEngine", "Return Code: '%s'", szReturnCode);
  }
}

#endif

EZ_STATICLINK_FILE(Foundation, Foundation_Application_Implementation_Android_Application_android);

