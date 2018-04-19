#include <PCH.h>
#include <Inspector/LogDockWidget.moc.h>
#include <qlistwidget.h>
#include <Foundation/Communication/Telemetry.h>
#include <MainWindow.moc.h>
#include <Foundation/Logging/LogEntry.h>
#include <GuiFoundation/Models/LogModel.moc.h>

ezQtLogDockWidget* ezQtLogDockWidget::s_pWidget = nullptr;

ezQtLogDockWidget::ezQtLogDockWidget(QWidget* parent) : QDockWidget (parent)
{
  s_pWidget = this;
  setupUi (this);
  LogWidget->GetSearchWidget()->setPlaceholderText(QStringLiteral("Search Log"));
}

void ezQtLogDockWidget::ResetStats()
{
  LogWidget->GetLog()->Clear();
}

void ezQtLogDockWidget::Log(const ezFormatString& sText)
{
  ezStringBuilder tmp;

  ezLogEntry lm;
  lm.m_sMsg = sText.GetText(tmp);
  lm.m_Type = ezLogMsgType::InfoMsg;
  lm.m_uiIndentation = 0;
  LogWidget->GetLog()->AddLogMsg(lm);
}

void ezQtLogDockWidget::ProcessTelemetry(void* pUnuseed)
{
  if (!s_pWidget)
    return;

  ezTelemetryMessage Msg;

  while (ezTelemetry::RetrieveMessage(' LOG', Msg) == EZ_SUCCESS)
  {
    ezLogEntry lm;
    ezInt8 iEventType = 0;

    Msg.GetReader() >> iEventType;
    Msg.GetReader() >> lm.m_uiIndentation;
    Msg.GetReader() >> lm.m_sTag;
    Msg.GetReader() >> lm.m_sMsg;

    if (iEventType == ezLogMsgType::EndGroup)
      Msg.GetReader() >> lm.m_fSeconds;

    lm.m_Type = (ezLogMsgType::Enum) iEventType;
    s_pWidget->LogWidget->GetLog()->AddLogMsg(lm);
  }
}
