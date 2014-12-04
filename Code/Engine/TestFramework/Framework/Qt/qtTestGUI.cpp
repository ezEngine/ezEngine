#include <TestFramework/PCH.h>

#ifdef EZ_USE_QT

#include <QtWidgets>
#include <QMessageBox>
#include <QStringBuilder>
#include <TestFramework/Framework/Qt/qtTestGUI.h>
#include <TestFramework/Framework/Qt/qtTestFramework.h>
#include <TestFramework/Framework/Qt/qtTestModel.h>
#include <TestFramework/Framework/Qt/qtTestDelegate.h>
#include <TestFramework/Framework/Qt/qtLogMessageDock.h>
#include <TestFramework/Utilities/TestOrder.h>

////////////////////////////////////////////////////////////////////////
// ezQtTestGUI public functions
////////////////////////////////////////////////////////////////////////

ezQtTestGUI::ezQtTestGUI(ezQtTestFramework& testFramework)
  : QMainWindow(), m_pTestFramework(&testFramework), m_pModel(nullptr), m_bExpandedCurrentTest(false), m_bAbort(false)
{
  this->setupUi(this);
  this->setWindowTitle(testFramework.GetTestName());

  QCoreApplication::setOrganizationDomain("www.ezengine.net");
  QCoreApplication::setOrganizationName("ezEngine Project");
  QCoreApplication::setApplicationName("ezTestFramework");
  QCoreApplication::setApplicationVersion("1.0.0");

  // Status Bar
  m_pStatusTextWorkState = new QLabel(this);
  testStatusBar->addWidget(m_pStatusTextWorkState);

  m_pStatusText = new QLabel(this);
  testStatusBar->addWidget(m_pStatusText);


  // Model
  m_pModel = new ezQtTestModel(this, m_pTestFramework);
  testTreeView->setModel(m_pModel);

  // Delegate
  m_pDelegate = new ezQtTestDelegate(this);

  // View
  //testTreeView->expandAll();
  testTreeView->resizeColumnToContents(4);
  testTreeView->resizeColumnToContents(3);
  testTreeView->resizeColumnToContents(2);
  testTreeView->resizeColumnToContents(1);
  testTreeView->resizeColumnToContents(0);
  testTreeView->header()->setStretchLastSection(true);
  testTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
  testTreeView->setUniformRowHeights(true);
  testTreeView->setItemDelegate(m_pDelegate);
  testTreeView->setSelectionMode(QAbstractItemView::SingleSelection);
  testTreeView->setSelectionBehavior(QAbstractItemView::SelectRows);

  // Message Log Dock
  m_pMessageLogDock = new ezQtLogMessageDock(this, &m_pTestFramework->GetTestResult());
  addDockWidget(Qt::RightDockWidgetArea, m_pMessageLogDock);

  // connect custom context menu
  connect(testTreeView, SIGNAL( customContextMenuRequested(const QPoint&) ), this, SLOT( onTestTreeViewCustomContextMenuRequested(const QPoint&) ));

  // connect current row changed signal
  QItemSelectionModel* pSelectionModel = testTreeView->selectionModel();
  connect(pSelectionModel, SIGNAL( currentRowChanged(const QModelIndex&, const QModelIndex&) ), this, SLOT( onSelectionModelCurrentRowChanged(const QModelIndex&) ));

  // Sync actions with test framework settings
  TestSettings settings = m_pTestFramework->GetSettings();
  this->actionAssertOnTestFail->setChecked(settings.m_bAssertOnTestFail);
  this->actionOpenHTMLOutput->setChecked(settings.m_bOpenHtmlOutput);
  this->actionKeepConsoleOpen->setChecked(settings.m_bKeepConsoleOpen);
  this->actionShowMessageBox->setChecked(settings.m_bShowMessageBox);

  // Hide the Windows console
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  if (!settings.m_bKeepConsoleOpen)
    ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif

  connect(m_pTestFramework, SIGNAL( TestResultReceived(qint32, qint32) ), this, SLOT( onTestFrameworkTestResultReceived(qint32, qint32) ) );

  UpdateButtonStates();
  LoadGUILayout();

  if (testFramework.GetSettings().m_bRunTests)
  {
    QTimer::singleShot(10, this, SLOT(on_actionRunTests_triggered()));
  }
}

ezQtTestGUI::~ezQtTestGUI()
{
  testTreeView->setModel(nullptr);
  testTreeView->setItemDelegate(nullptr);
  delete m_pModel;
  m_pModel = nullptr;
  delete m_pDelegate;
  m_pDelegate = nullptr;
}

void ezQtTestGUI::closeEvent(QCloseEvent* e)
{
  m_pTestFramework->SaveTestOrder();
  SaveGUILayout();
}

////////////////////////////////////////////////////////////////////////
// ezQtTestGUI public slots
////////////////////////////////////////////////////////////////////////

void ezQtTestGUI::on_actionAssertOnTestFail_triggered(bool bChecked)
{
  TestSettings settings = m_pTestFramework->GetSettings();
  settings.m_bAssertOnTestFail = bChecked;
  m_pTestFramework->SetSettings(settings);
}

void ezQtTestGUI::on_actionOpenHTMLOutput_triggered(bool bChecked)
{
  TestSettings settings = m_pTestFramework->GetSettings();
  settings.m_bOpenHtmlOutput = bChecked;
  m_pTestFramework->SetSettings(settings);
}

void ezQtTestGUI::on_actionKeepConsoleOpen_triggered(bool bChecked)
{
  TestSettings settings = m_pTestFramework->GetSettings();
  settings.m_bKeepConsoleOpen = bChecked;
  m_pTestFramework->SetSettings(settings);

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  if (!settings.m_bKeepConsoleOpen)
    ShowWindow(GetConsoleWindow(), SW_HIDE);
  else
    ShowWindow(GetConsoleWindow(), SW_SHOW);
#endif

}

void ezQtTestGUI::on_actionShowMessageBox_triggered(bool bChecked)
{
  TestSettings settings = m_pTestFramework->GetSettings();
  settings.m_bShowMessageBox = bChecked;
  m_pTestFramework->SetSettings(settings);
}

void ezQtTestGUI::on_actionRunTests_triggered()
{
  m_pTestFramework->SaveTestOrder();
  m_pModel->InvalidateAll();
  m_bExpandedCurrentTest = false;
  m_bAbort = false;
  m_pMessageLogDock->currentTestResultChanged(nullptr);

  m_pStatusTextWorkState->setText("<p><span style=\"font-weight:600; color:#ff5500;\" >  [Working...]</span></p>");

  // make sure we start with a clean state
  m_pTestFramework->ResetTests();

  while (m_pTestFramework->RunTestExecutionLoop() == ezTestAppRun::Continue)
  {
    UpdateButtonStates();

    if (m_bAbort)
    {
      m_pTestFramework->AbortTests();
    }
  }

  UpdateButtonStates();
  m_pMessageLogDock->currentTestResultChanged(nullptr);

  // MessageBox
  if (m_bAbort)
  {
    m_pStatusTextWorkState->setText("<p><span style=\"font-weight:600; color:#ff0000;\">  [Tests Aborted]</span></p>");

    if (m_pTestFramework->GetSettings().m_bShowMessageBox)
      QMessageBox::information(this, "Tests Aborted", "The tests were aborted by the user.", QMessageBox::Ok, QMessageBox::Ok);

    m_bAbort = false;
  }
  else
  {
    if (m_pTestFramework->GetTotalErrorCount() > 0)
    {
      m_pStatusTextWorkState->setText("<p><span style=\"font-weight:600; color:#ff0000;\">  [Tests Failed]</span></p>");

      if (m_pTestFramework->GetSettings().m_bShowMessageBox)
        QMessageBox::critical(this, "Tests Failed", "Some tests have failed.", QMessageBox::Ok, QMessageBox::Ok);
    }
    else
    {
      m_pStatusTextWorkState->setText("<p><span style=\"font-weight:600; color:#00aa00;\">  [All Tests Passed]</span></p>");

      if (m_pTestFramework->GetSettings().m_bShowMessageBox)
        QMessageBox::information(this, "Tests Succeeded", "All tests succeeded.", QMessageBox::Ok, QMessageBox::Ok);

      if (m_pTestFramework->GetSettings().m_bCloseOnSuccess)
        QTimer::singleShot(100, this, SLOT(on_actionQuit_triggered()));
    }
  }
}

void ezQtTestGUI::on_actionAbort_triggered()
{
  m_bAbort = true;
}

void ezQtTestGUI::on_actionQuit_triggered()
{
  close();
}

void ezQtTestGUI::on_actionEnableOnlyThis_triggered()
{
  QModelIndex CurrentIndex = testTreeView->currentIndex();
  if (!CurrentIndex.isValid())
    return;

  // Need to set data on column 0
  CurrentIndex = m_pModel->index(CurrentIndex.row(), 0, CurrentIndex.parent());

  m_pTestFramework->SetAllTestsEnabledStatus(false);
  EnableAllParents(CurrentIndex);
  SetCheckStateRecursive(CurrentIndex, true);

  m_pModel->dataChanged(QModelIndex(), QModelIndex());
}

void ezQtTestGUI::on_actionEnableAllChildren_triggered()
{
  QModelIndex CurrentIndex = testTreeView->currentIndex();
  if (!CurrentIndex.isValid())
    return;
  
  // Need to set data on column 0
  CurrentIndex = m_pModel->index(CurrentIndex.row(), 0, CurrentIndex.parent());
  SetCheckStateRecursive(CurrentIndex, true);
}

void ezQtTestGUI::on_actionEnableAll_triggered()
{
  m_pTestFramework->SetAllTestsEnabledStatus(true);
  m_pModel->dataChanged(QModelIndex(),QModelIndex());
}

void ezQtTestGUI::on_actionDisableAll_triggered()
{
  m_pTestFramework->SetAllTestsEnabledStatus(false);
  m_pModel->dataChanged(QModelIndex(),QModelIndex());
}

void ezQtTestGUI::on_actionExpandAll_triggered()
{
  testTreeView->expandAll();
}

void ezQtTestGUI::on_actionCollapseAll_triggered()
{
  testTreeView->collapseAll();
}

void ezQtTestGUI::onTestFrameworkTestResultReceived(qint32 iTestIndex, qint32 iSubTestIndex)
{
  m_pModel->TestDataChanged(iTestIndex, iSubTestIndex);

  QModelIndex TestModelIndex = m_pModel->index(iTestIndex, 0);
  QModelIndex LastSubTest = m_pModel->index(m_pModel->rowCount(TestModelIndex)-1, 0, TestModelIndex);

  if (iSubTestIndex != -1)
  {
    bool bExpanded = testTreeView->isExpanded(TestModelIndex);
    if (!bExpanded)
    {
      // Remember if we expanded the test so we can close it again once the tests is done.
      testTreeView->expand(TestModelIndex);
      m_bExpandedCurrentTest = true;
    }
    testTreeView->scrollTo(LastSubTest);
  }
  else if (m_bExpandedCurrentTest)
  {
    m_bExpandedCurrentTest = false;
    testTreeView->collapse(TestModelIndex);
  }

  // Update status bar
  const ezUInt32 uiTestCount = m_pTestFramework->GetTestCount();
  const ezUInt32 uiFailed = m_pTestFramework->GetTestsFailedCount();
  const ezUInt32 uiPassed = m_pTestFramework->GetTestsPassedCount();
  const ezUInt32 uiErrors = m_pTestFramework->GetTotalErrorCount();
  double fTestDurationInSeconds = m_pTestFramework->GetTotalTestDuration() / 1000.0;

  // Get the current test's sub-test completion ratio
  float fSubTestPercentage = 0.0f;
  if (iTestIndex != -1 && iSubTestIndex != -1)
  {
    fSubTestPercentage = (float)m_pTestFramework->GetTestResult().GetSubTestCount(m_pTestFramework->GetCurrentTestIndex(), ezTestResultQuery::Executed)
      / (float)m_pTestFramework->GetSubTestEnabledCount(m_pTestFramework->GetCurrentTestIndex());
  }

  float fProgress = 100.0f * (fSubTestPercentage + uiFailed + uiPassed) / uiTestCount;
  QString sStatusText = QLatin1String("[progress: ") % QString::number(fProgress, 'f', 2) % QLatin1String("%] [passed: ") % QString::number(uiPassed)
    % QLatin1String("] [failed: ") % QString::number(uiFailed) % QLatin1String("] [errors: ") % QString::number(uiErrors) % QLatin1String("] [time taken: ")
    % QString::number(fTestDurationInSeconds, 'f', 2) % QLatin1String(" seconds]");

  m_pStatusText->setText(sStatusText);
  m_pMessageLogDock->currentTestResultChanged(&m_pTestFramework->GetTestResult().GetTestResultData(iTestIndex, iSubTestIndex));

  QApplication::processEvents();
}

void ezQtTestGUI::onTestTreeViewCustomContextMenuRequested(const QPoint& pnt)
{
  QModelIndex CurrentIndex = testTreeView->currentIndex();

  QMenu ContextMenu;
  if (CurrentIndex.isValid())
  {
    ContextMenu.addAction(actionEnableOnlyThis);
    ContextMenu.addAction(actionEnableAllChildren);
  }
  ContextMenu.addAction(actionEnableAll);
  ContextMenu.addAction(actionDisableAll);
  ContextMenu.addSeparator();
  ContextMenu.addAction(actionExpandAll);
  ContextMenu.addAction(actionCollapseAll);

  ContextMenu.exec(testTreeView->viewport()->mapToGlobal(pnt));
}

void ezQtTestGUI::onSelectionModelCurrentRowChanged(const QModelIndex& index)
{
  if (!index.isValid())
  {
    m_pMessageLogDock->currentTestSelectionChanged(nullptr);
  }

  const ezQtTestModelEntry* pEntry = (ezQtTestModelEntry*) index.internalPointer();
  const ezTestResultData* pTestResult = pEntry->GetTestResult();

  m_pMessageLogDock->currentTestSelectionChanged(pTestResult);
}


////////////////////////////////////////////////////////////////////////
// ezQtTestGUI private functions
////////////////////////////////////////////////////////////////////////

void ezQtTestGUI::UpdateButtonStates()
{
  bool bTestsRunning = m_pTestFramework->GetTestsRunning();

  pushButtonQuit->setEnabled(!bTestsRunning);
  pushButtonAbort->setEnabled(bTestsRunning);
  pushButtonRunTests->setEnabled(!bTestsRunning);

  actionQuit->setEnabled(!bTestsRunning);
  actionAbort->setEnabled(bTestsRunning);
  actionRunTests->setEnabled(!bTestsRunning);
}

void ezQtTestGUI::SaveGUILayout()
{
  QSettings Settings;

  Settings.beginGroup("MainWindow");

  Settings.setValue("WindowGeometry", saveGeometry());
  Settings.setValue("WindowState", saveState());
  Settings.setValue("IsMaximized", isMaximized());

  if (!isMaximized()) 
  {
    Settings.setValue("WindowPosition", pos());
    Settings.setValue("WindowSize", size());
  }

  Settings.endGroup();
}

void ezQtTestGUI::LoadGUILayout()
{
  QSettings Settings;
  Settings.beginGroup("MainWindow");

  restoreGeometry(Settings.value("WindowGeometry", saveGeometry() ).toByteArray());
  restoreState(Settings.value("WindowState", saveState()).toByteArray());
  move(Settings.value("WindowPosition", pos()).toPoint());
  resize(Settings.value("WindowSize", size()).toSize());

  if (Settings.value("IsMaximized", isMaximized()).toBool())
    showMaximized();

  Settings.endGroup();
}

void ezQtTestGUI::SetCheckStateRecursive(const QModelIndex& index, bool bChecked)
{
  m_pModel->setData(index, bChecked ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole);

  ezInt32 iChildren = m_pModel->rowCount(index);
  for (ezInt32 i = 0; i < iChildren; ++i)
  {
    SetCheckStateRecursive(index.child(i, 0), bChecked);
  }
}

void ezQtTestGUI::EnableAllParents(const QModelIndex& index)
{ 
  QModelIndex ParentIndex = m_pModel->parent(index);
  while (ParentIndex.isValid())
  {
    m_pModel->setData(ParentIndex, Qt::Checked, Qt::CheckStateRole);
    ParentIndex = m_pModel->parent(ParentIndex);
  }
}


////////////////////////////////////////////////////////////////////////
// ezQtTestGUI public static functions
////////////////////////////////////////////////////////////////////////

void ezQtTestGUI::SetDarkTheme()
{
  //return;
  QApplication::setStyle(QStyleFactory::create("fusion"));
  //return;
  QPalette palette;

  palette.setColor(QPalette::WindowText, QColor(200, 200, 200, 255));
  palette.setColor(QPalette::Button, QColor(100, 100, 100, 255));
  palette.setColor(QPalette::Light, QColor(97, 97, 97, 255));
  palette.setColor(QPalette::Midlight, QColor(59, 59, 59, 255));
  palette.setColor(QPalette::Dark, QColor(37, 37, 37, 255));
  palette.setColor(QPalette::Mid, QColor(45, 45, 45, 255));
  palette.setColor(QPalette::Text, QColor(200, 200, 200, 255));
  palette.setColor(QPalette::BrightText, QColor(37, 37, 37, 255));
  palette.setColor(QPalette::ButtonText, QColor(200, 200, 200, 255));
  palette.setColor(QPalette::Base, QColor(40, 40, 40, 255));
  palette.setColor(QPalette::Window, QColor(68, 68, 68, 255));
  palette.setColor(QPalette::Shadow, QColor(0, 0, 0, 255));
  palette.setColor(QPalette::Highlight, QColor(103, 141, 178, 255));
  palette.setColor(QPalette::HighlightedText, QColor(255, 255, 255, 255));
  palette.setColor(QPalette::Link, QColor(0, 0, 238, 255));
  palette.setColor(QPalette::LinkVisited, QColor(82, 24, 139, 255));
  palette.setColor(QPalette::AlternateBase, QColor(46, 46, 46, 255));
  QBrush NoRoleBrush(QColor(0, 0, 0, 255), Qt::NoBrush);
  palette.setBrush(QPalette::NoRole, NoRoleBrush);
  palette.setColor(QPalette::ToolTipBase, QColor(255, 255, 220, 255));
  palette.setColor(QPalette::ToolTipText, QColor(0, 0, 0, 255));

  palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(128, 128, 128, 255));
  palette.setColor(QPalette::Disabled, QPalette::Button, QColor(80, 80, 80, 255));
  palette.setColor(QPalette::Disabled, QPalette::Text, QColor(105, 105, 105, 255));
  palette.setColor(QPalette::Disabled, QPalette::BrightText, QColor(255, 255, 255, 255));
  palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(128, 128, 128, 255));
  palette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(86, 117, 148, 255));

  QApplication::setPalette(palette);
}
#endif


EZ_STATICLINK_FILE(TestFramework, TestFramework_Framework_Qt_qtTestGUI);

