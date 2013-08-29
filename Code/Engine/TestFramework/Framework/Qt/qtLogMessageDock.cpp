#include <TestFramework/PCH.h>
#ifdef EZ_USE_QT

#include <TestFramework/Framework/Qt/qtLogMessageDock.h>
#include <TestFramework/Framework/TestFramework.h>
#include <QApplication>

////////////////////////////////////////////////////////////////////////
// ezQtLogMessageDock public functions
////////////////////////////////////////////////////////////////////////

ezQtLogMessageDock::ezQtLogMessageDock(QObject* pParent)
{
  setupUi(this);
  m_pModel = new ezQtLogMessageModel(this);
  ListView->setModel(m_pModel);
}

ezQtLogMessageDock::~ezQtLogMessageDock()
{
  ListView->setModel(NULL);
  delete m_pModel;
  m_pModel = NULL;
}

void ezQtLogMessageDock::currentTestResultChanged(const ezTestResult* pTestResult)
{
  m_pModel->currentTestResultChanged(pTestResult);
}

////////////////////////////////////////////////////////////////////////
// ezQtLogMessageModel public functions
////////////////////////////////////////////////////////////////////////

ezQtLogMessageModel::ezQtLogMessageModel(QObject* pParent) : QAbstractItemModel(pParent), m_pTestResult(NULL)
{
}

ezQtLogMessageModel::~ezQtLogMessageModel()
{
}

void ezQtLogMessageModel::currentTestResultChanged(const ezTestResult* pTestResult)
{
  m_pTestResult = pTestResult;
  UpdateVisibleEntries();
}


////////////////////////////////////////////////////////////////////////
// ezQtLogMessageModel QAbstractItemModel functions
////////////////////////////////////////////////////////////////////////

QVariant ezQtLogMessageModel::data(const QModelIndex& index, int role) const
{
  if (!index.isValid() || m_pTestResult == NULL|| index.column() != 0)
    return QVariant();

  ezInt32 iRow = index.row();
  if (iRow < 0 || iRow >= (ezInt32)m_VisibleEntries.size())
    return QVariant();

  ezUInt32 uiLogIdx = m_VisibleEntries[iRow];
  const ezTestOutputMessage& Message = m_pTestResult->m_TestOutput[uiLogIdx];

  switch (role)
  {
  case Qt::DisplayRole:
    {
      return QString(Message.m_sMessage.c_str());
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
  if (!index.isValid() || m_pTestResult == NULL)
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
  if (parent.isValid() || m_pTestResult == NULL || column != 0)
    return QModelIndex();

  return createIndex(row, column, row);
}

QModelIndex ezQtLogMessageModel::parent(const QModelIndex& index) const
{
  return QModelIndex();
}

int ezQtLogMessageModel::rowCount(const QModelIndex& parent) const
{
  if (parent.isValid() || m_pTestResult == NULL)
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
  if (m_pTestResult == NULL)
    return;

  ezUInt32 uiEntries = (ezUInt32)m_pTestResult->m_TestOutput.size();
  // TODO: filter out uninteresting messages
  for (ezUInt32 i = 0; i < uiEntries; ++i)
  {
    m_VisibleEntries.push_back(i);
  }
  beginResetModel();
  endResetModel();
}

#endif