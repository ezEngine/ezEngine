#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/Models/LogModel.moc.h>
#include <QColor>
#include <QThread>


ezQtLogModel::ezQtLogModel(QObject* pParent)
  : QAbstractItemModel(pParent)
{
  m_bIsValid = true;
  m_LogLevel = ezLogMsgType::InfoMsg;
}

void ezQtLogModel::Invalidate()
{
  if (!m_bIsValid)
    return;

  beginResetModel();
  m_bIsValid = false;
  endResetModel();
}

void ezQtLogModel::Clear()
{
  if (m_AllMessages.IsEmpty())
    return;

  {
    EZ_LOCK(m_NewMessagesMutex);

    m_uiNumErrors = 0;
    m_uiNumSeriousWarnings = 0;
    m_uiNumWarnings = 0;

    m_AllMessages.Clear();
    m_VisibleMessages.Clear();
    m_BlockQueue.Clear();
    Invalidate();
    m_bIsValid = true;
  }

  Q_EMIT NewErrorsOrWarnings(nullptr, false);
}

void ezQtLogModel::SetLogLevel(ezLogMsgType::Enum logLevel)
{
  if (m_LogLevel == logLevel)
    return;

  m_LogLevel = logLevel;
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

    if (m_NewMessages.GetCount() > 1000)
    {
      // at the end of a play session, there can be a lot of log message queued up
      // this will take ages to process and is mostly pointless, so just print the last ones
      m_NewMessages.PopFront();
    }

    m_NewMessages.PushBack(msg);
  }

  // always queue the message processing, otherwise it can happen that an error during this
  // triggers recursive logging, which is forbidden
  QMetaObject::invokeMethod(this, "ProcessNewMessages", Qt::ConnectionType::QueuedConnection);

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

QVariant ezQtLogModel::data(const QModelIndex& index, int iRole) const
{
  if (!index.isValid() || index.column() != 0)
    return QVariant();

  UpdateVisibleEntries();

  const ezInt32 iRow = index.row();
  if (iRow < 0 || iRow >= (ezInt32)m_VisibleMessages.GetCount())
    return QVariant();

  const ezLogEntry& msg = *m_VisibleMessages[iRow];

  switch (iRole)
  {
    case Qt::DisplayRole:
    {
      if (msg.m_sMsg.FindSubString("\n") != nullptr)
      {
        ezStringBuilder sTemp = msg.m_sMsg;
        sTemp.ReplaceAll("\n", " ");
        return ezMakeQString(sTemp);
      }
      return ezMakeQString(msg.m_sMsg);
    }
    case Qt::ToolTipRole:
    {
      return ezMakeQString(msg.m_sMsg);
    }
    case Qt::ForegroundRole:
    {
      switch (msg.m_Type)
      {
        case ezLogMsgType::BeginGroup:
          return ezToQtColor(ezColorScheme::LightUI(ezColorScheme::Gray));
        case ezLogMsgType::EndGroup:
          return ezToQtColor(ezColorScheme::DarkUI(ezColorScheme::Gray));
        case ezLogMsgType::ErrorMsg:
          return ezToQtColor(ezColorScheme::LightUI(ezColorScheme::Red));
        case ezLogMsgType::SeriousWarningMsg:
          return ezToQtColor(ezColorScheme::LightUI(ezColorScheme::Orange));
        case ezLogMsgType::WarningMsg:
          return ezToQtColor(ezColorScheme::LightUI(ezColorScheme::Yellow));
        case ezLogMsgType::SuccessMsg:
          return ezToQtColor(ezColorScheme::LightUI(ezColorScheme::Green));
        case ezLogMsgType::DevMsg:
          return ezToQtColor(ezColorScheme::LightUI(ezColorScheme::Blue));
        case ezLogMsgType::DebugMsg:
          return ezToQtColor(ezColorScheme::LightUI(ezColorScheme::Cyan));
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
    return Qt::ItemFlags();

  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant ezQtLogModel::headerData(int iSection, Qt::Orientation orientation, int iRole) const
{
  return QVariant();
}

QModelIndex ezQtLogModel::index(int iRow, int iColumn, const QModelIndex& parent) const
{
  if (parent.isValid() || iColumn != 0)
    return QModelIndex();

  return createIndex(iRow, iColumn, iRow);
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
  bool bNewErrors = false;
  ezStringBuilder sLatestWarning;
  ezStringBuilder sLatestError;

  {
    EZ_LOCK(m_NewMessagesMutex);
    ezStringBuilder s;
    for (const auto& msg : m_NewMessages)
    {
      m_AllMessages.PushBack(msg);

      if (msg.m_Type == ezLogMsgType::BeginGroup || msg.m_Type == ezLogMsgType::EndGroup)
      {
        s.SetPrintf("%*s<<< %s", msg.m_uiIndentation, "", msg.m_sMsg.GetData());

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
        s.SetPrintf("%*s%s", 4 * msg.m_uiIndentation, "", msg.m_sMsg.GetData());
        m_AllMessages.PeekBack().m_sMsg = s;

        if (msg.m_Type == ezLogMsgType::ErrorMsg)
        {
          sLatestError = msg.m_sMsg;
          bNewErrors = true;
          ++m_uiNumErrors;
        }
        else if (msg.m_Type == ezLogMsgType::SeriousWarningMsg)
        {
          sLatestWarning = msg.m_sMsg;
          bNewErrors = true;
          ++m_uiNumSeriousWarnings;
        }
        else if (msg.m_Type == ezLogMsgType::WarningMsg)
        {
          sLatestWarning = msg.m_sMsg;
          bNewErrors = true;
          ++m_uiNumWarnings;
        }
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

  if (bNewErrors)
  {
    if (!sLatestError.IsEmpty())
    {
      Q_EMIT NewErrorsOrWarnings(sLatestError, true);
    }
    else
    {
      Q_EMIT NewErrorsOrWarnings(sLatestWarning, false);
    }
  }
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
