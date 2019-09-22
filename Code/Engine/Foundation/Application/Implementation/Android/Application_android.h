#pragma once

#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

#if EZ_ENABLED(EZ_PLATFORM_ANDROID)

#include <Foundation/Basics.h>
#include <Foundation/Strings/String.h>

class ezApplication;
struct AInputEvent;

class ezAndroidApplication
{
public:
  ezAndroidApplication(struct android_app* pApp, ezApplication* pEzApp);
  ~ezAndroidApplication();
  void AndroidRun();
  void HandleCmd(int32_t cmd);
  int32_t HandleInput(AInputEvent* pEvent);
  void HandleIdent(ezInt32 iIdent);

private:
  struct android_app* m_pApp;
  ezApplication* m_pEzApp;
};

#endif

