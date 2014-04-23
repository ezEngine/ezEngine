#pragma once
#ifdef EZ_USE_QT

#include <TestFramework/Basics.h>
#include <TestFramework/Framework/Qt/qtTestFramework.h>
#include <QAbstractItemModel>
#include <QColor>
#include <QIcon>

class ezQtTestFramework;

/// \brief Helper class that stores the test hierarchy used in ezQtTestModel.
class ezQtTestModelEntry
{
public:
  ezQtTestModelEntry(const ezTestFrameworkResult* pResult, ezInt32 iTestIndex = -1, ezInt32 iSubTestIndex = -1);
  ~ezQtTestModelEntry();

private:
  ezQtTestModelEntry(ezQtTestModelEntry&);
  void operator=(ezQtTestModelEntry&);

public:
  enum ezTestModelEntryType
  {
    RootNode,
    TestNode,
    SubTestNode
  };

  void ClearEntries();
  ezUInt32 GetNumSubEntries() const;
  ezQtTestModelEntry* GetSubEntry(ezUInt32 uiIndex) const;
  void AddSubEntry(ezQtTestModelEntry* pEntry);
  ezQtTestModelEntry* GetParentEntry() const { return m_pParentEntry; }
  ezUInt32 GetIndexInParent() const { return m_uiIndexInParent; }
  ezTestModelEntryType GetNodeType() const;
  const ezTestResultData* GetTestResult() const;
  ezInt32 GetTestIndex() const { return m_iTestIndex; }
  ezInt32 GetSubTestIndex() const { return m_iSubTestIndex; }

private:
  const ezTestFrameworkResult* m_pResult;
  ezInt32 m_iTestIndex;
  ezInt32 m_iSubTestIndex; 

  ezQtTestModelEntry* m_pParentEntry;
  ezUInt32 m_uiIndexInParent;
  std::deque<ezQtTestModelEntry*> m_SubEntries;
};

/// \brief A Model that lists all unit tests and sub-tests in a tree.
class EZ_TEST_DLL ezQtTestModel : public QAbstractItemModel
{
  Q_OBJECT
public:
  ezQtTestModel(QObject* pParent, ezQtTestFramework* pTestFramework);
  virtual ~ezQtTestModel();

  void Reset();
  void InvalidateAll();
  void TestDataChanged(ezInt32 iTestIndex, ezInt32 iSubTestIndex);

  struct UserRoles
  {
    enum Enum
    {
      Duration = Qt::UserRole,
      DurationColor = Qt::UserRole + 1,
    };
  };

  struct Columns
  {
    enum Enum
    {
      Name = 0,
      Status,
      Duration,
      Errors,
      Asserts,
      Progress,
      ColumnCount,
    };
  };

public: //QAbstractItemModel interface
  virtual QVariant data(const QModelIndex& index, int role) const override;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const override;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
  virtual QModelIndex parent(const QModelIndex& index) const override;
  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

public slots:
  void UpdateModel();

private:
  ezQtTestFramework* m_pTestFramework;
  ezTestFrameworkResult* m_pResult;
  ezQtTestModelEntry m_Root;
  QColor m_SucessColor;
  QColor m_FailedColor;
  QColor m_TestColor;
  QColor m_SubTestColor;
  QIcon m_TestIcon;
  QIcon m_TestIconOff;
};

#endif

