#include <PCH.h>
#include <Core/Application/Application.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <QApplication>
#include <QSettings>
#include <QtNetwork/QHostInfo>
#include <GuiFoundation/UIServices/ImageCache.moc.h>

static ezQtEditorApp g_EditorApp;

class ezEditorApplication : public ezApplication
{
public:
  ezEditorApplication()
  {
    EnableMemoryLeakReporting(true);
  }

  virtual void BeforeEngineInit() override
  {
    ezQtEditorApp::GetInstance()->InitQt(GetArgumentCount(), (char**)GetArgumentsArray());
  }

  virtual void AfterEngineShutdown() override
  {
    ezQtEditorApp::GetInstance()->DeInitQt();
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
};

EZ_APPLICATION_ENTRY_POINT(ezEditorApplication);

