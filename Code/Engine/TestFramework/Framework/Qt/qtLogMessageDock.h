#pragma once
#ifdef EZ_USE_QT

#include <TestFramework/Basics.h>
#include <QDockWidget>
#include <QAbstractItemModel>
#include <Code/Engine/TestFramework/ui_qtLogMessageDock.h>
#include <vector>

class ezQtTestFramework;
struct ezTestResult;
class ezQtLogMessageModel;

/// \brief Dock widget that lists the output of a given ezTestResult struct.
class EZ_TEST_DLL ezQtLogMessageDock : public QDockWidget, public Ui_qtLogMessageDock
{
  Q_OBJECT
public:
  ezQtLogMessageDock(QObject* pParent);
  virtual ~ezQtLogMessageDock();

public slots:
  void currentTestResultChanged(const ezTestResult* pTestResult);

private:
  ezQtLogMessageModel* m_pModel;
};

/// \brief Model used by ezQtLogMessageDock to list the output entries in ezTestResult.
class EZ_TEST_DLL ezQtLogMessageModel : public QAbstractItemModel
{
  Q_OBJECT
public:
  ezQtLogMessageModel(QObject* pParent);
  virtual ~ezQtLogMessageModel();

public slots:
  void currentTestResultChanged(const ezTestResult* pTestResult);

public: //QAbstractItemModel interface
  virtual QVariant data(const QModelIndex& index, int role) const EZ_OVERRIDE;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const EZ_OVERRIDE;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const EZ_OVERRIDE;
  virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const EZ_OVERRIDE;
  virtual QModelIndex parent(const QModelIndex& index) const EZ_OVERRIDE;
  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const EZ_OVERRIDE;
  virtual int columnCount(const QModelIndex& parent = QModelIndex()) const EZ_OVERRIDE;

private:
  void UpdateVisibleEntries();

private:
  const ezTestResult* m_pTestResult;
  std::vector<ezUInt32> m_VisibleEntries;
};

#endif

