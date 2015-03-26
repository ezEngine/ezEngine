#include <TestFramework/PCH.h>
#ifdef EZ_USE_QT

#include <TestFramework/Framework/Qt/qtLogMessageDock.h>
#include <TestFramework/Framework/TestFramework.h>
#include <Foundation/Strings/StringUtils.h>
#include <QApplication>
#include <QStringBuilder>

////////////////////////////////////////////////////////////////////////
// ezQtLogMessageDock public functions
////////////////////////////////////////////////////////////////////////

ezQtLogMessageDock::ezQtLogMessageDock(QObject* pParent, const ezTestFrameworkResult* pResult)
{
  setupUi(this);
  m_pModel = new ezQtLogMessageModel(this, pResult);
  ListView->setModel(m_pModel);
}

ezQtLogMessageDock::~ezQtLogMessageDock()
{
  ListView->setModel(nullptr);
  delete m_pModel;
  m_pModel = nullptr;
}

void ezQtLogMessageDock::resetModel()
{
  m_pModel->resetModel();
}

void ezQtLogMessageDock::currentTestResultChanged(const ezTestResultData* pTestResult)
{
  m_pModel->currentTestResultChanged(pTestResult);
  ListView->scrollToBottom();
}

void ezQtLogMessageDock::currentTestSelectionChanged(const ezTestResultData* pTestResult)
{
  m_pModel->currentTestSelectionChanged(pTestResult);
  ListView->scrollTo(m_pModel->GetLastIndexOfTestSelection(), QAbstractItemView::EnsureVisible);
  ListView->scrollTo(m_pModel->GetFirstIndexOfTestSelection(), QAbstractItemView::EnsureVisible);
}

////////////////////////////////////////////////////////////////////////
// ezQtLogMessageModel public functions
////////////////////////////////////////////////////////////////////////

ezQtLogMessageModel::ezQtLogMessageModel(QObject* pParent, const ezTestFrameworkResult* pResult)
  : QAbstractItemModel(pParent), m_pTestResult(pResult)
{
}

ezQtLogMessageModel::~ezQtLogMessageModel()
{
}

void ezQtLogMessageModel::resetModel()
{
  beginResetModel();
  currentTestResultChanged(nullptr);
  endResetModel();
}

QModelIndex ezQtLogMessageModel::GetFirstIndexOfTestSelection()
{
  if (pCurrentTestSelection == nullptr || pCurrentTestSelection->m_iFirstOutput == -1)
    return QModelIndex();

  ezInt32 iEntries = (ezInt32)m_VisibleEntries.size();
  for (int i = 0; i < iEntries; ++i)
  {
    if ((ezInt32)m_VisibleEntries[i] >= pCurrentTestSelection->m_iFirstOutput)
      return index(i, 0);
  }
  return index(rowCount() - 1, 0);
}

QModelIndex ezQtLogMessageModel::GetLastIndexOfTestSelection()
{
  if (pCurrentTestSelection == nullptr || pCurrentTestSelection->m_iLastOutput == -1)
    return QModelIndex();

  ezInt32 iEntries = (ezInt32)m_VisibleEntries.size();
  for (int i = 0; i < iEntries; ++i)
  {
    if ((ezInt32)m_VisibleEntries[i] >= pCurrentTestSelection->m_iLastOutput)
      return index(i, 0);
  }
  return index(rowCount() - 1, 0);
}

void ezQtLogMessageModel::currentTestResultChanged(const ezTestResultData* pTestResult)
{
  UpdateVisibleEntries();
  currentTestSelectionChanged(pTestResult);
}

void ezQtLogMessageModel::currentTestSelectionChanged(const ezTestResultData* pTestResult)
{
  pCurrentTestSelection = pTestResult;
  if (pCurrentTestSelection != nullptr)
  {
    dataChanged(index(pCurrentTestSelection->m_iFirstOutput, 0), index(pCurrentTestSelection->m_iLastOutput, 0));
  }
}


////////////////////////////////////////////////////////////////////////
// ezQtLogMessageModel QAbstractItemModel functions
////////////////////////////////////////////////////////////////////////

QVariant ezQtLogMessageModel::data(const QModelIndex& index, int role) const
{
  if (!index.isValid() || m_pTestResult == nullptr|| index.column() != 0)
    return QVariant();

  const ezInt32 iRow = index.row();
  if (iRow < 0 || iRow >= (ezInt32)m_VisibleEntries.size())
    return QVariant();

  const ezUInt32 uiLogIdx = m_VisibleEntries[iRow];
  const ezUInt8 uiIndention = m_VisibleEntriesIndention[iRow];
  const ezTestOutputMessage& Message = *m_pTestResult->GetOutputMessage(uiLogIdx);
  const ezTestErrorMessage* pError = (Message.m_iErrorIndex != -1) ? m_pTestResult->GetErrorMessage(Message.m_iErrorIndex) : nullptr;
  switch (role)
  {
  case Qt::DisplayRole:
    {
      if (pError != nullptr)
      {
        QString sBlockStart = QLatin1String("\n") % QString((uiIndention + 1) * 3, ' ');
        QString sBlockName = pError->m_sBlock.empty() ? QLatin1String("") : (sBlockStart % QLatin1String("Block: ") + QLatin1String(pError->m_sBlock.c_str()));
        QString sMessage = pError->m_sMessage.empty() ? QLatin1String("") : (sBlockStart % QLatin1String("Message: ") + QLatin1String(pError->m_sMessage.c_str()));
        QString sErrorMessage = QString(uiIndention * 3, ' ') % QString(Message.m_sMessage.c_str()) %
          sBlockName %
          sBlockStart % QLatin1String("File: ") % QLatin1String(pError->m_sFile.c_str()) %
          sBlockStart % QLatin1String("Line: ") % QString::number(pError->m_iLine) %
          sBlockStart % QLatin1String("Function: ") % QLatin1String(pError->m_sFunction.c_str()) %
          sMessage;
       
        return sErrorMessage;
      }
      return QString(uiIndention * 3, ' ') + QString(Message.m_sMessage.c_str());
    }
  case Qt::TextColorRole:
    {
      switch (Message.m_Type)
      {
      case ezTestOutput::BeginBlock:
      case ezTestOutput::Message:
        return QColor(Qt::yellow);
      case ezTestOutput::Error:
        return QColor(Qt::red);
      case ezTestOutput::Success:
        return QColor(Qt::green);
      case ezTestOutput::StartOutput:
      case ezTestOutput::EndBlock:
      case ezTestOutput::ImportantInfo:
      case ezTestOutput::Details:
      case ezTestOutput::Duration:
      case ezTestOutput::FinalResult:
        return QVariant();
      default:
        return QVariant();
      }
    }
  case Qt::BackgroundColorRole:
    {
      QPalette palette = QApplication::palette();
      if (pCurrentTestSelection != nullptr && pCurrentTestSelection->m_iFirstOutput != -1)
      {
        if (pCurrentTestSelection->m_iFirstOutput <= (ezInt32)uiLogIdx && (ezInt32)uiLogIdx <= pCurrentTestSelection->m_iLastOutput)
        {
          return palette.midlight().color();
        }
      }
      return palette.base().color();
    }
  //case Qt::DecorationRole:
  //  {
  //    switch (Message.m_Type)
  //    {
  //    case ezTestOutput::StartOutput:
  //    case ezTestOutput::BeginBlock:
  //    case ezTestOutput::EndBlock:
  //    case ezTestOutput::ImportantInfo:
  //    case ezTestOutput::Details:
  //    case ezTestOutput::Success:
  //    case ezTestOutput::Message:
  //      return QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation);
  //    case ezTestOutput::Error:
  //      return QApplication::style()->standardIcon(QStyle::SP_MessageBoxCritical);
  //    case ezTestOutput::Duration:
  //    case ezTestOutput::FinalResult:
  //      return QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation);
  //    }
  //  }
  default:
    return QVariant();
  }
}

Qt::ItemFlags ezQtLogMessageModel::flags(const QModelIndex& index) const
{
  if (!index.isValid() || m_pTestResult == nullptr)
    return 0;

  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant ezQtLogMessageModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
  {
    switch (section)
    {
    case 0:
      return QString("Log Entry");
    }
  }
  return QVariant();
}

QModelIndex ezQtLogMessageModel::index(int row, int column, const QModelIndex& parent) const
{
  if (parent.isValid() || m_pTestResult == nullptr || column != 0)
    return QModelIndex();

  return createIndex(row, column, row);
}

QModelIndex ezQtLogMessageModel::parent(const QModelIndex& index) const
{
  return QModelIndex();
}

int ezQtLogMessageModel::rowCount(const QModelIndex& parent) const
{
  if (parent.isValid() || m_pTestResult == nullptr)
    return 0;

  return (int)m_VisibleEntries.size();
}

int ezQtLogMessageModel::columnCount(const QModelIndex& parent) const
{
  return 1;
}


////////////////////////////////////////////////////////////////////////
// ezQtLogMessageModel private functions
////////////////////////////////////////////////////////////////////////

void ezQtLogMessageModel::UpdateVisibleEntries()
{
  m_VisibleEntries.clear();
  m_VisibleEntriesIndention.clear();
  if (m_pTestResult == nullptr)
    return;

  ezUInt8 uiIndention = 0;
  ezUInt32 uiEntries = m_pTestResult->GetOutputMessageCount();
  /// \todo filter out uninteresting messages
  for (ezUInt32 i = 0; i < uiEntries; ++i)
  {
    ezTestOutput::Enum Type = m_pTestResult->GetOutputMessage(i)->m_Type;
    if (Type == ezTestOutput::BeginBlock)
      uiIndention++;
    if (Type == ezTestOutput::EndBlock)
      uiIndention--;

    m_VisibleEntries.push_back(i);
    m_VisibleEntriesIndention.push_back(uiIndention);
  }
  beginResetModel();
  endResetModel();
}

#endif

EZ_STATICLINK_FILE(TestFramework, TestFramework_Framework_Qt_qtLogMessageDock);

