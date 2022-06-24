#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>

void ezQtEditorApp::AddRestartRequiredReason(const char* szReason)
{
  if (!s_RestartRequiredReasons.Find(szReason).IsValid())
  {
    s_RestartRequiredReasons.Insert(szReason);
    UpdateGlobalStatusBarMessage();
  }

  ezStringBuilder s;
  s.Format("The editor process must be restarted.\nReason: '{0}'\n\nDo you want to restart now?", szReason);

  if (ezQtUiServices::MessageBoxQuestion(s, QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::Yes) == QMessageBox::StandardButton::Yes)
  {
    if (ezToolsProject::CanCloseProject())
    {
      LaunchEditor(ezToolsProject::GetSingleton()->GetProjectFile());

      QApplication::closeAllWindows();
      return;
    }
  }
}

void ezQtEditorApp::AddReloadProjectRequiredReason(const char* szReason)
{
  if (!s_ReloadProjectRequiredReasons.Find(szReason).IsValid())
  {
    s_ReloadProjectRequiredReasons.Insert(szReason);

    ezStringBuilder s;
    s.Format("The project must be reloaded.\nReason: '{0}'", szReason);

    ezQtUiServices::MessageBoxInformation(s);

    UpdateGlobalStatusBarMessage();
  }
}

void ezQtEditorApp::UpdateGlobalStatusBarMessage()
{
  ezStringBuilder sText;

  if (!s_RestartRequiredReasons.IsEmpty())
    sText.Append("Restart the editor to apply changes.   ");

  if (!s_ReloadProjectRequiredReasons.IsEmpty())
    sText.Append("Reload the project to apply changes.   ");

  ezQtUiServices::ShowGlobalStatusBarMessage(sText);
}
