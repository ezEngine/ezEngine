#include <PCH.h>
#include <Core/Application/Application.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <QApplication>
#include <QSettings>
#include <QtNetwork/QHostInfo>
#include <GuiFoundation/UIServices/ImageCache.moc.h>

static ezEditorApp g_EditorApp;

class ezEditorApplication : public ezApplication
{
public:
  ezEditorApplication()
  {
    EnableMemoryLeakReporting(true);
  }

  virtual ApplicationExecution Run() override
  {
    QHostInfo hostInfo;
    hostInfo = QHostInfo::fromName(QHostInfo::localHostName());
    ezString sHostName = QHostInfo::localHostName().toUtf8().data();

    ezEditorApp::GetInstance()->StartupEditor("ezEditor", sHostName, GetArgumentCount(), (char**) GetArgumentsArray());

    {
      const ezInt32 iReturnCode = ezEditorApp::GetInstance()->RunEditor();

      SetReturnCode(iReturnCode);

      ezEditorApp::GetInstance()->ShutdownEditor();
    }

    return ezApplication::Quit;
  }
};

EZ_APPLICATION_ENTRY_POINT(ezEditorApplication);

