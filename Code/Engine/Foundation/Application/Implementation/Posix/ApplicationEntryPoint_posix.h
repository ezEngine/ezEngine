
#pragma once

/// \file


/// \brief Same as EZ_APPLICATION_ENTRY_POINT but should be used for applications that shall always show a console window.
#define EZ_CONSOLEAPP_ENTRY_POINT EZ_APPLICATION_ENTRY_POINT


/// \brief This macro allows for easy creation of application entry points (since they can't be placed in DLLs)
///
/// Just use the macro in a cpp file of your application and supply your app class (must be derived from ezApplication).
/// The additional (optional) parameters are passed to the constructor of your app class.
#define EZ_APPLICATION_ENTRY_POINT(AppClass, ...)                                                                                           \
  EZ_ALIGN_VARIABLE(static char appBuffer[sizeof(AppClass)], EZ_ALIGNMENT_OF(AppClass)); /* Not on the stack to cope with smaller stacks */ \
                                                                                                                                            \
  int main(int argc, const char** argv)                                                                                                     \
  {                                                                                                                                         \
                                                                                                                                            \
    AppClass* pApp = new (appBuffer) AppClass(__VA_ARGS__);                                                                                 \
    pApp->SetCommandLineArguments((ezUInt32)argc, argv);                                                                                    \
    ezRun(pApp); /* Life cycle & run method calling */                                                                                      \
    const int iReturnCode = pApp->GetReturnCode();                                                                                          \
    if (iReturnCode != 0)                                                                                                                   \
    {                                                                                                                                       \
      const char* szReturnCode = pApp->TranslateReturnCode();                                                                               \
      if (szReturnCode != nullptr && szReturnCode[0] != '\0')                                                                               \
        ezLog::Printf("Return Code: '%s'\n", szReturnCode);                                                                                 \
    }                                                                                                                                       \
    pApp->~AppClass();                                                                                                                      \
    memset(pApp, 0, sizeof(AppClass));                                                                                                      \
    return iReturnCode;                                                                                                                     \
  }
