#include <Inspector/LogWidget.moc.h>
#include <qlistwidget.h>
#include <Foundation/Communication/Telemetry.h>
#include <MainWindow.moc.h>

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
  static bool bConnected = false;

  if (ezTelemetry::IsConnectedToServer())
  {
    bConnected = true;

    if ((uiServerID == 0) && (ezTelemetry::GetServerID() != 0))
    {
      uiServerID = ezTelemetry::GetServerID();

      if (CheckAutoClear->isChecked())
        ListLog->clear();

      ezStringBuilder s;
      s.Format("Connected to Server with ID %i", uiServerID);

      //ListLog->addItem(QString::fromUtf8("Connected to Server with ID %1").arg(uiServerID));
      //ListLog->setCurrentItem(ListLog->item(ListLog->count() - 1));

      ezMainWindow::s_pWidget->Log(s.GetData());

      ezTelemetry::SendToServer('APP', 'RQDT');
    }

    if (uiServerID != ezTelemetry::GetServerID())
    {
      uiServerID = ezTelemetry::GetServerID();

      if (CheckAutoClear->isChecked())
        ListLog->clear();

      ezStringBuilder s;
      s.Format("Connected to Server with ID %i", uiServerID);

      //ListLog->addItem(QString::fromUtf8("Connected to new Server with ID %1").arg(uiServerID));
      //ListLog->setCurrentItem(ListLog->item(ListLog->count() - 1));

      ezMainWindow::s_pWidget->Log(s.GetData());
    }
    else
    if (!bConnected)
    {
      ezMainWindow::s_pWidget->Log("Reconnected to Server.");
    }
  }
  else
  {
    if (bConnected)
    {
      ezMainWindow::s_pWidget->Log("Lost Connection to Server.");
    }

    bConnected = false;
  }
}

