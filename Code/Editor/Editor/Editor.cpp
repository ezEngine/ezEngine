#include <EditorPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <QApplication>
#include <QSettings>
#include <QtNetwork/QHostInfo>
#include <corecrt_io.h>
#include <fcntl.h>

class ezEditorApplication : public ezApplication
{
public:
  typedef ezApplication SUPER;

  ezEditorApplication()
    : ezApplication("ezEditor")
  {
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

  virtual Execution Run() override
  {
    {
      ezStringBuilder cmdHelp;
      if (ezCommandLineOption::LogAvailableOptionsToBuffer(cmdHelp, ezCommandLineOption::LogAvailableModes::IfHelpRequested, "_Editor;cvar"))
      {
        ezQtUiServices::GetSingleton()->MessageBoxInformation(cmdHelp);
        return ezApplication::Execution::Quit;
      }
    }

    ezQtEditorApp::GetSingleton()->StartupEditor();
    {
      const ezInt32 iReturnCode = ezQtEditorApp::GetSingleton()->RunEditor();
      SetReturnCode(iReturnCode);
    }
    ezQtEditorApp::GetSingleton()->ShutdownEditor();

    return ezApplication::Execution::Quit;
  }

private:
  ezQtEditorApp* m_pEditorApp;
};

EZ_APPLICATION_ENTRY_POINT(ezEditorApplication);
