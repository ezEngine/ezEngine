#include <Inspector/MainWindow.moc.h>
#include <Foundation/Communication/Telemetry.h>
#include <qlistwidget.h>
#include <qinputdialog.h>

ezMainWindow::ezMainWindow() : QMainWindow()
{
  setupUi(this);

}

void ezMainWindow::paintEvent(QPaintEvent* event)
{
  static ezUInt32 uiServerID = 0;

  // Update General Info
  {
    if (!ezTelemetry::IsConnectedToServer())
    {
      LabelStatus->setText("<p><span style=\" font-weight:600;\">Status: </span><span style=\" font-weight:600; color:#ff0000;\">Not Connected</span></p>");
      LabelServer->setText("<p>Server: N/A</p>");
      LabelPing->setText("<p>Ping: N/A</p>");

      LabelNumAllocs->setText(QString::fromUtf8("Allocations: N/A"));
      LabelNumDeallocs->setText(QString::fromUtf8("Deallocations: N/A"));
      LabelNumLiveAllocs->setText(QString::fromUtf8("Live Allocs: N/A"));
      LabelUsedMemory->setText(QString::fromUtf8("Used Memory: N/A"));
      LabelMemoryOverhead->setText(QString::fromUtf8("Overhead: N/A"));

    }
    else
    {
      LabelStatus->setText("<p><span style=\" font-weight:600;\">Status: </span><span style=\" font-weight:600; color:#00aa00;\">Connected</span></p>");
      LabelServer->setText(QString::fromUtf8("<p>Server: %1 (%2)</p>").arg(ezTelemetry::GetServerName()).arg(ezTelemetry::GetServerIP()));
      LabelPing->setText(QString::fromUtf8("<p>Ping: %1ms</p>").arg((ezUInt32) ezTelemetry::GetPingToServer().GetMilliSeconds()));

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

  ezTelemetry::CallProcessMessagesCallbacks();

  update();
}

void ezMainWindow::on_ButtonClearLog_clicked()
{
  ListLog->clear();
}

void ezMainWindow::on_ButtonConnect_clicked()
{
  bool bOk = false;
  QString sRes = QInputDialog::getText(this, "Input Server Name or IP Address", "", QLineEdit::Normal, "", &bOk);

  if (!bOk)
    return;

  if (ezTelemetry::ConnectToServer(sRes.toUtf8().data()) == EZ_SUCCESS)
  {
  }
}
