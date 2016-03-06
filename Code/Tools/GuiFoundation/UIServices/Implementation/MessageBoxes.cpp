#include <GuiFoundation/PCH.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QMessageBox>

ezString ezUIServices::s_sApplicationName = "ezEditor";

void ezUIServices::MessageBoxStatus(const ezStatus& s, const char* szFailureMsg, const char* szSuccessMsg, bool bOnlySuccessMsgIfDetails)
{
  ezStringBuilder sResult;

  if (s.m_Result.Succeeded())
  {
    if (ezStringUtils::IsNullOrEmpty(szSuccessMsg))
      return;

    if (bOnlySuccessMsgIfDetails && s.m_sMessage.IsEmpty())
      return;

    sResult = szSuccessMsg;

    if (!s.m_sMessage.IsEmpty())
      sResult.AppendFormat("\n\nDetails:\n%s", s.m_sMessage.GetData());

    MessageBoxInformation(sResult);
  }
  else
  {
    sResult = szFailureMsg;

    if (!s.m_sMessage.IsEmpty())
      sResult.AppendFormat("\n\nDetails:\n%s", s.m_sMessage.GetData());

    MessageBoxWarning(sResult);
  }
}

void ezUIServices::MessageBoxInformation(const char* szMsg)
{
  QMessageBox::information(QApplication::activeWindow(), QString::fromUtf8(s_sApplicationName.GetData()), QString::fromUtf8(szMsg), QMessageBox::StandardButton::Ok);
}

void ezUIServices::MessageBoxWarning(const char* szMsg)
{
  QMessageBox::warning(QApplication::activeWindow(), QString::fromUtf8(s_sApplicationName.GetData()), QString::fromUtf8(szMsg), QMessageBox::StandardButton::Ok);
}

QMessageBox::StandardButton ezUIServices::MessageBoxQuestion(const char* szMsg, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton)
{
  return QMessageBox::question(QApplication::activeWindow(), QString::fromUtf8(s_sApplicationName.GetData()), QString::fromUtf8(szMsg), buttons, defaultButton);
}
