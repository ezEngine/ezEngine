#include <PCH.h>
#include <Inspector/LogWidget.moc.h>
#include <qlistwidget.h>
#include <Foundation/Communication/Telemetry.h>
#include <MainWindow.moc.h>

ezLogWidget* ezLogWidget::s_pWidget = nullptr;

ezLogWidget::ezLogWidget(QWidget* parent) : QDockWidget (parent)
{
  s_pWidget = this;

  setupUi (this);

  m_LogLevel = ezLogMsgType::All;
  m_sSearchText.Clear();

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

  LineSearch->setText(m_sSearchText.GetData());

  ComboLogLevel->setCurrentIndex(ezLogMsgType::All - m_LogLevel);
}

void ezLogWidget::Log(const char* szFormat, ...)
{
  char szString[4096];

  va_list args;
  va_start (args, szFormat);

  ezStringUtils::vsnprintf(szString, 4096, szFormat, args);

  va_end (args);

  const bool bLastSelected = (ListLog->count() == 0) || (ListLog->currentItem() == ListLog->item(ListLog->count() - 1));

  LogMsg lm;
  lm.m_sMsg = szString;
  lm.m_Type = ezLogMsgType::InfoMsg;
  lm.m_uiIndentation = 0;

  m_Messages.PushBack(lm);

    if (IsFiltered(lm))
      return;

  QListWidgetItem* pItem = CreateLogItem(lm, m_Messages.GetCount() - 1);
  ListLog->addItem(pItem);

  if (bLastSelected)
    ListLog->setCurrentItem(ListLog->item(ListLog->count() - 1));
}

QListWidgetItem* ezLogWidget::CreateLogItem(const LogMsg& lm, ezInt32 iMessageIndex)
{
  ezStringBuilder sFormat;

  if (lm.m_Type == ezLogMsgType::BeginGroup)
    sFormat.Format("%*s>> %s (%s)", lm.m_uiIndentation * 4, "", lm.m_sMsg.GetData(), lm.m_sTag.GetData());
  else if (lm.m_Type == ezLogMsgType::EndGroup)
    sFormat.Format("%*s<< %s", lm.m_uiIndentation * 4, "", lm.m_sMsg.GetData());
  else if (lm.m_sTag.IsEmpty())
    sFormat.Format("%*s%s", lm.m_uiIndentation * 4, "", lm.m_sMsg.GetData());
  else
    sFormat.Format("%*s[%s]%s", lm.m_uiIndentation * 4, "", lm.m_sTag.GetData(), lm.m_sMsg.GetData());

  QListWidgetItem* pItem = new QListWidgetItem;
  pItem->setText(sFormat.GetData());

  switch (lm.m_Type)
  {
  case ezLogMsgType::BeginGroup:
    pItem->setTextColor(QColor::fromRgb(160, 90, 255));
    break;
  case ezLogMsgType::EndGroup:
    pItem->setTextColor(QColor::fromRgb(110, 60, 185));
    break;
  case ezLogMsgType::ErrorMsg:
    pItem->setTextColor(QColor::fromRgb(255, 0, 0));
    break;
  case ezLogMsgType::SeriousWarningMsg:
    pItem->setTextColor(QColor::fromRgb(255, 64, 0));
    break;
  case ezLogMsgType::WarningMsg:
    pItem->setTextColor(QColor::fromRgb(255, 140, 0));
    break;
  case ezLogMsgType::SuccessMsg:
    pItem->setTextColor(QColor::fromRgb(0, 128, 0));
    break;
  case ezLogMsgType::InfoMsg:
    //pItem->setTextColor(QColor::fromRgb(0, 0, 0));
    break;
  case ezLogMsgType::DevMsg:
    pItem->setTextColor(QColor::fromRgb(128, 128, 128));
    break;
  case ezLogMsgType::DebugMsg:
    pItem->setTextColor(QColor::fromRgb(255, 0, 255));
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

void ezLogWidget::ProcessTelemetry(void* pUnuseed)
{
  if (!s_pWidget)
    return;

  ezTelemetryMessage Msg;

  const bool bLastSelected = (s_pWidget->ListLog->count() == 0) || (s_pWidget->ListLog->currentItem() == s_pWidget->ListLog->item(s_pWidget->ListLog->count() - 1));
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
    lm.m_Type = (ezLogMsgType::Enum) iEventType;
    lm.m_uiIndentation = uiIndentation;

    s_pWidget->m_Messages.PushBack(lm);

    if (s_pWidget->IsFiltered(lm))
      continue;

    bChange = true;

    QListWidgetItem* pItem = s_pWidget->CreateLogItem(lm, s_pWidget->m_Messages.GetCount() - 1);
    s_pWidget->ListLog->addItem(pItem);
  }

  if (bChange && bLastSelected)
    s_pWidget->ListLog->setCurrentItem(s_pWidget->ListLog->item(s_pWidget->ListLog->count() - 1));
}

void ezLogWidget::on_ComboLogLevel_currentIndexChanged(int iIndex)
{
  m_LogLevel = (ezLogMsgType::Enum) (ezLogMsgType::All - iIndex);

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
      if ((m_Messages[FilteredList.PeekBack()].m_Type == ezLogMsgType::BeginGroup) &&
          (m_Messages[i].m_Type == ezLogMsgType::EndGroup))
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