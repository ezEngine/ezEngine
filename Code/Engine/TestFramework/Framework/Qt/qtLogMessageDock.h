#pragma once
#ifdef EZ_USE_QT

#include <TestFramework/Basics.h>
#include <QDockWidget>
#include <QAbstractItemModel>
#include <Code/Engine/TestFramework/ui_qtLogMessageDock.h>
#include <vector>

class ezQtTestFramework;
struct ezTestResultData;
class ezQtLogMessageModel;
class ezTestFrameworkResult;

/// \brief Dock widget that lists the output of a given ezResult struct.
class EZ_TEST_DLL ezQtLogMessageDock : public QDockWidget, public Ui_qtLogMessageDock
{
  Q_OBJECT
public:
  ezQtLogMessageDock(QObject* pParent, const ezTestFrameworkResult* pResult);
  virtual ~ezQtLogMessageDock();

public slots:
  void resetModel();
  void currentTestResultChanged(const ezTestResultData* pTestResult);
  void currentTestSelectionChanged(const ezTestResultData* pTestResult);

private:
  ezQtLogMessageModel* m_pModel;
};

/// \brief Model used by ezQtLogMessageDock to list the output entries in ezResult.
class EZ_TEST_DLL ezQtLogMessageModel : public QAbstractItemModel
{
  Q_OBJECT
public:
  ezQtLogMessageModel(QObject* pParent, const ezTestFrameworkResult* pResult);
  virtual ~ezQtLogMessageModel();
  
  void resetModel();
  QModelIndex GetFirstIndexOfTestSelection();
  QModelIndex GetLastIndexOfTestSelection();

public slots:
  void currentTestResultChanged(const ezTestResultData* pTestResult);
  void currentTestSelectionChanged(const ezTestResultData* pTestResult);

public: //QAbstractItemModel interface
  virtual QVariant data(const QModelIndex& index, int role) const override;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const override;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
  virtual QModelIndex parent(const QModelIndex& index) const override;
  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;

private:
  void UpdateVisibleEntries();

private:
  const ezTestResultData* pCurrentTestSelection;
  const ezTestFrameworkResult* m_pTestResult;
  std::vector<ezUInt32> m_VisibleEntries;
  std::vector<ezUInt8> m_VisibleEntriesIndention;
};

#endif

