#include <Inspector/InspectorPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Logging/LogEntry.h>
#include <GuiFoundation/Models/LogModel.moc.h>
#include <Inspector/LogDockWidget.moc.h>
#include <Inspector/MainWindow.moc.h>
#include <qlistwidget.h>

ezQtLogDockWidget* ezQtLogDockWidget::s_pWidget = nullptr;

ezQtLogDockWidget::ezQtLogDockWidget(QWidget* pParent)
  : ads::CDockWidget("Log", pParent)
{
  s_pWidget = this;
  setupUi(this);
  LogWidget->GetSearchWidget()->setPlaceholderText(QStringLiteral("Search Log"));

  this->setWidget(LogWidget);
}

void ezQtLogDockWidget::ResetStats()
{
  LogWidget->GetLog()->Clear();
}

void ezQtLogDockWidget::Log(const ezFormatString& text)
{
  ezStringBuilder tmp;

  ezLogEntry lm;
  lm.m_sMsg = text.GetText(tmp);
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

    lm.m_Type = (ezLogMsgType::Enum)iEventType;
    s_pWidget->LogWidget->GetLog()->AddLogMsg(lm);
  }
}
