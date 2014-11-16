#include <PCH.h>
#include <Core/Application/Application.h>
#include <EditorFramework/EditorFramework.h>
#include <QApplication>
#include <QSettings>
#include <QtNetwork/QHostInfo>

class ezEditorApp : public ezApplication
{
public:
  ezEditorApp()
  {
    EnableMemoryLeakReporting(false);
  }

  virtual ApplicationExecution Run() override
  {
    QHostInfo hostInfo;
    hostInfo = QHostInfo::fromName(QHostInfo::localHostName());
    ezString sHostName = QHostInfo::localHostName().toUtf8().data();

    ezEditorFramework::StartupEditor("ezEditor", sHostName, GetArgumentCount(), (char**) GetArgumentsArray());

    {
      const ezInt32 iReturnCode = ezEditorFramework::RunEditor();

      SetReturnCode(iReturnCode);

      ezEditorFramework::ShutdownEditor();
    }

    return ezApplication::Quit;
  }
};

EZ_APPLICATION_ENTRY_POINT(ezEditorApp);

