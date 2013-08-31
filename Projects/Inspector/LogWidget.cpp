#include <Inspector/LogWidget.moc.h>
#include <qlistwidget.h>
#include <Foundation/Communication/Telemetry.h>
#include <MainWindow.moc.h>

ezLogWidget* ezLogWidget::s_pWidget = NULL;

ezLogWidget::ezLogWidget(QWidget* parent) : QDockWidget (parent)
{
  s_pWidget = this;

  setupUi (this);

  ResetStats();
}

void ezLogWidget::on_ButtonClearLog_clicked()
{
  ListLog->clear();
  s_pWidget->m_Messages.Clear();
}

void ezLogWidget::ResetStats()
{
  ListLog->clear();
  s_pWidget->m_Messages.Clear();

  m_LogLevel = ezLog::EventType::All;
  m_sSearchText.Clear();

  ComboLogLevel->setCurrentIndex(ezLog::EventType::All - m_LogLevel);
}


void ezLogWidget::UpdateStats()
{
}

QListWidgetItem* ezLogWidget::CreateLogItem(const LogMsg& lm, ezInt32 iMessageIndex)
{
  ezStringBuilder sFormat;

  if (lm.m_Type == ezLog::EventType::BeginGroup)
    sFormat.Format("%*s>> %s", lm.m_uiIndentation * 4, "", lm.m_sMsg.GetData());
  else if (lm.m_Type == ezLog::EventType::EndGroup)
    sFormat.Format("%*s<< %s", lm.m_uiIndentation * 4, "", lm.m_sMsg.GetData());
  else if (lm.m_sTag.IsEmpty())
    sFormat.Format("%*s%s", lm.m_uiIndentation * 4, "", lm.m_sMsg.GetData());
  else
    sFormat.Format("%*s[%s]%s", lm.m_uiIndentation * 4, "", lm.m_sTag.GetData(), lm.m_sMsg.GetData());

  QListWidgetItem* pItem = new QListWidgetItem;
  pItem->setText(sFormat.GetData());

  switch(lm.m_Type)
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

  pItem->setData(Qt::UserRole, iMessageIndex);

  return pItem;
}

bool ezLogWidget::IsFiltered(const LogMsg& lm)
{
  if (lm.m_Type > m_LogLevel)
    return true;

  if (m_sSearchText.IsEmpty())
    return false;

  if (lm.m_sTag.FindSubString_NoCase(m_sSearchText.GetData()))
    return false;

  if (lm.m_sMsg.FindSubString_NoCase(m_sSearchText.GetData()))
    return false;

  return true;
}

void ezLogWidget::ProcessTelemetry_Log(void* pPassThrough)
{
  ezLogWidget* pWindow = (ezLogWidget*) pPassThrough;

  ezTelemetryMessage Msg;

  const bool bLastSelected = (pWindow->ListLog->count() == 0) || (pWindow->ListLog->currentItem() == pWindow->ListLog->item(pWindow->ListLog->count() - 1));
  bool bChange = false;

  while (ezTelemetry::RetrieveMessage('LOG', Msg) == EZ_SUCCESS)
  {
    ezInt16 iEventType = 0;
    ezUInt16 uiIndentation = 0;
    ezString sTag, sText;

    Msg.GetReader() >> iEventType;
    Msg.GetReader() >> uiIndentation;
    Msg.GetReader() >> sTag;
    Msg.GetReader() >> sText;

    LogMsg lm;
    lm.m_sMsg = sText;
    lm.m_sTag = sTag;
    lm.m_Type = (ezLog::EventType::Enum) iEventType;
    lm.m_uiIndentation = uiIndentation;

    s_pWidget->m_Messages.PushBack(lm);

    if (s_pWidget->IsFiltered(lm))
      continue;

    bChange = true;

    QListWidgetItem* pItem = s_pWidget->CreateLogItem(lm, s_pWidget->m_Messages.GetCount() - 1);
    pWindow->ListLog->addItem(pItem);
  }

  if (bChange && bLastSelected)
    pWindow->ListLog->setCurrentItem(pWindow->ListLog->item(pWindow->ListLog->count() - 1));
}

void ezLogWidget::on_ComboLogLevel_currentIndexChanged(int iIndex)
{
  m_LogLevel = (ezLog::EventType::Enum) (ezLog::EventType::All - iIndex);

  UpdateLogList();
}

void ezLogWidget::on_LineSearch_textChanged(QString sText)
{
  m_sSearchText = sText.toUtf8().data();

  UpdateLogList();
}

void ezLogWidget::UpdateLogList()
{
  ezInt32 iPrevSel = -1;

  if (ListLog->currentRow() >= 0)
  {
    iPrevSel = ListLog->item(ListLog->currentRow())->data(Qt::UserRole).toInt();
  }

  ListLog->blockSignals(true);
  ListLog->clear();

  ezDeque<ezInt32> FilteredList;

  for (ezUInt32 i = 0; i < m_Messages.GetCount(); ++i)
  { 
    if (IsFiltered(m_Messages[i]))
      continue;

    if (!FilteredList.IsEmpty())
    {
      if ((m_Messages[FilteredList.PeekBack()].m_Type == ezLog::EventType::BeginGroup) &&
          (m_Messages[i].m_Type == ezLog::EventType::EndGroup))
      {
        FilteredList.PopBack();
        continue;
      }
    }

    FilteredList.PushBack(i);
  }

  ezInt32 iSelectRow = -1;

  for (ezUInt32 i = 0; i < FilteredList.GetCount(); ++i)
  {
    QListWidgetItem* pItem = CreateLogItem(m_Messages[FilteredList[i]], FilteredList[i]);
    ListLog->addItem(pItem);

    if ((iSelectRow == -1) && (FilteredList[i] >= iPrevSel))
      iSelectRow = i;
  }

  
  if (iSelectRow >= 0)
    ListLog->setCurrentRow(iSelectRow);

  ListLog->blockSignals(false);
}