
#pragma once

/// \file

#include <Foundation/Memory/MemoryTracker.h>

namespace Details
{
  template<typename AppClass, typename ...Args>
  int ConsoleEntry(int argc, const char** argv, Args&&... arguments)
  {
    static char appBuffer[sizeof(AppClass)]; // Not on the stack to cope with smaller stacks.

    AppClass* pApp = new (appBuffer) AppClass(std::forward<Args>(arguments)...);
    pApp->SetCommandLineArguments((ezUInt32)argc, argv);
    ezRun(pApp); // Life cycle & run method calling

    const int iReturnCode = pApp->GetReturnCode();
    if (iReturnCode != 0)
    {
      std::string text = pApp->TranslateReturnCode();
      if (!text.empty())
        printf("Return Code: '%s'\n", text.c_str());
    }

    const bool memLeaks = pApp->IsMemoryLeakReportingEnabled();
    pApp->~AppClass();
    memset(pApp, 0, sizeof(AppClass));
    if (memLeaks)
      ezMemoryTracker::DumpMemoryLeaks();

    return iReturnCode;
  }

  template<typename AppClass, typename ...Args>
  int ApplicationEntry(Args&&... arguments)
  {
    static char appBuffer[sizeof(AppClass)]; // Not on the stack to cope with smaller stacks.

    AppClass* pApp = new (appBuffer) AppClass(std::forward<Args>(arguments)...);
    pApp->SetCommandLineArguments((ezUInt32)__argc, const_cast<const char**>(__argv));
    ezRun(pApp); //Life cycle & run method calling

    const int iReturnCode = pApp->GetReturnCode();
    if (iReturnCode != 0)
    {
      std::string text = pApp->TranslateReturnCode(); \
        if (!text.empty())
          printf("Return Code: '%s'\n", text.c_str());
    }

    const bool memLeaks = pApp->IsMemoryLeakReportingEnabled();
    pApp->~AppClass();
    memset(pApp, 0, sizeof(AppClass));
    if (memLeaks)
      ezMemoryTracker::DumpMemoryLeaks();

    return iReturnCode;
  }
}

/// \brief Same as EZ_APPLICATION_ENTRY_POINT but should be used for applications that shall always show a console window.
#define EZ_CONSOLEAPP_ENTRY_POINT(AppClass, ...)                                          \
  /* Enables that on machines with multiple GPUs the NVIDIA GPU is preferred */           \
  extern "C" { _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001; }             \
  int main(int argc, const char** argv)                                                   \
  {                                                                                       \
    return Details::ConsoleEntry<AppClass>(argc, argv, __VA_ARGS__);                      \
  }

/// \brief This macro allows for easy creation of application entry points (since they can't be placed in DLLs)
///
/// Just use the macro in a cpp file of your application and supply your app class (must be derived from ezApplication).
/// The additional (optional) parameters are passed to the constructor of your app class.
#define EZ_APPLICATION_ENTRY_POINT(AppClass, ...)                                                   \
  /* Enables that on machines with multiple GPUs the NVIDIA GPU is preferred */                     \
  extern "C" { _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001; }                       \
  int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) \
  {                                                                                                 \
    return Details::ApplicationEntry<AppClass>(__VA_ARGS__);                                        \
  }
