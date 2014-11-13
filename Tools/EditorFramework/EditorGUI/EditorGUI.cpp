#include <PCH.h>
#include <EditorFramework/EditorGUI.moc.h>
#include <EditorFramework/EditorFramework.h>
#include <QSettings>
#include <QMessageBox>

ezEditorGUI* ezEditorGUI::GetInstance()
{
  static ezEditorGUI instance;
  return &instance;
}

ezEditorGUI::ezEditorGUI()
{
  m_pColorDlg = nullptr;
}

void ezEditorGUI::SaveState()
{
  QSettings Settings;
  Settings.beginGroup("EditorGUI");
  {
    Settings.setValue("ColorDlgPos", m_ColorDlgPos);
  }
  Settings.endGroup();
}

void ezEditorGUI::LoadState()
{
  QSettings Settings;
  Settings.beginGroup("EditorGUI");
  {
    m_ColorDlgPos = Settings.value("ColorDlgPos", QPoint(100, 100)).toPoint();
  }
  Settings.endGroup();
}

void ezEditorGUI::MessageBoxStatus(const ezStatus& s, const char* szFailureMsg, const char* szSuccessMsg, bool bOnlySuccessMsgIfDetails)
{
  ezStringBuilder sResult;

  if (s.m_Result.Succeeded())
  {
    if (ezStringUtils::IsNullOrEmpty(szSuccessMsg))
      return;

    if (bOnlySuccessMsgIfDetails && s.m_sError.IsEmpty())
      return;

    sResult = szSuccessMsg;

    if (!s.m_sError.IsEmpty())
      sResult.AppendFormat("\n\nDetails:\n%s", s.m_sError.GetData());

    MessageBoxInformation(sResult);
  }
  else
  {
    sResult = szFailureMsg;

    if (!s.m_sError.IsEmpty())
      sResult.AppendFormat("\n\nDetails:\n%s", s.m_sError.GetData());

    MessageBoxWarning(sResult);
  }
}

void ezEditorGUI::MessageBoxInformation(const char* szMsg)
{
  QMessageBox::information(ezEditorFramework::GetMainWindow(), QLatin1String(ezEditorFramework::GetApplicationName().GetData()), QString::fromUtf8(szMsg), QMessageBox::StandardButton::Ok);
}

void ezEditorGUI::MessageBoxWarning(const char* szMsg)
{
  QMessageBox::warning(ezEditorFramework::GetMainWindow(), QLatin1String(ezEditorFramework::GetApplicationName().GetData()), QString::fromUtf8(szMsg), QMessageBox::StandardButton::Ok);
}

