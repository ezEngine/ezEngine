#include <GuiFoundationPCH.h>

#include <GuiFoundation/Models/LogModel.moc.h>
#include <QColor>
#include <QThread>


ezQtLogModel::ezQtLogModel(QObject* parent)
    : QAbstractItemModel(parent)
{
  m_bIsValid = true;
  m_LogLevel = ezLogMsgType::InfoMsg;
}

void ezQtLogModel::Invalidate()
{
  if (!m_bIsValid)
    return;

  m_bIsValid = false;
  dataChanged(QModelIndex(), QModelIndex());
}

void ezQtLogModel::Clear()
{
  if (m_AllMessages.IsEmpty())
    return;

  m_AllMessages.Clear();
  m_VisibleMessages.Clear();
  m_BlockQueue.Clear();
  Invalidate();
  m_bIsValid = true;
}

void ezQtLogModel::SetLogLevel(ezLogMsgType::Enum LogLevel)
{
  if (m_LogLevel == LogLevel)
    return;

  m_LogLevel = LogLevel;
  Invalidate();
}

void ezQtLogModel::SetSearchText(const char* szText)
{
  if (m_sSearchText == szText)
    return;

  m_sSearchText = szText;
  Invalidate();
}

void ezQtLogModel::AddLogMsg(const ezLogEntry& msg)
{
  {
    EZ_LOCK(m_NewMessagesMutex);
    m_NewMessages.PushBack(msg);
  }

  if (QThread::currentThread() == thread())
  {
    ProcessNewMessages();
  }
  else
  {
    QMetaObject::invokeMethod(this, "ProcessNewMessages", Qt::ConnectionType::QueuedConnection);
  }

  return;
}

bool ezQtLogModel::IsFiltered(const ezLogEntry& lm) const
{
  if (lm.m_Type < ezLogMsgType::None)
    return false;

  if (lm.m_Type > m_LogLevel)
    return true;

  if (m_sSearchText.IsEmpty())
    return false;

  if (lm.m_sMsg.FindSubString_NoCase(m_sSearchText.GetData()))
    return false;

  return true;
}

////////////////////////////////////////////////////////////////////////
// ezQtLogModel QAbstractItemModel functions
////////////////////////////////////////////////////////////////////////

QVariant ezQtLogModel::data(const QModelIndex& index, int role) const
{
  if (!index.isValid() || index.column() != 0)
    return QVariant();

  UpdateVisibleEntries();

  const ezInt32 iRow = index.row();
  if (iRow < 0 || iRow >= (ezInt32)m_VisibleMessages.GetCount())
    return QVariant();

  const ezLogEntry& msg = *m_VisibleMessages[iRow];

  switch (role)
  {
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
    {
      return QString::fromUtf8(msg.m_sMsg.GetData());
    }
    case Qt::TextColorRole:
    {
      switch (msg.m_Type)
      {
        case ezLogMsgType::BeginGroup:
          return QColor::fromRgb(160, 90, 255);
        case ezLogMsgType::EndGroup:
          return QColor::fromRgb(110, 60, 185);
        case ezLogMsgType::ErrorMsg:
          return QColor::fromRgb(255, 0, 0);
        case ezLogMsgType::SeriousWarningMsg:
          return QColor::fromRgb(255, 64, 0);
        case ezLogMsgType::WarningMsg:
          return QColor::fromRgb(255, 140, 0);
        case ezLogMsgType::SuccessMsg:
          return QColor::fromRgb(0, 128, 0);
        case ezLogMsgType::DevMsg:
          return QColor::fromRgb(63, 143, 210);
        case ezLogMsgType::DebugMsg:
          return QColor::fromRgb(128, 128, 128);
        default:
          return QVariant();
      }
    }

    default:
      return QVariant();
  }
}

Qt::ItemFlags ezQtLogModel::flags(const QModelIndex& index) const
{
  if (!index.isValid())
    return 0;

  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant ezQtLogModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  return QVariant();
}

QModelIndex ezQtLogModel::index(int row, int column, const QModelIndex& parent) const
{
  if (parent.isValid() || column != 0)
    return QModelIndex();

  return createIndex(row, column, row);
}

QModelIndex ezQtLogModel::parent(const QModelIndex& index) const
{
  return QModelIndex();
}

int ezQtLogModel::rowCount(const QModelIndex& parent) const
{
  if (parent.isValid())
    return 0;

  UpdateVisibleEntries();

  return (int)m_VisibleMessages.GetCount();
}

int ezQtLogModel::columnCount(const QModelIndex& parent) const
{
  return 1;
}


void ezQtLogModel::ProcessNewMessages()
{
  EZ_LOCK(m_NewMessagesMutex);
  ezStringBuilder s;
  for (const auto& msg : m_NewMessages)
  {
    m_AllMessages.PushBack(msg);

    if (msg.m_Type == ezLogMsgType::BeginGroup || msg.m_Type == ezLogMsgType::EndGroup)
    {
      s.Printf("%*s<<< %s", msg.m_uiIndentation, "", msg.m_sMsg.GetData());

      if (msg.m_Type == ezLogMsgType::EndGroup)
      {
        s.AppendFormat(" ({0} sec) >>>", ezArgF(msg.m_fSeconds, 3));
      }
      else if (!msg.m_sTag.IsEmpty())
      {
        s.Append(" (", msg.m_sTag, ") >>>");
      }
      else
      {
        s.Append(" >>>");
      }

      m_AllMessages.PeekBack().m_sMsg = s;
    }
    else
    {
      s.Printf("%*s%s", 4 * msg.m_uiIndentation, "", msg.m_sMsg.GetData());
      m_AllMessages.PeekBack().m_sMsg = s;
    }


    // if the message would not be shown anyway, don't trigger an update
    if (IsFiltered(msg))
      continue;

    if (msg.m_Type == ezLogMsgType::BeginGroup)
    {
      m_BlockQueue.PushBack(&m_AllMessages.PeekBack());
      continue;
    }
    else if (msg.m_Type == ezLogMsgType::EndGroup)
    {
      if (!m_BlockQueue.IsEmpty())
      {
        m_BlockQueue.PopBack();
        continue;
      }
    }

    for (auto pMsg : m_BlockQueue)
    {
      beginInsertRows(QModelIndex(), m_VisibleMessages.GetCount(), m_VisibleMessages.GetCount());
      m_VisibleMessages.PushBack(pMsg);
      endInsertRows();
    }

    m_BlockQueue.Clear();

    beginInsertRows(QModelIndex(), m_VisibleMessages.GetCount(), m_VisibleMessages.GetCount());
    m_VisibleMessages.PushBack(&m_AllMessages.PeekBack());
    endInsertRows();
  }

  m_NewMessages.Clear();
}

void ezQtLogModel::UpdateVisibleEntries() const
{
  if (m_bIsValid)
    return;

  m_bIsValid = true;
  m_VisibleMessages.Clear();

  for (const auto& msg : m_AllMessages)
  {
    if (IsFiltered(msg))
      continue;

    if (msg.m_Type == ezLogMsgType::EndGroup)
    {
      if (!m_VisibleMessages.IsEmpty())
      {
        if (m_VisibleMessages.PeekBack()->m_Type == ezLogMsgType::BeginGroup)
          m_VisibleMessages.PopBack();
        else
          m_VisibleMessages.PushBack(&msg);
      }
    }
    else
    {
      m_VisibleMessages.PushBack(&msg);
    }
  }
}
