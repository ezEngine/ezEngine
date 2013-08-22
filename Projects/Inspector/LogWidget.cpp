#include <Inspector/LogWidget.moc.h>
#include <qlistwidget.h>
#include <Foundation/Communication/Telemetry.h>

ezLogWidget* ezLogWidget::s_pWidget = NULL;

ezLogWidget::ezLogWidget(QWidget* parent) : QDockWidget (parent)
{
  s_pWidget = this;

  setupUi (this);

}

void ezLogWidget::on_ButtonClearLog_clicked()
{
  ListLog->clear();
}

void ezLogWidget::UpdateStats()
{
  static ezUInt32 uiServerID = 0;

    if (ezTelemetry::IsConnectedToServer())
    {
      if ((uiServerID == 0) && (ezTelemetry::GetServerID() != 0))
      {
        uiServerID = ezTelemetry::GetServerID();

        if (CheckAutoClear->isChecked())
          ListLog->clear();

        ListLog->addItem(QString::fromUtf8("Connected to Server with ID %1").arg(uiServerID));
        ListLog->setCurrentItem(ListLog->item(ListLog->count() - 1));

        ezTelemetry::SendToServer('APP', 'RQDT');
      }

      if (uiServerID != ezTelemetry::GetServerID())
      {
        uiServerID = ezTelemetry::GetServerID();

        if (CheckAutoClear->isChecked())
          ListLog->clear();

        ListLog->addItem(QString::fromUtf8("Connected to new Server with ID %1").arg(uiServerID));
        ListLog->setCurrentItem(ListLog->item(ListLog->count() - 1));
      }
    }
}

