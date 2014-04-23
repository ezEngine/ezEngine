#pragma once
#ifdef EZ_USE_QT

#include <TestFramework/Basics.h>
#include <QMainWindow>
#include <Code/Engine/TestFramework/ui_qtTestGUI.h>

class ezQtTestFramework;
class ezQtTestModel;
class ezQtTestDelegate;
class ezQtLogMessageDock;
class QLabel;
class QPoint;

/// \brief Main window for the test framework GUI.
class EZ_TEST_DLL ezQtTestGUI : public QMainWindow, public Ui_qtTestGUI
{
  Q_OBJECT
public:
  ezQtTestGUI(ezQtTestFramework& testFramework);
  ~ezQtTestGUI();

private:
  ezQtTestGUI(ezQtTestGUI&);
  void operator=(ezQtTestGUI&);

private slots:
  void on_actionAssertOnTestFail_triggered(bool bChecked);
  void on_actionOpenHTMLOutput_triggered(bool bChecked);
  void on_actionKeepConsoleOpen_triggered(bool bChecked);
  void on_actionShowMessageBox_triggered(bool bChecked);
  void on_actionRunTests_triggered();
  void on_actionAbort_triggered();
  void on_actionQuit_triggered();

  void on_actionEnableOnlyThis_triggered();
  void on_actionEnableAllChildren_triggered();
  void on_actionEnableAll_triggered();
  void on_actionDisableAll_triggered();

  void on_actionExpandAll_triggered();
  void on_actionCollapseAll_triggered();

  void onTestFrameworkTestResultReceived(qint32 iTestIndex, qint32 iSubTestIndex);
  void onTestTreeViewCustomContextMenuRequested(const QPoint& pnt);

  void onSelectionModelCurrentRowChanged(const QModelIndex& index);

private:
  void UpdateButtonStates();
  void SaveGUILayout();
  void LoadGUILayout();

  void SetCheckStateRecursive(const QModelIndex& index, bool bChecked);
  void EnableAllParents(const QModelIndex& index);

public:
  static void SetDarkTheme();

protected:
  virtual void closeEvent(QCloseEvent* e) override;

private:
  ezQtTestFramework* m_pTestFramework;
  ezQtTestModel* m_pModel;
  ezQtTestDelegate* m_pDelegate;
  ezQtLogMessageDock* m_pMessageLogDock;
  QLabel* m_pStatusTextWorkState;
  QLabel* m_pStatusText;
  ezInt32 m_bExpandedCurrentTest;
  bool m_bAbort;
};

#endif

