#include <Editor/EditorPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Utilities/CommandLineOptions.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#  include <shellscalingapi.h>
#endif

namespace
{
  /// Writes text to the console of the process that launched this one, if there is one.
  ///
  /// The editor is built as a window application, so it has no console of its own and printf goes
  /// nowhere by default. Attaching to the parent's console is what makes the output visible when it
  /// was started from a shell. When there is no console - the double-clicked case - the text is
  /// silently dropped, because there is nowhere to put it that would not block waiting for a click.
  void PrintToParentConsole(ezStringView sText)
  {
    const ezStringBuilder sOut(sText);

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
    // Fails when the parent has no console, and also when one is already attached - the latter is not
    // an error, that console can be written to just as well, so only the handle below decides.
    ::AttachConsole(ATTACH_PARENT_PROCESS);

    const HANDLE hOut = ::GetStdHandle(STD_OUTPUT_HANDLE);

    if (hOut == nullptr || hOut == INVALID_HANDLE_VALUE)
      return;

    // written through the handle rather than printf, because the CRT's stdout is not connected to a
    // console that was only attached just now
    DWORD uiWritten = 0;
    ::WriteFile(hOut, sOut.GetData(), sOut.GetElementCount(), &uiWritten, nullptr);
#else
    // every other platform this builds for has a normal stdout
    printf("%s", sOut.GetData());
    fflush(stdout);
#endif
  }
} // namespace

class ezEditorApplication : public ezApplication
{
public:
  using SUPER = ezApplication;

  ezEditorApplication()
    : ezApplication("ezEditor")
  {
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
#endif
    EnableMemoryLeakReporting(true);

    m_pEditorApp = new ezQtEditorApp;
  }

  virtual ezResult BeforeCoreSystemsStartup() override
  {
    ezStartup::AddApplicationTag("tool");
    ezStartup::AddApplicationTag("editor");
    ezStartup::AddApplicationTag("editorapp");

    ezQtEditorApp::GetSingleton()->InitQt(GetArgumentCount(), (char**)GetArgumentsArray());

    return EZ_SUCCESS;
  }

  virtual void AfterCoreSystemsShutdown() override
  {
    ezQtEditorApp::GetSingleton()->DeInitQt();

    delete m_pEditorApp;
    m_pEditorApp = nullptr;
  }

  virtual void Run() override
  {
    {
      ezStringBuilder cmdHelp;
      if (ezCommandLineOption::LogAvailableOptionsToBuffer(cmdHelp, ezCommandLineOption::LogAvailableModes::IfHelpRequested, "_Editor;cvar"))
      {
        PrintToParentConsole(cmdHelp);

        QuitApplication();
        return;
      }
    }

    ezQtEditorApp::GetSingleton()->StartupEditor();
    {
      const ezInt32 iReturnCode = ezQtEditorApp::GetSingleton()->RunEditor();
      SetReturnCode(iReturnCode);
    }
    ezQtEditorApp::GetSingleton()->ShutdownEditor();

    QuitApplication();
  }

private:
  ezQtEditorApp* m_pEditorApp;
};

EZ_APPLICATION_ENTRY_POINT(ezEditorApplication);
