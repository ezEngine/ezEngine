#include <InspectorPCH.h>

#include <Inspector/CVarsWidget.moc.h>
#include <Inspector/DataTransferWidget.moc.h>
#include <Inspector/FileWidget.moc.h>
#include <Inspector/GlobalEventsWidget.moc.h>
#include <Inspector/InputWidget.moc.h>
#include <Inspector/LogDockWidget.moc.h>
#include <Inspector/MainWindow.moc.h>
#include <Inspector/MemoryWidget.moc.h>
#include <Inspector/PluginsWidget.moc.h>
#include <Inspector/ReflectionWidget.moc.h>
#include <Inspector/ResourceWidget.moc.h>
#include <Inspector/SubsystemsWidget.moc.h>
#include <Inspector/TimeWidget.moc.h>
#include <Inspector/MainWidget.moc.h>

ezQtMainWindow* ezQtMainWindow::s_pWidget = nullptr;

ezQtMainWindow::ezQtMainWindow()
  : QMainWindow()
{
  s_pWidget = this;

  setupUi(this);

  m_DockManager = new ads::CDockManager(this);
  m_DockManager->setConfigFlags(static_cast<ads::CDockManager::ConfigFlags>(
    ads::CDockManager::DockAreaHasCloseButton |
    ads::CDockManager::DockAreaCloseButtonClosesTab |
    ads::CDockManager::OpaqueSplitterResize |
    ads::CDockManager::AllTabsHaveCloseButton
    ));

  QSettings Settings;
  SetAlwaysOnTop((OnTopMode)Settings.value("AlwaysOnTop", (int)WhenConnected).toInt());

  Settings.beginGroup("MainWindow");

  restoreGeometry(Settings.value("WindowGeometry", saveGeometry()).toByteArray());

  // The dock manager will set ownership to null on add so there is no reason to provide an owner here.
  // Setting one will actually cause memory corruptions on shutdown for unknown reasons.
  ezQtMainWidget* pMainWidget = new ezQtMainWidget();
  ezQtLogDockWidget* pLogWidget = new ezQtLogDockWidget();
  ezQtMemoryWidget* pMemoryWidget = new ezQtMemoryWidget();
  ezQtTimeWidget* pTimeWidget = new ezQtTimeWidget();
  ezQtInputWidget* pInputWidget = new ezQtInputWidget();
  ezQtCVarsWidget* pCVarsWidget = new ezQtCVarsWidget();
  ezQtSubsystemsWidget* pSubsystemsWidget = new ezQtSubsystemsWidget();
  ezQtFileWidget* pFileWidget = new ezQtFileWidget();
  ezQtPluginsWidget* pPluginsWidget = new ezQtPluginsWidget();
  ezQtGlobalEventsWidget* pGlobalEventesWidget = new ezQtGlobalEventsWidget();
  ezQtReflectionWidget* pReflectionWidget = new ezQtReflectionWidget();
  ezQtDataWidget* pDataWidget = new ezQtDataWidget();
  ezQtResourceWidget* pResourceWidget = new ezQtResourceWidget();

  EZ_VERIFY(nullptr != QWidget::connect(pMainWidget, &ads::CDockWidget::viewToggled, this, &ezQtMainWindow::DockWidgetVisibilityChanged), "");
  EZ_VERIFY(nullptr != QWidget::connect(pLogWidget, &ads::CDockWidget::viewToggled, this, &ezQtMainWindow::DockWidgetVisibilityChanged), "");
  EZ_VERIFY(nullptr != QWidget::connect(pTimeWidget, &ads::CDockWidget::viewToggled, this, &ezQtMainWindow::DockWidgetVisibilityChanged), "");
  EZ_VERIFY(nullptr != QWidget::connect(pMemoryWidget, &ads::CDockWidget::viewToggled, this, &ezQtMainWindow::DockWidgetVisibilityChanged), "");
  EZ_VERIFY(nullptr != QWidget::connect(pInputWidget, &ads::CDockWidget::viewToggled, this, &ezQtMainWindow::DockWidgetVisibilityChanged), "");
  EZ_VERIFY(nullptr != QWidget::connect(pCVarsWidget, &ads::CDockWidget::viewToggled, this, &ezQtMainWindow::DockWidgetVisibilityChanged), "");
  EZ_VERIFY(nullptr != QWidget::connect(pReflectionWidget, &ads::CDockWidget::viewToggled, this, &ezQtMainWindow::DockWidgetVisibilityChanged), "");
  EZ_VERIFY(nullptr != QWidget::connect(pSubsystemsWidget, &ads::CDockWidget::viewToggled, this, &ezQtMainWindow::DockWidgetVisibilityChanged), "");
  EZ_VERIFY(nullptr != QWidget::connect(pFileWidget, &ads::CDockWidget::viewToggled, this, &ezQtMainWindow::DockWidgetVisibilityChanged), "");
  EZ_VERIFY(nullptr != QWidget::connect(pPluginsWidget, &ads::CDockWidget::viewToggled, this, &ezQtMainWindow::DockWidgetVisibilityChanged), "");
  EZ_VERIFY(nullptr != QWidget::connect(pGlobalEventesWidget, &ads::CDockWidget::viewToggled, this, &ezQtMainWindow::DockWidgetVisibilityChanged), "");
  EZ_VERIFY(nullptr != QWidget::connect(pDataWidget, &ads::CDockWidget::viewToggled, this, &ezQtMainWindow::DockWidgetVisibilityChanged), "");
  EZ_VERIFY(nullptr != QWidget::connect(pResourceWidget, &ads::CDockWidget::viewToggled, this, &ezQtMainWindow::DockWidgetVisibilityChanged), "");

  QMenu* pHistoryMenu = new QMenu;
  pHistoryMenu->setTearOffEnabled(true);
  pHistoryMenu->setTitle(QLatin1String("Stat Histories"));
  pHistoryMenu->setIcon(QIcon(":/Icons/Icons/StatHistory.png"));

  for (ezUInt32 i = 0; i < 10; ++i)
  {
    m_pStatHistoryWidgets[i] = new ezQtStatVisWidget(this, i);
    m_DockManager->addDockWidgetTab(ads::BottomDockWidgetArea, m_pStatHistoryWidgets[i]);

    EZ_VERIFY(nullptr != QWidget::connect(m_pStatHistoryWidgets[i], &ads::CDockWidget::viewToggled, this, &ezQtMainWindow::DockWidgetVisibilityChanged), "");

    pHistoryMenu->addAction(&m_pStatHistoryWidgets[i]->m_ShowWindowAction);

    m_pActionShowStatIn[i] = new QAction(this);

    EZ_VERIFY(nullptr != QWidget::connect(m_pActionShowStatIn[i], &QAction::triggered, ezQtMainWidget::s_pWidget, &ezQtMainWidget::ShowStatIn), "");
  }

  // delay this until after all widgets are created
  for (ezUInt32 i = 0; i < 10; ++i)
  {
    m_pStatHistoryWidgets[i]->toggleView(false); // hide
  }

  setContextMenuPolicy(Qt::NoContextMenu);

  menuWindows->addMenu(pHistoryMenu);

  pMemoryWidget->raise();

  m_DockManager->addDockWidget(ads::LeftDockWidgetArea, pMainWidget);
  m_DockManager->addDockWidget(ads::CenterDockWidgetArea, pLogWidget);

  m_DockManager->addDockWidget(ads::RightDockWidgetArea, pCVarsWidget);
  m_DockManager->addDockWidgetTab(ads::RightDockWidgetArea, pGlobalEventesWidget);
  m_DockManager->addDockWidgetTab(ads::RightDockWidgetArea, pDataWidget);
  m_DockManager->addDockWidgetTab(ads::RightDockWidgetArea, pInputWidget);
  m_DockManager->addDockWidgetTab(ads::RightDockWidgetArea, pPluginsWidget);
  m_DockManager->addDockWidgetTab(ads::RightDockWidgetArea, pReflectionWidget);
  m_DockManager->addDockWidgetTab(ads::RightDockWidgetArea, pResourceWidget);
  m_DockManager->addDockWidgetTab(ads::RightDockWidgetArea, pSubsystemsWidget);

  m_DockManager->addDockWidget(ads::BottomDockWidgetArea, pFileWidget);
  m_DockManager->addDockWidgetTab(ads::BottomDockWidgetArea, pMemoryWidget);
  m_DockManager->addDockWidgetTab(ads::BottomDockWidgetArea, pTimeWidget);


  pLogWidget->raise();
  pCVarsWidget->raise();

  auto dockState = Settings.value("DockManagerState");
  if (dockState.isValid() && dockState.type() == QVariant::ByteArray)
  {
    m_DockManager->restoreState(dockState.toByteArray(), 1);
  }

  move(Settings.value("WindowPosition", pos()).toPoint());
  resize(Settings.value("WindowSize", size()).toSize());
  if (Settings.value("IsMaximized", isMaximized()).toBool())
    showMaximized();

  restoreState(Settings.value("WindowState", saveState()).toByteArray());

  Settings.endGroup();

  for (ezInt32 i = 0; i < 10; ++i)
    m_pStatHistoryWidgets[i]->Load();

  SetupNetworkTimer();
}

ezQtMainWindow::~ezQtMainWindow()
{
  for (ezInt32 i = 0; i < 10; ++i)
  {
    m_pStatHistoryWidgets[i]->Save();
  }
  // The dock manager does not take ownership of dock widgets.
  auto dockWidgets = m_DockManager->dockWidgetsMap();
  for (auto it = dockWidgets.begin(); it != dockWidgets.end(); ++it)
  {
    m_DockManager->removeDockWidget(it.value());
    delete it.value();
  }
}

void ezQtMainWindow::closeEvent(QCloseEvent* event)
{
  const bool bMaximized = isMaximized();
  if (bMaximized)
    showNormal();

  QSettings Settings;

  Settings.beginGroup("MainWindow");

  Settings.setValue("DockManagerState", m_DockManager->saveState(1));
  Settings.setValue("WindowGeometry", saveGeometry());
  Settings.setValue("WindowState", saveState());
  Settings.setValue("IsMaximized", bMaximized);
  Settings.setValue("WindowPosition", pos());
  if (!bMaximized)
    Settings.setValue("WindowSize", size());

  Settings.endGroup();
}

void ezQtMainWindow::SetupNetworkTimer()
{
  // reset the timer to fire again
  if (m_pNetworkTimer == nullptr)
    m_pNetworkTimer = new QTimer(this);

  m_pNetworkTimer->singleShot(40, this, SLOT(UpdateNetworkTimeOut()));
}

void ezQtMainWindow::UpdateNetworkTimeOut()
{
  UpdateNetwork();

  SetupNetworkTimer();
}

void ezQtMainWindow::UpdateNetwork()
{
  bool bResetStats = false;

  {
    static ezUInt32 uiServerID = 0;
    static bool bConnected = false;
    static ezString sLastServerName;

    if (ezTelemetry::IsConnectedToServer())
    {
      if (uiServerID != ezTelemetry::GetServerID())
      {
        uiServerID = ezTelemetry::GetServerID();
        bResetStats = true;

        ezStringBuilder s;
        s.Format("Connected to new Server with ID {0}", uiServerID);

        ezQtLogDockWidget::s_pWidget->Log(s.GetData());
      }
      else if (!bConnected)
      {
        ezQtLogDockWidget::s_pWidget->Log("Reconnected to Server.");
      }

      if (sLastServerName != ezTelemetry::GetServerName())
      {
        sLastServerName = ezTelemetry::GetServerName();
        setWindowTitle(QString("ezInspector - %1").arg(sLastServerName.GetData()));
      }

      bConnected = true;
    }
    else
    {
      if (bConnected)
      {
        ezQtLogDockWidget::s_pWidget->Log("Lost Connection to Server.");
        setWindowTitle(QString("ezInspector - disconnected"));
        sLastServerName.Clear();
      }

      bConnected = false;
    }
  }

  if (bResetStats)
  {


    ezQtMainWidget::s_pWidget->ResetStats();
    ezQtLogDockWidget::s_pWidget->ResetStats();
    ezQtMemoryWidget::s_pWidget->ResetStats();
    ezQtTimeWidget::s_pWidget->ResetStats();
    ezQtInputWidget::s_pWidget->ResetStats();
    ezQtCVarsWidget::s_pWidget->ResetStats();
    ezQtReflectionWidget::s_pWidget->ResetStats();
    ezQtFileWidget::s_pWidget->ResetStats();
    ezQtPluginsWidget::s_pWidget->ResetStats();
    ezQtSubsystemsWidget::s_pWidget->ResetStats();
    ezQtGlobalEventsWidget::s_pWidget->ResetStats();
    ezQtDataWidget::s_pWidget->ResetStats();
    ezQtResourceWidget::s_pWidget->ResetStats();
  }

  UpdateAlwaysOnTop();

  ezQtMainWidget::s_pWidget->UpdateStats();
  ezQtPluginsWidget::s_pWidget->UpdateStats();
  ezQtSubsystemsWidget::s_pWidget->UpdateStats();
  ezQtMemoryWidget::s_pWidget->UpdateStats();
  ezQtTimeWidget::s_pWidget->UpdateStats();
  ezQtFileWidget::s_pWidget->UpdateStats();
  ezQtResourceWidget::s_pWidget->UpdateStats();
  // ezQtDataWidget::s_pWidget->UpdateStats();

  for (ezInt32 i = 0; i < 10; ++i)
    m_pStatHistoryWidgets[i]->UpdateStats();

  ezTelemetry::PerFrameUpdate();
}

void ezQtMainWindow::DockWidgetVisibilityChanged(bool bVisible)
{
  // TODO: add menu entry for qt main widget

  ActionShowWindowLog->setChecked(!ezQtLogDockWidget::s_pWidget->isClosed());
  ActionShowWindowMemory->setChecked(!ezQtMemoryWidget::s_pWidget->isClosed());
  ActionShowWindowTime->setChecked(!ezQtTimeWidget::s_pWidget->isClosed());
  ActionShowWindowInput->setChecked(!ezQtInputWidget::s_pWidget->isClosed());
  ActionShowWindowCVar->setChecked(!ezQtCVarsWidget::s_pWidget->isClosed());
  ActionShowWindowReflection->setChecked(!ezQtReflectionWidget::s_pWidget->isClosed());
  ActionShowWindowSubsystems->setChecked(!ezQtSubsystemsWidget::s_pWidget->isClosed());
  ActionShowWindowFile->setChecked(!ezQtFileWidget::s_pWidget->isClosed());
  ActionShowWindowPlugins->setChecked(!ezQtPluginsWidget::s_pWidget->isClosed());
  ActionShowWindowGlobalEvents->setChecked(!ezQtGlobalEventsWidget::s_pWidget->isClosed());
  ActionShowWindowData->setChecked(!ezQtDataWidget::s_pWidget->isClosed());
  ActionShowWindowResource->setChecked(!ezQtResourceWidget::s_pWidget->isClosed());

  for (ezInt32 i = 0; i < 10; ++i)
    m_pStatHistoryWidgets[i]->m_ShowWindowAction.setChecked(!m_pStatHistoryWidgets[i]->isClosed());
}


void ezQtMainWindow::SetAlwaysOnTop(OnTopMode Mode)
{
  m_OnTopMode = Mode;

  QSettings Settings;
  Settings.setValue("AlwaysOnTop", (int)m_OnTopMode);

  ActionNeverOnTop->setChecked((m_OnTopMode == Never) ? Qt::Checked : Qt::Unchecked);
  ActionAlwaysOnTop->setChecked((m_OnTopMode == Always) ? Qt::Checked : Qt::Unchecked);
  ActionOnTopWhenConnected->setChecked((m_OnTopMode == WhenConnected) ? Qt::Checked : Qt::Unchecked);

  UpdateAlwaysOnTop();
}

void ezQtMainWindow::UpdateAlwaysOnTop()
{
  static bool bOnTop = false;

  bool bNewState = bOnTop;

  if (m_OnTopMode == Always || (m_OnTopMode == WhenConnected && ezTelemetry::IsConnectedToServer()))
    bNewState = true;
  else
    bNewState = false;

  if (bOnTop != bNewState)
  {
    bOnTop = bNewState;

    hide();

    if (bOnTop)
      setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    else
      setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint | Qt::WindowStaysOnBottomHint);

    show();
  }
}

void ezQtMainWindow::ProcessTelemetry(void* pUnuseed)
{
  if (!s_pWidget)
    return;

  ezTelemetryMessage Msg;

  while (ezTelemetry::RetrieveMessage(' APP', Msg) == EZ_SUCCESS)
  {
    switch (Msg.GetMessageID())
    {
      case 'ASRT':
      {
        ezString sSourceFile, sFunction, sExpression, sMessage;
        ezUInt32 uiLine = 0;

        Msg.GetReader() >> sSourceFile;
        Msg.GetReader() >> uiLine;
        Msg.GetReader() >> sFunction;
        Msg.GetReader() >> sExpression;
        Msg.GetReader() >> sMessage;

        ezQtLogDockWidget::s_pWidget->Log("");
        ezQtLogDockWidget::s_pWidget->Log("<<< Application Assertion >>>");
        ezQtLogDockWidget::s_pWidget->Log("");

        ezQtLogDockWidget::s_pWidget->Log(ezFmt("    Expression: '{0}'", sExpression));
        ezQtLogDockWidget::s_pWidget->Log("");

        ezQtLogDockWidget::s_pWidget->Log(ezFmt("    Message: '{0}'", sMessage));
        ezQtLogDockWidget::s_pWidget->Log("");

        ezQtLogDockWidget::s_pWidget->Log(ezFmt("   File: '{0}'", sSourceFile));

        ezQtLogDockWidget::s_pWidget->Log(ezFmt("   Line: {0}", uiLine));

        ezQtLogDockWidget::s_pWidget->Log(ezFmt("   In Function: '{0}'", sFunction));

        ezQtLogDockWidget::s_pWidget->Log("");

        ezQtLogDockWidget::s_pWidget->Log(">>> Application Assertion <<<");
        ezQtLogDockWidget::s_pWidget->Log("");
      }
      break;
    }
  }
}
