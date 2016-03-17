#include <PCH.h>
#include <Core/Application/Application.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <QApplication>
#include <QSettings>
#include <QtNetwork/QHostInfo>
#include <GuiFoundation/UIServices/ImageCache.moc.h>

class ezEditorApplication : public ezApplication
{
public:
  ezEditorApplication()
  {
    EnableMemoryLeakReporting(true);

    m_pEditorApp = new ezQtEditorApp;
  }

  virtual void BeforeCoreStartup() override
  {
    ezQtEditorApp::GetSingleton()->InitQt(GetArgumentCount(), (char**)GetArgumentsArray());
  }

  virtual void AfterCoreShutdown() override
  {
    ezQtEditorApp::GetSingleton()->DeInitQt();

    delete m_pEditorApp;
    m_pEditorApp = nullptr;
  }

  virtual ApplicationExecution Run() override
  {
    QHostInfo hostInfo;
    hostInfo = QHostInfo::fromName(QHostInfo::localHostName());
    ezString sHostName = QHostInfo::localHostName().toUtf8().data();

    ezQtEditorApp::GetSingleton()->StartupEditor("ezEditor", sHostName);
    {
      const ezInt32 iReturnCode = ezQtEditorApp::GetSingleton()->RunEditor();
      SetReturnCode(iReturnCode);
    }
    ezQtEditorApp::GetSingleton()->ShutdownEditor();

    return ezApplication::Quit;
  }

private:
  ezQtEditorApp* m_pEditorApp;
};

EZ_APPLICATION_ENTRY_POINT(ezEditorApplication);

