#pragma once

#include <Foundation/Basics/Platform/Android/AndroidUtils.h>
#include <android/log.h>
#include <android_native_app_glue.h>

/// \file


/// \brief Same as EZ_APPLICATION_ENTRY_POINT but should be used for applications that shall always show a console window.
#define EZ_CONSOLEAPP_ENTRY_POINT EZ_APPLICATION_ENTRY_POINT


/// \brief This macro allows for easy creation of application entry points (since they can't be placed in DLLs)
///
/// Just use the macro in a cpp file of your application and supply your app class (must be derived from ezApplication).
/// The additional (optional) parameters are passed to the constructor of your app class.
#define EZ_APPLICATION_ENTRY_POINT(AppClass, ...)                                              \
  static char appBuffer[sizeof(AppClass)]; /* Not on the stack to cope with smaller stacks */  \
                                                                                               \
  extern "C" void android_main(struct android_app* app)                                        \
  {                                                                                            \
    ezAndroidUtils::SetAndroidApp(app);                                                        \
    AppClass* pApp = new (appBuffer) AppClass(__VA_ARGS__);                                    \
    ezRun(pApp); /* Life cycle & run method calling */                                         \
    const int iReturnCode = pApp->GetReturnCode();                                             \
    if (iReturnCode != 0)                                                                      \
    {                                                                                          \
      const char* szReturnCode = pApp->TranslateReturnCode();                                  \
      if (szReturnCode != nullptr && szReturnCode[0] != '\0')                                  \
        __android_log_print(ANDROID_LOG_ERROR, "ezEngine", "Return Code: '%s'", szReturnCode); \
    }                                                                                          \
    pApp->~AppClass();                                                                         \
    memset(pApp, 0, sizeof(AppClass));                                                         \
    /* TODO: do something with iReturnCode? */                                                 \
  }
