
#pragma once

  /// \brief This macro allows for easy creation of application entry points (since they can't be placed in DLLs)
  ///
  /// Just use the macro in a cpp file of your application and supply your app class (must be derived from ezApplication).
  /// The additional (optional) parameters are passed to the constructor of your app class.
  #define EZ_ENTRY_POINT(AppClass, ...) \
    static char appBuffer[sizeof(AppClass)]; /* Not on the stack to cope with smaller stacks */ \
    \
    int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) \
    { \
      \
      AppClass* pApp = new (appBuffer) AppClass(__VA_ARGS__); \
      pApp->SetCommandLineArguments(__argc, const_cast<const char**>(__argv)); \
      ezRun(pApp); /* Life cycle & run method calling */ \
      int iReturnCode = pApp->GetReturnCode(); \
      pApp->~AppClass(); \
      memset(pApp, 0, sizeof(AppClass));  /* Debug only?*/ \
      return iReturnCode; \
    }

