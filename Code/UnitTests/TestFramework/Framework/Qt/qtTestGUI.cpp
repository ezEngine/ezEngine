#include <TestFrameworkPCH.h>

#ifdef EZ_USE_QT

#  include <QtWidgets>

#  include <Foundation/Strings/PathUtils.h>

#  include <TestFramework/Framework/Qt/qtLogMessageDock.h>
#  include <TestFramework/Framework/Qt/qtTestDelegate.h>
#  include <TestFramework/Framework/Qt/qtTestGUI.h>
#  include <TestFramework/Framework/Qt/qtTestModel.h>

#  if EZ_ENABLED(USE_WIN_EXTRAS)
#    include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#    include <QtWinExtras/QWinTaskbarButton>
#    include <QtWinExtras/QWinTaskbarProgress>
#  endif

////////////////////////////////////////////////////////////////////////
// ezQtTestGUI public functions
////////////////////////////////////////////////////////////////////////

ezQtTestGUI::ezQtTestGUI(ezQtTestFramework& testFramework)
  : QMainWindow()
  , m_pTestFramework(&testFramework)
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
  // testTreeView->expandAll();
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
  connect(
    testTreeView, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(onTestTreeViewCustomContextMenuRequested(const QPoint&)));

  // connect current row changed signal
  QItemSelectionModel* pSelectionModel = testTreeView->selectionModel();
  connect(pSelectionModel, SIGNAL(currentRowChanged(const QModelIndex&, const QModelIndex&)), this,
    SLOT(onSelectionModelCurrentRowChanged(const QModelIndex&)));

  // Sync actions with test framework settings
  TestSettings settings = m_pTestFramework->GetSettings();
  this->actionAssertOnTestFail->setChecked(settings.m_bAssertOnTestFail);
  this->actionOpenHTMLOutput->setChecked(settings.m_bOpenHtmlOutputOnError);
  this->actionKeepConsoleOpen->setChecked(settings.m_bKeepConsoleOpen);
  this->actionShowMessageBox->setChecked(settings.m_bShowMessageBox);
  this->actionDisableSuccessfulTests->setChecked(settings.m_bAutoDisableSuccessfulTests);

  // Hide the Windows console
#  if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  if (!settings.m_bKeepConsoleOpen)
    ShowWindow(GetConsoleWindow(), SW_HIDE);
#  endif

  connect(m_pTestFramework, SIGNAL(TestResultReceived(qint32, qint32)), this, SLOT(onTestFrameworkTestResultReceived(qint32, qint32)));

  UpdateButtonStates();
  LoadGUILayout();

  if (testFramework.GetSettings().m_bRunTests)
  {
    QTimer::singleShot(10, this, SLOT(on_actionRunTests_triggered()));
  }
}

ezQtTestGUI::~ezQtTestGUI()
{
#  if EZ_ENABLED(USE_WIN_EXTRAS)
  if (m_pWinTaskBarProgress)
  {
    QObject::disconnect(m_OnProgressDestroyed);
    m_pWinTaskBarProgress->hide();
    m_pWinTaskBarProgress = nullptr;
  }

  if (m_pWinTaskBarButton)
  {
    QObject::disconnect(m_OnButtonDestroyed);
    delete m_pWinTaskBarButton;
    m_pWinTaskBarButton = nullptr;
  }
#  endif

  testTreeView->setModel(nullptr);
  testTreeView->setItemDelegate(nullptr);
  delete m_pModel;
  m_pModel = nullptr;
  delete m_pDelegate;
  m_pDelegate = nullptr;
}

void ezQtTestGUI::closeEvent(QCloseEvent* e)
{
  m_pTestFramework->AutoSaveTestOrder();
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
  settings.m_bOpenHtmlOutputOnError = bChecked;
  m_pTestFramework->SetSettings(settings);
}

void ezQtTestGUI::on_actionKeepConsoleOpen_triggered(bool bChecked)
{
  TestSettings settings = m_pTestFramework->GetSettings();
  settings.m_bKeepConsoleOpen = bChecked;
  m_pTestFramework->SetSettings(settings);

#  if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  if (!settings.m_bKeepConsoleOpen)
    ShowWindow(GetConsoleWindow(), SW_HIDE);
  else
    ShowWindow(GetConsoleWindow(), SW_SHOW);
#  endif
}

void ezQtTestGUI::on_actionShowMessageBox_triggered(bool bChecked)
{
  TestSettings settings = m_pTestFramework->GetSettings();
  settings.m_bShowMessageBox = bChecked;
  m_pTestFramework->SetSettings(settings);
}

void ezQtTestGUI::on_actionDisableSuccessfulTests_triggered(bool bChecked)
{
  TestSettings settings = m_pTestFramework->GetSettings();
  settings.m_bAutoDisableSuccessfulTests = bChecked;
  m_pTestFramework->SetSettings(settings);
}

void ezQtTestGUI::on_actionSaveTestSettingsAs_triggered()
{
  ezStringBuilder tmp;
  ezStringView defaultDir = ezPathUtils::GetFileDirectory(m_pTestFramework->GetAbsTestSettingsFilePath());
  QString dir = defaultDir.IsValid() ? defaultDir.GetData(tmp) : QString();
  QString sSavePath = QFileDialog::getSaveFileName(this, QLatin1String("Save Test Settings As"), dir);
  if (!sSavePath.isEmpty())
  {
    m_pTestFramework->SaveTestOrder(sSavePath.toUtf8().data());
  }
}

void ezQtTestGUI::on_actionRunTests_triggered()
{
  // For some reason during tests 'run tests' is called again
  // while already running tests  so we early out here.
  if (m_pTestFramework->GetTestsRunning())
    return;

  m_uiTestsEnabledCount = m_pTestFramework->GetTestEnabledCount();
  m_uiSubTestsEnabledCount = 0;

  m_pTestFramework->AutoSaveTestOrder();
  m_pModel->InvalidateAll();
  m_bExpandedCurrentTest = false;
  m_bAbort = false;
  m_pMessageLogDock->currentTestResultChanged(nullptr);

  m_pStatusTextWorkState->setText("<p><span style=\"font-weight:600; color:#ff5500;\" >  [Working...]</span></p>");

  // make sure we start with a clean state
  m_pTestFramework->ResetTests();
  m_pMessageLogDock->resetModel();
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

#  if EZ_ENABLED(USE_WIN_EXTRAS)
    if (m_pWinTaskBarProgress)
    {
      m_pWinTaskBarProgress->pause();
    }
#  endif
  }
  else
  {
    if (m_pTestFramework->GetTotalErrorCount() > 0)
    {
      m_pStatusTextWorkState->setText("<p><span style=\"font-weight:600; color:#ff0000;\">  [Tests Failed]</span></p>");

      if (m_pTestFramework->GetSettings().m_bShowMessageBox)
        QMessageBox::critical(this, "Tests Failed", "Some tests have failed.", QMessageBox::Ok, QMessageBox::Ok);

#  if EZ_ENABLED(USE_WIN_EXTRAS)
      if (m_pWinTaskBarProgress)
      {
        m_pWinTaskBarProgress->stop();
      }
#  endif
    }
    else
    {
      m_pStatusTextWorkState->setText("<p><span style=\"font-weight:600; color:#00aa00;\">  [All Tests Passed]</span></p>");

      if (m_pTestFramework->GetSettings().m_bShowMessageBox)
        QMessageBox::information(this, "Tests Succeeded", "All tests succeeded.", QMessageBox::Ok, QMessageBox::Ok);

      if (m_pTestFramework->GetSettings().m_bCloseOnSuccess)
        QTimer::singleShot(100, this, SLOT(on_actionQuit_triggered()));

#  if EZ_ENABLED(USE_WIN_EXTRAS)
      if (m_pWinTaskBarProgress)
      {
        m_pWinTaskBarProgress->reset();
      }
#  endif
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
  SetCheckStateRecursive(CurrentIndex, true);
  EnableAllParents(CurrentIndex);

  m_pModel->dataChanged(QModelIndex(), QModelIndex());
}


void ezQtTestGUI::on_actionEnableOnlyFailed_triggered()
{
  m_pTestFramework->SetAllFailedTestsEnabledStatus();
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
  m_pModel->dataChanged(QModelIndex(), QModelIndex());
}

void ezQtTestGUI::on_actionDisableAll_triggered()
{
  m_pTestFramework->SetAllTestsEnabledStatus(false);
  m_pModel->dataChanged(QModelIndex(), QModelIndex());
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
  QModelIndex LastSubTest = m_pModel->index(m_pModel->rowCount(TestModelIndex) - 1, 0, TestModelIndex);

  if (iTestIndex != -1 && m_uiSubTestsEnabledCount == 0)
  {
    // started a new main test
    m_uiSubTestsEnabledCount = m_pTestFramework->GetSubTestEnabledCount(iTestIndex);
  }

  if (iSubTestIndex == -1)
  {
    // main test was just finished, reset the sub-test count
    m_uiSubTestsEnabledCount = 0;
  }

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
  const ezUInt32 uiTestCount = m_uiTestsEnabledCount;
  const ezUInt32 uiFailed = m_pTestFramework->GetTestsFailedCount();
  const ezUInt32 uiPassed = m_pTestFramework->GetTestsPassedCount();
  const ezUInt32 uiErrors = m_pTestFramework->GetTotalErrorCount();
  double fTestDurationInSeconds = m_pTestFramework->GetTotalTestDuration() / 1000.0;

  // Get the current test's sub-test completion ratio
  float fSubTestPercentage = 0.0f;
  if (iTestIndex != -1 && iSubTestIndex != -1)
  {
    fSubTestPercentage =
      (float)m_pTestFramework->GetTestResult().GetSubTestCount(m_pTestFramework->GetCurrentTestIndex(), ezTestResultQuery::Executed) /
      (float)m_uiSubTestsEnabledCount;
  }

  float fProgress = 100.0f * (fSubTestPercentage + uiFailed + uiPassed) / uiTestCount;
  QString sStatusText = QLatin1String("[progress: ") % QString::number(fProgress, 'f', 2) % QLatin1String("%] [passed: ") %
                        QString::number(uiPassed) % QLatin1String("] [failed: ") % QString::number(uiFailed) %
                        QLatin1String("] [errors: ") % QString::number(uiErrors) % QLatin1String("] [time taken: ") %
                        QString::number(fTestDurationInSeconds, 'f', 2) % QLatin1String(" seconds]");

  m_pStatusText->setText(sStatusText);
  m_pMessageLogDock->currentTestResultChanged(&m_pTestFramework->GetTestResult().GetTestResultData(iTestIndex, iSubTestIndex));

#  if EZ_ENABLED(USE_WIN_EXTRAS)
  if (m_pWinTaskBarButton == nullptr)
  {
    auto ClearPointers = [this]() {
      m_pWinTaskBarButton = nullptr;
      m_pWinTaskBarProgress = nullptr;
    };
    m_pWinTaskBarButton = new QWinTaskbarButton(QApplication::activeWindow());
    m_pWinTaskBarButton->setWindow(QApplication::topLevelWindows()[0]);
    m_OnButtonDestroyed = QObject::connect(m_pWinTaskBarButton, &QObject::destroyed, ClearPointers);

    m_pWinTaskBarProgress = m_pWinTaskBarButton->progress();
    m_OnProgressDestroyed = QObject::connect(m_pWinTaskBarProgress, &QObject::destroyed, ClearPointers);
    m_pWinTaskBarProgress->setMinimum(0);
    m_pWinTaskBarProgress->setMaximum(1000);
    m_pWinTaskBarProgress->setValue(0);
    m_pWinTaskBarProgress->reset();
    m_pWinTaskBarProgress->show();
    m_pWinTaskBarProgress->setVisible(true);
  }

  if (m_pWinTaskBarProgress)
  {
    if (uiErrors == 0)
      m_pWinTaskBarProgress->resume();
    else
      m_pWinTaskBarProgress->stop();

    const ezUInt32 uiProMille = static_cast<ezUInt32>(fProgress * 10.0f);
    m_pWinTaskBarProgress->setValue(uiProMille);
  }
#  endif

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
  ContextMenu.addAction(actionEnableOnlyFailed);
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

  const ezQtTestModelEntry* pEntry = (ezQtTestModelEntry*)index.internalPointer();
  const ezTestResultData* pTestResult = pEntry->GetTestResult();

  m_pMessageLogDock->currentTestSelectionChanged(pTestResult);
}

void OpenInExplorer(const char* szPath)
{
  QStringList args;

  args << QDir::toNativeSeparators(szPath);

  QProcess::startDetached("explorer", args);
}

void ezQtTestGUI::on_actionOpenTestDataFolder_triggered()
{
  ezStringBuilder sDir;
  if (ezFileSystem::ResolveSpecialDirectory(">sdk", sDir).Failed())
    return;

  sDir.AppendPath(m_pTestFramework->GetRelTestDataPath());
  OpenInExplorer(sDir);
}

void ezQtTestGUI::on_actionOpenOutputFolder_triggered()
{
  m_pTestFramework->CreateOutputFolder();
  const char* szDir = m_pTestFramework->GetAbsOutputPath();

  OpenInExplorer(szDir);
}

void ezQtTestGUI::on_actionOpenHTMLFile_triggered()
{
  std::string sOutputFile = std::string(ezTestFramework::GetInstance()->GetAbsOutputPath()) + "/UnitTestsLog.htm";

  QDesktopServices::openUrl(QUrl::fromLocalFile(sOutputFile.c_str()));
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

  restoreGeometry(Settings.value("WindowGeometry", saveGeometry()).toByteArray());
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
  // return;
  QApplication::setStyle(QStyleFactory::create("fusion"));
  // return;
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
