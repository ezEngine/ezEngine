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
bool ezEditorFramework::s_bContentModified = false;

ezEvent<const ezEditorFramework::EditorEvent&> ezEditorFramework::s_EditorEvents;

QMainWindow* ezEditorFramework::s_pMainWindow = nullptr;

ezSet<ezString> ezEditorFramework::s_RestartRequiredReasons;

void ezEditorFramework::StartupEditor(const ezString& sAppName)
{
  s_sApplicationName = sAppName;
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

  if (!s_sScenePath.IsEmpty())
  {
    sTitle.Append(" - ", s_sScenePath.GetData());
  }
  else
  if (!s_sProjectPath.IsEmpty())
  {
    sTitle.Append(" - ", s_sProjectPath.GetData());
  }

  if (s_bContentModified)
  {
    sTitle.Append("*");
  }

  if (s_pMainWindow)
    s_pMainWindow->setWindowTitle(QString::fromUtf8(sTitle.GetData()));
}

void ezEditorFramework::SaveWindowLayout()
{
  if (!s_pMainWindow)
    return;

  const bool bMaximized = s_pMainWindow->isMaximized();

  if (bMaximized)
    s_pMainWindow->showNormal();

  QSettings Settings;
  Settings.beginGroup("EditorMainWnd");
  {
    Settings.setValue("WindowGeometry", s_pMainWindow->saveGeometry());
    Settings.setValue("WindowState", s_pMainWindow->saveState());
    Settings.setValue("IsMaximized", bMaximized);
    Settings.setValue("WindowPosition", s_pMainWindow->pos());

    if (!bMaximized)
      Settings.setValue("WindowSize", s_pMainWindow->size());
  }
  Settings.endGroup();
}

void ezEditorFramework::RestoreWindowLayout()
{
  if (!s_pMainWindow)
    return;

  QSettings Settings;
  Settings.beginGroup("EditorMainWnd");
  {
    s_pMainWindow->restoreGeometry(Settings.value("WindowGeometry", s_pMainWindow->saveGeometry()).toByteArray());

    s_pMainWindow->move(Settings.value("WindowPosition", s_pMainWindow->pos()).toPoint());
    s_pMainWindow->resize(Settings.value("WindowSize", s_pMainWindow->size()).toSize());

    if (Settings.value("IsMaximized", s_pMainWindow->isMaximized()).toBool())
      s_pMainWindow->showMaximized();

    s_pMainWindow->restoreState(Settings.value("WindowState", s_pMainWindow->saveState()).toByteArray());
  }
  Settings.endGroup();
}