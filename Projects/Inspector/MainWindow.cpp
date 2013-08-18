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

  // Update Log
  {
    ezTelemetryMessage Msg;

    const bool bLastSelected = (ListLog->count() == 0) || (ListLog->currentItem() == ListLog->item(ListLog->count() - 1));
    bool bChange = false;

    ezTelemetry::AcceptMessagesForSystem('LOG', true);
    ezTelemetry::AcceptMessagesForSystem('MEM', true);

    while (ezTelemetry::RetrieveMessage('LOG', Msg) == EZ_SUCCESS)
    {
      bChange = true;

      ezUInt16 uiEventType = 0;
      ezUInt16 uiIndentation = 0;
      ezString sTag, sText;

      Msg.GetReader() >> uiEventType;
      Msg.GetReader() >> uiIndentation;
      Msg.GetReader() >> sTag;
      Msg.GetReader() >> sText;

      ezStringBuilder sFormat;

      if (sTag.IsEmpty())
        sFormat.Format("%*s%s", uiIndentation * 4, "", sText.GetData());
      else
        sFormat.Format("%*s[%s]%s", uiIndentation * 4, "", sTag.GetData(), sText.GetData());

      QListWidgetItem* pItem = new QListWidgetItem;
      pItem->setText(sFormat.GetData());

      switch(uiEventType)
      {
      case ezLog::EventType::BeginGroup:
        pItem->setTextColor(QColor::fromRgb(100, 0, 255));
        break;
      case ezLog::EventType::EndGroup:
        pItem->setTextColor(QColor::fromRgb(100, 0, 255));
        break;
      case ezLog::EventType::FlushToDisk:
        pItem->setText(">> Log Flush To Disk <<");
        pItem->setTextColor(QColor::fromRgb(255, 130, 0));
        break;
      case ezLog::EventType::FatalErrorMsg:
        pItem->setTextColor(QColor::fromRgb(150, 0, 0));
        break;
      case ezLog::EventType::ErrorMsg:
        pItem->setTextColor(QColor::fromRgb(255, 0, 0));
        break;
      case ezLog::EventType::SeriousWarningMsg:
        pItem->setTextColor(QColor::fromRgb(255, 64, 0));
        break;
      case ezLog::EventType::WarningMsg:
        pItem->setTextColor(QColor::fromRgb(255, 140, 0));
        break;
      case ezLog::EventType::SuccessMsg:
        pItem->setTextColor(QColor::fromRgb(0, 128, 0));
        break;
      case ezLog::EventType::InfoMsg:
        pItem->setTextColor(QColor::fromRgb(0, 0, 0));
        break;
      case ezLog::EventType::DevMsg:
        pItem->setTextColor(QColor::fromRgb(128, 128, 128));
        break;
      case ezLog::EventType::DebugMsg:
        pItem->setTextColor(QColor::fromRgb(255, 0, 255));
        break;
      case ezLog::EventType::DebugRegularMsg:
        pItem->setTextColor(QColor::fromRgb(0, 255, 255));
        break;
      }

      ListLog->addItem(pItem);
    };

    while (ezTelemetry::RetrieveMessage('MEM', Msg) == EZ_SUCCESS)
    {
      ezIAllocator::Stats MemStat;
      Msg.GetReader().ReadBytes(&MemStat, sizeof(ezIAllocator::Stats));

      LabelNumAllocs->setText(QString::fromUtf8("Allocations: %1").arg(MemStat.m_uiNumAllocations));
      LabelNumDeallocs->setText(QString::fromUtf8("Deallocations: %1").arg(MemStat.m_uiNumDeallocations));
      LabelNumLiveAllocs->setText(QString::fromUtf8("Live Allocs: %1").arg(MemStat.m_uiNumLiveAllocations));

      if (MemStat.m_uiUsedMemorySize < 1024)
        LabelUsedMemory->setText(QString::fromUtf8("Used Memory: %1 Byte").arg(MemStat.m_uiUsedMemorySize));
      else
      if (MemStat.m_uiUsedMemorySize < 1024 * 1024)
        LabelUsedMemory->setText(QString::fromUtf8("Used Memory: %1 KB").arg(MemStat.m_uiUsedMemorySize / 1024));
      else
        LabelUsedMemory->setText(QString::fromUtf8("Used Memory: %1 MB").arg(MemStat.m_uiUsedMemorySize / 1024 / 1024));

      const ezUInt32 uiOverhead = MemStat.m_uiUsedMemorySize - MemStat.m_uiAllocationSize;

      if (uiOverhead < 1024)
        LabelMemoryOverhead->setText(QString::fromUtf8("Overhead: %1 Byte").arg(uiOverhead));
      else
      if (uiOverhead < 1024 * 1024)
        LabelMemoryOverhead->setText(QString::fromUtf8("Overhead: %1 KB").arg(uiOverhead / 1024));
      else
        LabelMemoryOverhead->setText(QString::fromUtf8("Overhead: %1 MB").arg((uiOverhead) / 1024 / 1024));

    }

    if (bChange && bLastSelected)
      ListLog->setCurrentItem(ListLog->item(ListLog->count() - 1));
  }

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
