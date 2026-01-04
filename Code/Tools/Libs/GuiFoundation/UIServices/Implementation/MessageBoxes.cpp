#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/Logging/Log.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

const char* ezQtUiServices::GetOwnVersionString()
{
  return EZ_PP_STRINGIFY(BUILDSYSTEM_SDKVERSION_MAJOR) "." EZ_PP_STRINGIFY(BUILDSYSTEM_SDKVERSION_MINOR) "." EZ_PP_STRINGIFY(BUILDSYSTEM_SDKVERSION_PATCH);
}

void ezQtUiServices::MessageBoxStatus(const ezStatus& s, const char* szFailureMsg, const char* szSuccessMsg, bool bOnlySuccessMsgIfDetails)
{
  ezStringBuilder sResult;

  if (s.Succeeded())
  {
    if (ezStringUtils::IsNullOrEmpty(szSuccessMsg))
      return;

    if (bOnlySuccessMsgIfDetails && s.GetMessageString().IsEmpty())
      return;

    sResult = szSuccessMsg;

    if (!s.GetMessageString().IsEmpty())
      sResult.AppendFormat("\n\nDetails:\n{0}", s.GetMessageString());

    MessageBoxInformation(sResult);
  }
  else
  {
    sResult = szFailureMsg;

    if (!s.GetMessageString().IsEmpty())
      sResult.AppendFormat("\n\nDetails:\n{0}", s.GetMessageString());

    MessageBoxWarning(sResult);
  }
}

void ezQtUiServices::MessageBoxInformation(const ezFormatString& msg, ezStringView sDontShowAgainID)
{
  ezStringBuilder tmp;

  if (s_bHeadless)
    ezLog::Info(msg.GetText(tmp));
  else
  {
    QMessageBox box(QApplication::activeWindow());
    box.setWindowTitle(ezApplication::GetApplicationInstance()->GetApplicationName().GetData());
    box.setText(QString::fromUtf8(msg.GetTextCStr(tmp)));
    box.setIcon(QMessageBox::Icon::Information);

    QSettings Settings;
    Settings.beginGroup(GetOwnVersionString());
    Settings.beginGroup("msgbox-dontshow");

    if (!sDontShowAgainID.IsEmpty())
    {
      ShowAllDocumentsTemporaryStatusBarMessage(msg, ezTime::Seconds(3));

      if (Settings.value(sDontShowAgainID.GetData(tmp), 0) != 0)
      {
        // don't show the messagebox again was selected at some point
        return;
      }

      box.setCheckBox(new QCheckBox("Don't show again"));
    }

    box.exec();

    if (box.checkBox() && box.checkBox()->isChecked())
    {
      Settings.setValue(sDontShowAgainID.GetData(tmp), 1);
    }
  }
}

void ezQtUiServices::MessageBoxWarning(const ezFormatString& msg)
{
  ezStringBuilder tmp;

  if (s_bHeadless)
    ezLog::Warning(msg.GetText(tmp));
  else
  {
    QMessageBox::warning(QApplication::activeWindow(), ezApplication::GetApplicationInstance()->GetApplicationName().GetData(), QString::fromUtf8(msg.GetTextCStr(tmp)), QMessageBox::StandardButton::Ok);
  }
}

QMessageBox::StandardButton ezQtUiServices::MessageBoxQuestion(
  const ezFormatString& msg, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton)
{
  if (s_bHeadless)
  {
    return defaultButton;
  }
  else
  {
    ezStringBuilder tmp;

    return QMessageBox::question(QApplication::activeWindow(), ezApplication::GetApplicationInstance()->GetApplicationName().GetData(), QString::fromUtf8(msg.GetTextCStr(tmp)), buttons, defaultButton);
  }
}
