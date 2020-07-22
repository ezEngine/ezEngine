#pragma once

#ifdef EZ_USE_QT

#  include <QMainWindow>
#  include <TestFramework/ui_qtTestGUI.h>

#  include <TestFramework/TestFrameworkDLL.h>

#  if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#    define USE_WIN_EXTRAS EZ_ON
#  else
#    define USE_WIN_EXTRAS EZ_OFF
#  endif


class ezQtTestFramework;
class ezQtTestModel;
class ezQtTestDelegate;
class ezQtLogMessageDock;
class QLabel;
class QPoint;

QT_BEGIN_NAMESPACE

class QWinTaskbarProgress;
class QWinTaskbarButton;

QT_END_NAMESPACE

QT_USE_NAMESPACE

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

private Q_SLOTS:
  void on_actionAssertOnTestFail_triggered(bool bChecked);
  void on_actionOpenHTMLOutput_triggered(bool bChecked);
  void on_actionKeepConsoleOpen_triggered(bool bChecked);
  void on_actionShowMessageBox_triggered(bool bChecked);
  void on_actionDisableSuccessfulTests_triggered(bool bChecked);
  void on_actionSaveTestSettingsAs_triggered();
  void on_actionSaveTestOrderAs_triggered();
  void on_actionRunTests_triggered();
  void on_actionAbort_triggered();
  void on_actionQuit_triggered();

  void on_actionEnableOnlyThis_triggered();
  void on_actionEnableOnlyFailed_triggered();
  void on_actionEnableAllChildren_triggered();
  void on_actionEnableAll_triggered();
  void on_actionDisableAll_triggered();

  void on_actionExpandAll_triggered();
  void on_actionCollapseAll_triggered();

  void onTestFrameworkTestResultReceived(qint32 iTestIndex, qint32 iSubTestIndex);
  void onTestTreeViewCustomContextMenuRequested(const QPoint& pnt);

  void onSelectionModelCurrentRowChanged(const QModelIndex& index);

  void on_actionOpenTestDataFolder_triggered();
  void on_actionOpenOutputFolder_triggered();
  void on_actionOpenHTMLFile_triggered();
  void on_actionUpdateReferenceImages_triggered();

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
  ezQtTestFramework* m_pTestFramework = nullptr;
  ezQtTestModel* m_pModel = nullptr;
  ezQtTestDelegate* m_pDelegate = nullptr;
  ezQtLogMessageDock* m_pMessageLogDock = nullptr;
  QLabel* m_pStatusTextWorkState = nullptr;
  QLabel* m_pStatusText = nullptr;
  bool m_bExpandedCurrentTest = false;
  bool m_bAbort = false;
  ezUInt32 m_uiTestsEnabledCount = 0;
  ezUInt32 m_uiSubTestsEnabledCount = 0;

#  if EZ_ENABLED(USE_WIN_EXTRAS)
  QWinTaskbarButton* m_pWinTaskBarButton = nullptr;
  QWinTaskbarProgress* m_pWinTaskBarProgress = nullptr;
  QMetaObject::Connection m_OnButtonDestroyed;
  QMetaObject::Connection m_OnProgressDestroyed;
#  endif
};

#endif
