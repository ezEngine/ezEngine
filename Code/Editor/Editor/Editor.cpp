#include <Editor/EditorPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Utilities/CommandLineOptions.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
#  include <shellscalingapi.h>
#endif

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
        ezQtUiServices::GetSingleton()->MessageBoxInformation(cmdHelp);
        RequestApplicationQuit();
        return;
      }
    }

    ezQtEditorApp::GetSingleton()->StartupEditor();
    {
      const ezInt32 iReturnCode = ezQtEditorApp::GetSingleton()->RunEditor();
      SetReturnCode(iReturnCode);
    }
    ezQtEditorApp::GetSingleton()->ShutdownEditor();

    RequestApplicationQuit();
  }

private:
  ezQtEditorApp* m_pEditorApp;
};

EZ_APPLICATION_ENTRY_POINT(ezEditorApplication);
