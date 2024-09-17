#pragma once

#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Platform/Web/EmscriptenUtils.h>

/// \file


/// \brief Same as EZ_APPLICATION_ENTRY_POINT but should be used for applications that shall always show a console window.
#define EZ_CONSOLEAPP_ENTRY_POINT EZ_APPLICATION_ENTRY_POINT

/// \brief This macro allows for easy creation of application entry points (since they can't be placed in DLLs)
///
/// Just use the macro in a cpp file of your application and supply your app class (must be derived from ezApplication).
/// The additional (optional) parameters are passed to the constructor of your app class.
#define EZ_APPLICATION_ENTRY_POINT(AppClass, ...)                                                                                \
  alignas(EZ_ALIGNMENT_OF(AppClass)) static char appBuffer[sizeof(AppClass)]; /* Not on the stack to cope with smaller stacks */ \
                                                                                                                                 \
  EZ_APPLICATION_ENTRY_POINT_CODE_INJECTION                                                                                      \
                                                                                                                                 \
  AppClass* g_pApp = nullptr;                                                                                                    \
  ezInt32 g_iAppState = 0;                                                                                                       \
                                                                                                                                 \
  void ezWebRun()                                                                                                                \
  {                                                                                                                              \
    switch (g_iAppState)                                                                                                         \
    {                                                                                                                            \
      case 0:                                                                                                                    \
      {                                                                                                                          \
        bool bPreInitDone = true;                                                                                                \
        EZ_BROADCAST_EVENT(WebApp_PreInit, &bPreInitDone);                                                                       \
        if (bPreInitDone)                                                                                                        \
          g_iAppState = 1;                                                                                                       \
        break;                                                                                                                   \
      }                                                                                                                          \
      case 1:                                                                                                                    \
        ezRun_Startup(g_pApp).AssertSuccess();                                                                                   \
        g_iAppState = 2;                                                                                                         \
        break;                                                                                                                   \
      case 2:                                                                                                                    \
        g_pApp->Run();                                                                                                           \
        break;                                                                                                                   \
    }                                                                                                                            \
  }                                                                                                                              \
                                                                                                                                 \
  int main(int argc, const char** argv)                                                                                          \
  {                                                                                                                              \
    g_pApp = new (appBuffer) AppClass(__VA_ARGS__);                                                                              \
    g_pApp->SetCommandLineArguments((ezUInt32)argc, argv);                                                                       \
    ezEmscriptenUtils::SetMainLoopFunction(ezWebRun);                                                                            \
    /* There's no real 'shutdown' in web. Just ignore this. */                                                                   \
    return 0;                                                                                                                    \
  }
