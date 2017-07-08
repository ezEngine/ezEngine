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
    ezInt16 iEventType = 0;
    ezUInt16 uiIndentation = 0;
    ezString sTag, sText;

    Msg.GetReader() >> iEventType;
    Msg.GetReader() >> uiIndentation;
    Msg.GetReader() >> sTag;
    Msg.GetReader() >> sText;

    ezLogEntry lm;
    lm.m_sMsg = sText;
    lm.m_sTag = sTag;
    lm.m_Type = (ezLogMsgType::Enum) iEventType;
    lm.m_uiIndentation = uiIndentation;
    s_pWidget->LogWidget->GetLog()->AddLogMsg(lm);
  }
}
