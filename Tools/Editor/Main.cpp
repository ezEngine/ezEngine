#include <PCH.h>
#include <Core/Application/Application.h>
#include <EditorFramework/EditorFramework.h>
#include <QApplication>
#include <QSettings>

class ezEditorApp : public ezApplication
{
public:

  virtual ApplicationExecution Run() override
  {
    ezEditorFramework::StartupEditor("ezEditor", "DefaultUser", GetArgumentCount(), (char**) GetArgumentsArray());

    {
      const ezInt32 iReturnCode = ezEditorFramework::RunEditor();

      SetReturnCode(iReturnCode);

      ezEditorFramework::ShutdownEditor();
    }

    return ezApplication::Quit;
  }
};

EZ_APPLICATION_ENTRY_POINT(ezEditorApp);

