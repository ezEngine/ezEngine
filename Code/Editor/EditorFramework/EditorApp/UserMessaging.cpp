#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>

void ezQtEditorApp::AddRestartRequiredReason(const char* szReason)
{
  if (!m_RestartRequiredReasons.Find(szReason).IsValid())
  {
    m_RestartRequiredReasons.Insert(szReason);
    UpdateGlobalStatusBarMessage();
  }

  ezStringBuilder s;
  s.SetFormat("The editor process must be restarted.\nReason: '{0}'\n\nDo you want to restart now?", szReason);

  if (ezQtUiServices::MessageBoxQuestion(s, QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::Yes) == QMessageBox::StandardButton::Yes)
  {
    if (ezToolsProject::CanCloseProject())
    {
      LaunchEditor(ezToolsProject::GetSingleton()->GetProjectFile(), false);

      QApplication::closeAllWindows();
      return;
    }
  }
}

void ezQtEditorApp::AddReloadProjectRequiredReason(const char* szReason)
{
  if (!m_ReloadProjectRequiredReasons.Find(szReason).IsValid())
  {
    m_ReloadProjectRequiredReasons.Insert(szReason);

    ezStringBuilder s;
    s.SetFormat("The project must be reloaded.\nReason: '{0}'", szReason);

    ezQtUiServices::MessageBoxInformation(s);

    UpdateGlobalStatusBarMessage();
  }
}

void ezQtEditorApp::UpdateGlobalStatusBarMessage()
{
  ezStringBuilder sText;

  if (!m_RestartRequiredReasons.IsEmpty())
    sText.Append("Restart the editor to apply changes.   ");

  if (!m_ReloadProjectRequiredReasons.IsEmpty())
    sText.Append("Reload the project to apply changes.   ");

  ezQtUiServices::ShowGlobalStatusBarMessage(sText);
}
