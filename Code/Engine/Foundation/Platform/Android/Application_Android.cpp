#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_ANDROID)

#  include <Foundation/Application/Application.h>
#  include <Foundation/Logging/Log.h>
#  include <Foundation/Platform/Android/Application_Android.h>
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
  // #TODO: acquire sensors, set app->onAppCmd, set app->onInputEvent
}

ezAndroidApplication::~ezAndroidApplication() {}

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

    // APP_CMD_INIT_WINDOW has not triggered yet. Engine is not yet started.
    if (!m_bStarted)
      continue;

    if (bRun)
    {
      m_pEzApp->Run();

      if (m_pEzApp->ShouldApplicationQuit())
      {
        bRun = false;
        ANativeActivity_finish(m_pApp->activity);
      }
    }

    if (m_pApp->destroyRequested)
    {
      break;
    }
  }
}

void ezAndroidApplication::HandleCmd(int32_t cmd)
{
  switch (cmd)
  {
    case APP_CMD_INIT_WINDOW:
      if (m_pApp->window != nullptr)
      {
        EZ_VERIFY(ezRun_Startup(m_pEzApp).Succeeded(), "Failed to startup engine");
        m_bStarted = true;

        int width = ANativeWindow_getWidth(m_pApp->window);
        int height = ANativeWindow_getHeight(m_pApp->window);
        ezLog::Info("Init Window: {}x{}", width, height);
      }
      break;
    case APP_CMD_TERM_WINDOW:
      m_pEzApp->RequestApplicationQuit();
      break;
    default:
      break;
  }
  ezAndroidUtils::s_AppCommandEvent.Broadcast(cmd);
}

int32_t ezAndroidApplication::HandleInput(AInputEvent* pEvent)
{
  ezAndroidInputEvent event;
  event.m_pEvent = pEvent;
  event.m_bHandled = false;

  ezAndroidUtils::s_InputEvent.Broadcast(event);
  return event.m_bHandled ? 1 : 0;
}

void ezAndroidApplication::HandleIdent(ezInt32 iIdent)
{
  // #TODO:
}

EZ_FOUNDATION_DLL void ezAndroidRun(struct android_app* pApp, ezApplication* pEzApp)
{
  ezAndroidApplication androidApp(pApp, pEzApp);

  // This call will loop until APP_CMD_INIT_WINDOW is emitted which triggers ezRun_Startup
  androidApp.AndroidRun();

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
