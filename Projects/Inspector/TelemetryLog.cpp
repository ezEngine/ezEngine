#include <Inspector/MainWindow.moc.h>
#include <Inspector/LogWidget.moc.h>
#include <Foundation/Communication/Telemetry.h>
#include <qlistwidget.h>
#include <qinputdialog.h>

void ezLogWidget::ProcessTelemetry_Log(void* pPassThrough)
{
  ezLogWidget* pWindow = (ezLogWidget*) pPassThrough;

  ezTelemetryMessage Msg;

  const bool bLastSelected = (pWindow->ListLog->count() == 0) || (pWindow->ListLog->currentItem() == pWindow->ListLog->item(pWindow->ListLog->count() - 1));
  bool bChange = false;

  while (ezTelemetry::RetrieveMessage('LOG', Msg) == EZ_SUCCESS)
  {
    bChange = true;

    ezInt16 iEventType = 0;
    ezUInt16 uiIndentation = 0;
    ezString sTag, sText;

    Msg.GetReader() >> iEventType;
    Msg.GetReader() >> uiIndentation;
    Msg.GetReader() >> sTag;
    Msg.GetReader() >> sText;

    ezStringBuilder sFormat;

    if (iEventType == ezLog::EventType::BeginGroup)
      sFormat.Format("%*s>> %s", uiIndentation * 4, "", sText.GetData());
    else
    if (iEventType == ezLog::EventType::EndGroup)
      sFormat.Format("%*s<< %s", uiIndentation * 4, "", sText.GetData());
    else
    if (sTag.IsEmpty())
      sFormat.Format("%*s%s", uiIndentation * 4, "", sText.GetData());
    else
      sFormat.Format("%*s[%s]%s", uiIndentation * 4, "", sTag.GetData(), sText.GetData());

    QListWidgetItem* pItem = new QListWidgetItem;
    pItem->setText(sFormat.GetData());

    switch(iEventType)
    {
    case ezLog::EventType::BeginGroup:
      pItem->setTextColor(QColor::fromRgb(160, 90, 255));
      break;
    case ezLog::EventType::EndGroup:
      pItem->setTextColor(QColor::fromRgb(110, 60, 185));
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
      //pItem->setTextColor(QColor::fromRgb(0, 0, 0));
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

    pWindow->ListLog->addItem(pItem);
  }

  if (bChange && bLastSelected)
    pWindow->ListLog->setCurrentItem(pWindow->ListLog->item(pWindow->ListLog->count() - 1));
}

void ezLogWidget::ResetStats()
{
  if (CheckAutoClear->isChecked())
    ListLog->clear();
}

