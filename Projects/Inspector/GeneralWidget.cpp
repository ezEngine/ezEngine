#include <Inspector/GeneralWidget.moc.h>
#include <Foundation/Communication/Telemetry.h>

ezGeneralWidget* ezGeneralWidget::s_pWidget = NULL;

ezGeneralWidget::ezGeneralWidget(QWidget* parent) : QDockWidget (parent)
{
  s_pWidget = this;

  setupUi (this);

  
}

void ezGeneralWidget::UpdateStats()
{
  if (!ezTelemetry::IsConnectedToServer())
  {
    LabelStatus->setText("<p><span style=\" font-weight:600;\">Status: </span><span style=\" font-weight:600; color:#ff0000;\">Not Connected</span></p>");
    LabelServer->setText("<p>Server: N/A</p>");
    LabelPing->setText("<p>Ping: N/A</p>");
  }
  else
  {
    LabelStatus->setText("<p><span style=\" font-weight:600;\">Status: </span><span style=\" font-weight:600; color:#00aa00;\">Connected</span></p>");
    LabelServer->setText(QString::fromUtf8("<p>Server: %1 (%2)</p>").arg(ezTelemetry::GetServerName()).arg(ezTelemetry::GetServerIP()));
    LabelPing->setText(QString::fromUtf8("<p>Ping: %1ms</p>").arg((ezUInt32) ezTelemetry::GetPingToServer().GetMilliSeconds()));
  }
}

