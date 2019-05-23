
#pragma once

/// \file

#include <Foundation/Memory/MemoryTracker.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Threading/Lock.h>

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

  template <typename AppClass, typename... Args>
  int ApplicationEntry(Args&&... arguments)
  {
    static char appBuffer[sizeof(AppClass)]; // Not on the stack to cope with smaller stacks.

    // This mutex will prevent the console shutdown handler to return
    // as long as this entry point is not finished executing
    // (see consoleHandler below).
    static ezMutex shutdownMutex;
    EZ_LOCK(shutdownMutex);

    // We're attaching to a potential console parent process
    // and ignore failure when no such parent is present
    if (AttachConsole(ATTACH_PARENT_PROCESS) == TRUE)
    {
      // This handler overrides the default handler (which would
      // call ExitProcess which leads to unorderly engine shutdowns)
      const auto consoleHandler = [](_In_ DWORD dwCtrlType) -> BOOL
      {
        // We have to wait until the application has shut down orderly
        // since Windows will kill everything after this handler returns
        reinterpret_cast<AppClass*>(appBuffer)->RequestQuit();
        EZ_LOCK(shutdownMutex);
        return TRUE;
      };
      SetConsoleCtrlHandler(consoleHandler, TRUE);

      // this is necessary to direct the standard
      // output to the newly attached console
      freopen("CONOUT$", "w", stdout);
      freopen("CONOUT$", "w", stderr);
      freopen("CONIN$", "r", stdin);
      printf("\n");
    }

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

    if (GetConsoleWindow())
    {
      // Since the console already printed a new prompt the input pointer ends up in the blanks after
      // our output and no new prompt appears. Sending an enter keypress works around that limitation.
      INPUT ip = {};

      ip.type = INPUT_KEYBOARD;
      // key code for enter
      ip.ki.wVk = 0x0D;
      SendInput(1, &ip, sizeof(INPUT));

      ip.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
      SendInput(1, &ip, sizeof(INPUT));
    }

    return iReturnCode;
  }
}

/// \brief Same as EZ_APPLICATION_ENTRY_POINT but should be used for applications that shall always show a console window.
#define EZ_CONSOLEAPP_ENTRY_POINT(AppClass, ...)                                                                                           \
  /* Enables that on machines with multiple GPUs the NVIDIA GPU is preferred */                                                            \
  extern "C" {                                                                                                                             \
  _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;                                                                             \
  }                                                                                                                                        \
  int main(int argc, const char** argv) { return ezApplicationDetails::ConsoleEntry<AppClass>(argc, argv, __VA_ARGS__); }

/// \brief This macro allows for easy creation of application entry points (since they can't be placed in DLLs)
///
/// Just use the macro in a cpp file of your application and supply your app class (must be derived from ezApplication).
/// The additional (optional) parameters are passed to the constructor of your app class.
#define EZ_APPLICATION_ENTRY_POINT(AppClass, ...)                                                                                          \
  /* Enables that on machines with multiple GPUs the NVIDIA GPU is preferred */                                                            \
  extern "C" {                                                                                                                             \
  _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;                                                                             \
  }                                                                                                                                        \
  int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)                                        \
  {                                                                                                                                        \
    return ezApplicationDetails::ApplicationEntry<AppClass>(__VA_ARGS__);                                                                  \
  }

