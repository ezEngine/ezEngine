#include <PCH.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

void ezEditorApp::AddRestartRequiredReason(const char* szReason)
{
  if (!s_RestartRequiredReasons.Find(szReason).IsValid())
  {
    s_RestartRequiredReasons.Insert(szReason);

    ezStringBuilder s;
    s.Format("The editor process must be restarted.\nReason: '%s'", szReason);

    ezUIServices::MessageBoxInformation(s);

    UpdateGlobalStatusBarMessage();
  }
}

void ezEditorApp::AddReloadProjectRequiredReason(const char* szReason)
{
  if (!s_ReloadProjectRequiredReasons.Find(szReason).IsValid())
  {
    s_ReloadProjectRequiredReasons.Insert(szReason);

    ezStringBuilder s;
    s.Format("The project must be reloaded.\nReason: '%s'", szReason);

    ezUIServices::MessageBoxInformation(s);

    UpdateGlobalStatusBarMessage();
  }
}

void ezEditorApp::UpdateGlobalStatusBarMessage()
{
  ezStringBuilder sText;

  if (!s_RestartRequiredReasons.IsEmpty())
    sText.Append("Restart the Editor to apply changes.   ");

  if (!s_ReloadProjectRequiredReasons.IsEmpty())
    sText.Append("Reload the Project to apply changes.   ");

  ezUIServices::ShowGlobalStatusBarMessage(sText);
}