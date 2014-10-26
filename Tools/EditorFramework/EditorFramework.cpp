#include <PCH.h>
#include <EditorFramework/EditorFramework.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/JSONWriter.h>
#include <Foundation/IO/JSONReader.h>
#include <QMainWindow>
#include <QSettings>

ezString ezEditorFramework::s_sApplicationName("ezEditor");
ezString ezEditorFramework::s_sUserName("DefaultUser");
bool ezEditorFramework::s_bContentModified = false;

ezEvent<const ezEditorFramework::EditorEvent&> ezEditorFramework::s_EditorEvents;

ezEvent<ezEditorFramework::EditorRequest&> ezEditorFramework::s_EditorRequests;

ezSet<ezString> ezEditorFramework::s_RestartRequiredReasons;

void ezEditorFramework::StartupEditor(ezStringView sAppName, ezStringView sUserName)
{
  s_sApplicationName = sAppName;
  s_sUserName = sUserName;
  UpdateEditorWindowTitle();

  // load the settings
  GetSettings(SettingsCategory::Editor);
}

void ezEditorFramework::ShutdownEditor()
{
  SaveSettings();

  CloseProject();
}

void ezEditorFramework::UpdateEditorWindowTitle()
{
  ezStringBuilder sTitle = s_sApplicationName;

  //if (!s_sScenePath.IsEmpty())
  //{
  //  sTitle.Append(" - ", s_sScenePath.GetData());
  //}
  //else
  if (!s_sProjectPath.IsEmpty())
  {
    sTitle.Append(" - ", s_sProjectPath.GetData());
  }

  if (s_bContentModified)
  {
    sTitle.Append("*");
  }

  //if (s_pMainWindow)
  //  s_pMainWindow->setWindowTitle(QString::fromUtf8(sTitle.GetData()));
}
