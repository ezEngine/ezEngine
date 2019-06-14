
#pragma once

/// \file

#include <Foundation/Basics/Platform/Win/MinWindows.h>
#include <Foundation/Memory/MemoryTracker.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>

namespace ezApplicationDetails
{
  template <typename AppClass, typename... Args>
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

  EZ_FOUNDATION_DLL void AttachToConsoleWindow(ezMinWindows::BOOL (EZ_WINDOWS_WINAPI *consoleHandler)(ezMinWindows::DWORD dwCtrlType));
  EZ_FOUNDATION_DLL void DetachFromConsoleWindow();

  template <typename AppClass, typename... Args>
  int ApplicationEntry(Args&&... arguments)
  {
    static char appBuffer[sizeof(AppClass)]; // Not on the stack to cope with smaller stacks.

    // This mutex will prevent the console shutdown handler to return
    // as long as this entry point is not finished executing
    // (see consoleHandler below).
    static ezMutex shutdownMutex;
    EZ_LOCK(shutdownMutex);

    // This handler overrides the default handler (which would
    // call ExitProcess which leads to unorderly engine shutdowns)
    const auto consoleHandler = [](_In_ ezMinWindows::DWORD dwCtrlType) -> ezMinWindows::BOOL {
      // We have to wait until the application has shut down orderly
      // since Windows will kill everything after this handler returns
      reinterpret_cast<AppClass*>(appBuffer)->RequestQuit();
      EZ_LOCK(shutdownMutex);
      return 1;
    };
    AttachToConsoleWindow(consoleHandler);

    AppClass* pApp = new (appBuffer) AppClass(std::forward<Args>(arguments)...);
    pApp->SetCommandLineArguments((ezUInt32)__argc, const_cast<const char**>(__argv));
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
} // namespace ezApplicationDetails

/// \brief Same as EZ_APPLICATION_ENTRY_POINT but should be used for applications that shall always show a console window.
#define EZ_CONSOLEAPP_ENTRY_POINT(AppClass, ...)                                                                                           \
  /* Enables that on machines with multiple GPUs the NVIDIA GPU is preferred */                                                            \
  extern "C"                                                                                                                               \
  {                                                                                                                                        \
    _declspec(dllexport) ezMinWindows::DWORD NvOptimusEnablement = 0x00000001;                                                             \
  }                                                                                                                                        \
  int main(int argc, const char** argv) { return ezApplicationDetails::ConsoleEntry<AppClass>(argc, argv, __VA_ARGS__); }

// If windows.h is already included use the native types, otherwise use types from ezMinWindows
#ifdef _WINDOWS_
#  define _EZ_APPLICATION_ENTRY_POINT_HINSTANCE HINSTANCE
#  define _EZ_APPLICATION_ENTRY_POINT_LPSTR LPSTR
#else
#  define _EZ_APPLICATION_ENTRY_POINT_HINSTANCE ezMinWindows::HINSTANCE
#  define _EZ_APPLICATION_ENTRY_POINT_LPSTR ezMinWindows::LPSTR
#endif

/// \brief This macro allows for easy creation of application entry points (since they can't be placed in DLLs)
///
/// Just use the macro in a cpp file of your application and supply your app class (must be derived from ezApplication).
/// The additional (optional) parameters are passed to the constructor of your app class.
#define EZ_APPLICATION_ENTRY_POINT(AppClass, ...)                                                                                          \
  /* Enables that on machines with multiple GPUs the NVIDIA GPU is preferred */                                                            \
  extern "C"                                                                                                                               \
  {                                                                                                                                        \
    _declspec(dllexport) ezMinWindows::DWORD NvOptimusEnablement = 0x00000001;                                                             \
  }                                                                                                                                        \
  int EZ_WINDOWS_CALLBACK WinMain(_EZ_APPLICATION_ENTRY_POINT_HINSTANCE hInstance, _EZ_APPLICATION_ENTRY_POINT_HINSTANCE hPrevInstance,    \
    _EZ_APPLICATION_ENTRY_POINT_LPSTR lpCmdLine, int nCmdShow)                                                                             \
  {                                                                                                                                        \
    return ezApplicationDetails::ApplicationEntry<AppClass>(__VA_ARGS__);                                                                  \
  }
