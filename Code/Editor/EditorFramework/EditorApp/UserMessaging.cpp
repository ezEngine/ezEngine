#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>

void ezQtEditorApp::AddRestartRequiredReason(const char* szReason, bool bAskToRestart)
{
  if (!m_RestartRequiredReasons.Find(szReason).IsValid())
  {
    m_RestartRequiredReasons.Insert(szReason);
    UpdateGlobalStatusBarMessage();
  }

  if (!bAskToRestart)
    return;

  ezStringBuilder s;
  s.SetFormat("The editor process must be restarted.\nReason: '{0}'\n\nDo you want to restart now?", szReason);

  if (ezQtUiServices::MessageBoxQuestion(s, QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::Yes, QMessageBox::StandardButton::No) == QMessageBox::StandardButton::Yes)
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
    UpdateGlobalStatusBarMessage();
  }

  ezStringBuilder s;
  s.SetFormat("The project must be reloaded.\nReason: '{0}'\n\nDo you want to reload it now?", szReason);

  if (ezQtUiServices::MessageBoxQuestion(s, QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::Yes, QMessageBox::StandardButton::No) == QMessageBox::StandardButton::Yes)
  {
    if (ezToolsProject::CanCloseProject())
    {
      ezStringBuilder sProjectFile = ezToolsProject::GetSingleton()->GetProjectFile();

      SlotQueuedCloseProject();
      OpenProject(sProjectFile, true).IgnoreResult();
    }
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

void ezQtEditorApp::PhantomRttiManagerEventHandler(const ezPhantomRttiManagerEvent& e)
{
  if (e.m_Type == ezPhantomRttiManagerEvent::Type::TypeChanged && e.m_bIncompatibleChange)
  {
    // Don't ask to restart right away, this happens in the middle of processing the type updates from the engine process.
    ezStringBuilder sReason;
    sReason.SetFormat("The values of type '{}' were removed or changed.", e.m_pChangedType->GetTypeName());
    AddRestartRequiredReason(sReason, false);
  }
}
