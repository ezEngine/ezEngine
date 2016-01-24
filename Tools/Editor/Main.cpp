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
    ezQtEditorApp::GetInstance()->InitQt(GetArgumentCount(), (char**)GetArgumentsArray());
  }

  virtual void AfterCoreShutdown() override
  {
    ezQtEditorApp::GetInstance()->DeInitQt();

    delete m_pEditorApp;
    m_pEditorApp = nullptr;
  }

  virtual ApplicationExecution Run() override
  {
    QHostInfo hostInfo;
    hostInfo = QHostInfo::fromName(QHostInfo::localHostName());
    ezString sHostName = QHostInfo::localHostName().toUtf8().data();

    ezQtEditorApp::GetInstance()->StartupEditor("ezEditor", sHostName);
    {
      const ezInt32 iReturnCode = ezQtEditorApp::GetInstance()->RunEditor();
      SetReturnCode(iReturnCode);
    }
    ezQtEditorApp::GetInstance()->ShutdownEditor();

    return ezApplication::Quit;
  }

private:
  ezQtEditorApp* m_pEditorApp;
};

EZ_APPLICATION_ENTRY_POINT(ezEditorApplication);

