#include <TestFramework/PCH.h>
#ifdef EZ_USE_QT

#include <TestFramework/Framework/Qt/qtTestFramework.h>
#include <TestFramework/Framework/Qt/qtTestModel.h>
#include <QPalette>
#include <QApplication>
#include <QStringBuilder>

////////////////////////////////////////////////////////////////////////
// ezQtTestModelEntry public functions
////////////////////////////////////////////////////////////////////////

ezQtTestModelEntry::ezQtTestModelEntry()
  : m_pSubTestEntry(NULL), m_pTestEntry(NULL), m_pParentEntry(NULL), m_uiIndexInParent(0)
{
}

ezQtTestModelEntry::ezQtTestModelEntry(ezSubTestEntry* pSubTestEntry)
  : m_pSubTestEntry(pSubTestEntry), m_pTestEntry(NULL), m_pParentEntry(NULL), m_uiIndexInParent(0)
{
}

ezQtTestModelEntry::ezQtTestModelEntry(ezTestEntry* pTestEntry)
  : m_pSubTestEntry(NULL), m_pTestEntry(pTestEntry), m_pParentEntry(NULL), m_uiIndexInParent(0)
{
}

ezQtTestModelEntry::~ezQtTestModelEntry()
{
  ClearEntries();
}

void ezQtTestModelEntry::ClearEntries()
{
  for (ezInt32 i = (ezInt32)m_SubEntries.size() - 1; i >= 0; --i)
  {
    delete m_SubEntries[i];
  }
  m_SubEntries.clear();
}

ezUInt32 ezQtTestModelEntry::GetNumSubEntries() const
{
  return (ezInt32)m_SubEntries.size();
}

ezQtTestModelEntry* ezQtTestModelEntry::GetSubEntry(ezUInt32 uiIndex) const
{
  if (uiIndex >= GetNumSubEntries())
    return NULL;

  return m_SubEntries[uiIndex];
}

void ezQtTestModelEntry::AddSubEntry(ezQtTestModelEntry* pEntry)
{
  pEntry->m_pParentEntry = this;
  pEntry->m_uiIndexInParent = (ezUInt32)m_SubEntries.size();
  m_SubEntries.push_back(pEntry);
}

ezQtTestModelEntry::ezTestModelEntryType ezQtTestModelEntry::GetNodeType() const
{
  return (m_pTestEntry != NULL) ? TestNode : ((m_pSubTestEntry != NULL) ? SubTestNode : RootNode);
}

ezTestResult* ezQtTestModelEntry::GetTestResult() const
{
  switch (GetNodeType())
  {
  case ezQtTestModelEntry::SubTestNode:
    return &m_pSubTestEntry->m_Result;
  case ezQtTestModelEntry::TestNode:
    return &m_pTestEntry->m_Result;
  default:
    return NULL;
  }
}

static QColor ToneColor(const QColor& inputColor, const QColor& toneColor)
{
  qreal fHue = toneColor.hueF();
  qreal fSaturation = 1.0f;
  qreal fLightness = inputColor.lightnessF();
  fLightness = ezMath::Clamp(fLightness, 0.20, 0.80);
  return QColor::fromHslF(fHue, fSaturation, fLightness);
}

////////////////////////////////////////////////////////////////////////
// ezQtTestModel public functions
////////////////////////////////////////////////////////////////////////

ezQtTestModel::ezQtTestModel(QObject* pParent, ezQtTestFramework* pTestFramework)
  : QAbstractItemModel(pParent), m_pTestFramework(pTestFramework), m_Root()
{
  QPalette palette = QApplication::palette();

  // Derive state colors from the current active palette.
  m_SucessColor = ToneColor(palette.text().color(), QColor(Qt::green)).toRgb();
  m_FailedColor = ToneColor(palette.text().color(), QColor(Qt::red)).toRgb();

  m_TestColor = ToneColor(palette.base().color(), QColor(Qt::cyan)).toRgb();
  m_SubTestColor = ToneColor(palette.base().color(), QColor(Qt::blue)).toRgb();

  m_TestIcon = QIcon(":/Icons/Icons/pie.png");
  m_TestIconOff = QIcon(":/Icons/Icons/pie_off.png");

  UpdateModel();
}

ezQtTestModel::~ezQtTestModel()
{
  m_Root.ClearEntries();
}

void ezQtTestModel::Reset()
{
  beginResetModel();
  endResetModel();
}

void ezQtTestModel::InvalidateAll()
{
  dataChanged(QModelIndex(), QModelIndex());
}

void ezQtTestModel::TestDataChanged(ezInt32 iTestIndex, ezInt32 iSubTestIndex)
{
  QModelIndex TestModelIndex = index(iTestIndex, 0);
  // Invalidate whole test row
  emit dataChanged( TestModelIndex, index(iTestIndex, columnCount()-1) );
  
  // Invalidate all sub-tests
  const ezQtTestModelEntry* pEntry = (ezQtTestModelEntry*)TestModelIndex.internalPointer();
  ezInt32 iChildren = (ezInt32)pEntry->GetNumSubEntries();
  emit dataChanged( index(0, 0, TestModelIndex), index(iChildren-1, columnCount()-1, TestModelIndex) );
}


////////////////////////////////////////////////////////////////////////
// ezQtTestModel QAbstractItemModel functions
////////////////////////////////////////////////////////////////////////

QVariant ezQtTestModel::data(const QModelIndex& index, int role) const
{
  if (!index.isValid())
    return QVariant();

  const ezQtTestModelEntry* pEntry = (ezQtTestModelEntry*) index.internalPointer();
  const ezQtTestModelEntry* pParentEntry = pEntry->GetParentEntry();
  const ezQtTestModelEntry::ezTestModelEntryType entryType = pEntry->GetNodeType();
  
  const ezTestEntry* pTestEntry = NULL;
  const ezSubTestEntry* pSubTestEntry = NULL;
  bool bParentEnabled = true;
  bool bIsSubTest = entryType == ezQtTestModelEntry::SubTestNode;

  if (bIsSubTest)
  {
    pTestEntry = pParentEntry->GetTestEntry();
    pSubTestEntry = pEntry->GetSubTestEntry();
    bParentEnabled = pTestEntry->m_Result.m_bEnableTest;
  }
  else
  {
    pTestEntry = pEntry->GetTestEntry();
  }

  const ezTestResult& TestResult = *pEntry->GetTestResult();
  const char* szTestName = pSubTestEntry ? pSubTestEntry->m_szSubTestName : pTestEntry->m_szTestName;

  // Name
  if (index.column() == Columns::Name)
  {
    switch (role)
    {
    case Qt::DisplayRole:
      {
        return QString(szTestName);
      }
    case Qt::CheckStateRole:
      {
        return TestResult.m_bEnableTest ? Qt::Checked : Qt::Unchecked;
      }
    case Qt::DecorationRole:
      {
        return (TestResult.m_bEnableTest && bParentEnabled) ? m_TestIcon : m_TestIconOff;
      }
    default:
      return QVariant();
    }
  }
  // Status
  else if (index.column() == Columns::Status)
  {
    switch (role)
    {
    case Qt::DisplayRole:
      {
        if (TestResult.m_bEnableTest && bParentEnabled)
        {
          if (bIsSubTest)
          {
            return QString("Test Enabled");
          }
          else
          {
            // Count sub-test status
            const ezUInt32 iSubTests = (ezUInt32)pTestEntry->m_SubTests.size();
            ezUInt32 iEnabled = pTestEntry->GetSubTestCount(ezTestResultQuery::Enabled);
            return QString("%1 / %2 Enabled").arg(iEnabled).arg(iSubTests);
          }
        }
        else
        {
          return QString("Test Disabled");
        }
      }
    case Qt::TextAlignmentRole:
      {
        return Qt::AlignRight;
      }
    default:
      return QVariant();
    }
  }
  // Duration
  else if (index.column() == Columns::Duration)
  {
    switch (role)
    {
    case Qt::DisplayRole:
      {
        return QString::number(TestResult.m_fTestDuration, 'f', 4);
      }
    case Qt::TextAlignmentRole:
      {
        return Qt::AlignRight;
      }
    /*case Qt::BackgroundColorRole:
      {
        QPalette palette = QApplication::palette();
        return palette.alternateBase().color();
      }*/
    case UserRoles::Duration:
      {
        if (bIsSubTest && pTestEntry->m_Result.m_bExecuted)
        {
          return TestResult.m_fTestDuration / pTestEntry->m_Result.m_fTestDuration;
        }
        else if (TestResult.m_bExecuted)
        {
          return TestResult.m_fTestDuration / m_pTestFramework->GetTotalTestDuration();
        }
        return QVariant();
      }
    case UserRoles::DurationColor:
      {
        if (TestResult.m_bExecuted)
        {
          return (bIsSubTest ? m_SubTestColor : m_TestColor);
        }     
        return QVariant();
      }
    default:
      return QVariant();
    }
  }
  // Errors
  else if (index.column() == Columns::Errors)
  {
    switch (role)
    {
    case Qt::DisplayRole:
      {
        return QString("%1 / %2").arg(TestResult.m_uiErrorCount).arg(TestResult.m_TestOutput.size());
      }
    case Qt::BackgroundColorRole:
      {
        QPalette palette = QApplication::palette();
        return palette.alternateBase().color();
      }
    case Qt::TextColorRole:
      {
        if (TestResult.m_bExecuted)
        {
          return (TestResult.m_uiErrorCount == 0) ? m_SucessColor : m_FailedColor;
        }
        return QVariant();
      }
    case Qt::TextAlignmentRole:
      {
        return Qt::AlignRight;
      }

    default:
      return QVariant();
    }
  }
  // Progress
  else if (index.column() == Columns::Progress)
  {
    switch (role)
    {
    case Qt::DisplayRole:
      {
        if (TestResult.m_bEnableTest && bParentEnabled)
        {
          if (bIsSubTest)
          {
            if (TestResult.m_bExecuted)
            {
              return (TestResult.m_bSuccess) ? QString("Test Passed") : QString("Test Failed");
            }
            else
            {
              return QString("Test Pending");
            }
          }
          else
          {
            // Count sub-test status
            
            ezUInt32 iEnabled = pTestEntry->GetSubTestCount(ezTestResultQuery::Enabled);
            ezUInt32 iExecuted = pTestEntry->GetSubTestCount(ezTestResultQuery::Executed);
            ezUInt32 iSucceeded = pTestEntry->GetSubTestCount(ezTestResultQuery::Success);
           
            if (iExecuted == iEnabled)
            {
              return (iExecuted == iSucceeded) ? QString("Test Passed") : QString("Test Failed");
            }
            else
            {
              return QString("%1 / %2 Executed").arg(iExecuted).arg(iEnabled);
            }
          }
        }
        else
        {
          return QString("Test Disabled");
        }
      }
    case Qt::BackgroundColorRole:
      {
        QPalette palette = QApplication::palette();
        return palette.alternateBase().color();
      }
    case Qt::TextColorRole:
      {
        if (TestResult.m_bExecuted)
        {
          return TestResult.m_bSuccess ? m_SucessColor : m_FailedColor;
        }
        return QVariant();
      }
    case Qt::TextAlignmentRole:
      {
        return Qt::AlignRight;
      }

    default:
      return QVariant();
    }
  }

  return QVariant();
}

Qt::ItemFlags ezQtTestModel::flags(const QModelIndex& index) const
{
  if (!index.isValid())
    return 0;

  ezQtTestModelEntry* pEntry = (ezQtTestModelEntry*) index.internalPointer();
  if (pEntry == &m_Root)
    return 0;

  return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
}

QVariant ezQtTestModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
  {
    switch (section)
    {
    case Columns::Name:
      return QString("Name");
    case Columns::Status:
      return QString("Status");
    case Columns::Duration:
      return QString("Duration (ms)");
    case Columns::Errors:
      return QString("Errors / Output");
    case Columns::Progress:
      return QString("Progress");
    }
  }
  return QVariant();
}

QModelIndex ezQtTestModel::index(int row, int column, const QModelIndex& parent) const
{
  if (!hasIndex(row, column, parent))
    return QModelIndex();

  const ezQtTestModelEntry* pParent = NULL;

  if (!parent.isValid())
    pParent = &m_Root;
  else
    pParent = static_cast<ezQtTestModelEntry*>(parent.internalPointer());

  ezQtTestModelEntry* pEntry = pParent->GetSubEntry(row);
  return pEntry ? createIndex(row, column, pEntry) : QModelIndex();
}

QModelIndex ezQtTestModel::parent(const QModelIndex& index) const
{
  if (!index.isValid())
    return QModelIndex();

  ezQtTestModelEntry* pChild = static_cast<ezQtTestModelEntry*>(index.internalPointer());
  ezQtTestModelEntry* pParent = pChild->GetParentEntry();

  if (pParent == &m_Root)
    return QModelIndex();

  return createIndex(pParent->GetIndexInParent(), 0, pParent);
}

int ezQtTestModel::rowCount(const QModelIndex& parent) const
{
  if (parent.column() > 0)
    return 0;

  const ezQtTestModelEntry* pParent = NULL;

  if (!parent.isValid())
    pParent = &m_Root;
  else
    pParent = static_cast<ezQtTestModelEntry*>(parent.internalPointer());

  return pParent->GetNumSubEntries();
}

int ezQtTestModel::columnCount(const QModelIndex& parent) const
{
  return Columns::ColumnCount;
}

bool ezQtTestModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
  ezQtTestModelEntry* pEntry = static_cast<ezQtTestModelEntry*>(index.internalPointer());
  if (pEntry == NULL || index.column() != Columns::Name || role != Qt::CheckStateRole)
    return false;

  pEntry->GetTestResult()->m_bEnableTest = value.toBool();
  if (pEntry->GetNodeType() == ezQtTestModelEntry::TestNode)
  {
    TestDataChanged(pEntry->GetIndexInParent(), -1);
  }
  else
  {
    TestDataChanged(pEntry->GetParentEntry()->GetIndexInParent(), pEntry->GetIndexInParent());
  }
  return true;
}


////////////////////////////////////////////////////////////////////////
// ezQtTestModel public slots
////////////////////////////////////////////////////////////////////////

void ezQtTestModel::UpdateModel()
{
  m_Root.ClearEntries();
  const ezUInt32 uiTestCount = m_pTestFramework->GetTestCount();
  for (ezUInt32 uiTestIndex = 0; uiTestIndex < uiTestCount; ++uiTestIndex)
  {
    ezTestEntry* pTestEntry = m_pTestFramework->GetTest(uiTestIndex);
    ezQtTestModelEntry* pTestModelEntry = new ezQtTestModelEntry(pTestEntry);
    m_Root.AddSubEntry(pTestModelEntry);

    const ezUInt32 uiSubTestCount = (ezUInt32)pTestEntry->m_SubTests.size();
    for (ezUInt32 uiSubTest = 0; uiSubTest < uiSubTestCount; ++uiSubTest)
    {
      ezSubTestEntry* pSubTestEntry = &pTestEntry->m_SubTests[uiSubTest];
      ezQtTestModelEntry* pSubTestModelEntry = new ezQtTestModelEntry(pSubTestEntry);
      pTestModelEntry->AddSubEntry(pSubTestModelEntry);
    }
  }
  //reset();
}


#endif

EZ_STATICLINK_FILE(TestFramework, TestFramework_Framework_Qt_qtTestModel);

