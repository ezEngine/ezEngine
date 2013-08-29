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
  ezQtTestModelEntry();
  ezQtTestModelEntry(ezSubTestEntry* pSubTestEntry);
  ezQtTestModelEntry(ezTestEntry* pTestEntry);
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
  ezTestResult* GetTestResult() const;
  ezSubTestEntry* GetSubTestEntry() const { return m_pSubTestEntry; }
  ezTestEntry* GetTestEntry() const { return m_pTestEntry; }

private:
  ezSubTestEntry* m_pSubTestEntry;
  ezTestEntry* m_pTestEntry;
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
      Progress,
      ColumnCount,
    };
  };

public: //QAbstractItemModel interface
  virtual QVariant data(const QModelIndex& index, int role) const EZ_OVERRIDE;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const EZ_OVERRIDE;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const EZ_OVERRIDE;
  virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const EZ_OVERRIDE;
  virtual QModelIndex parent(const QModelIndex& index) const EZ_OVERRIDE;
  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const EZ_OVERRIDE;
  virtual int columnCount(const QModelIndex& parent = QModelIndex()) const EZ_OVERRIDE;
  virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) EZ_OVERRIDE;

public slots:
  void UpdateModel();

private:
  ezQtTestFramework* m_pTestFramework;
  ezQtTestModelEntry m_Root;
  QColor m_SucessColor;
  QColor m_FailedColor;
  QColor m_TestColor;
  QColor m_SubTestColor;
  QIcon m_TestIcon;
  QIcon m_TestIconOff;
};

#endif