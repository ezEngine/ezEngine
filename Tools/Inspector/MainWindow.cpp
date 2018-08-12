#include <PCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Threading/ThreadUtils.h>
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
#include <Inspector/StatVisWidget.moc.h>
#include <Inspector/SubsystemsWidget.moc.h>
#include <Inspector/TimeWidget.moc.h>
#include <QSettings>
#include <qdir.h>
#include <qfile.h>
#include <qinputdialog.h>
#include <qlistwidget.h>
#include <qstandardpaths.h>
#include <qtimer.h>

ezQtMainWindow* ezQtMainWindow::s_pWidget = nullptr;



ezQtMainWindow::ezQtMainWindow()
    : QMainWindow()
{
  s_pWidget = this;

  m_uiMaxStatSamples = 20000; // should be enough for 5 minutes of history at 60 Hz

  setupUi(this);

  QSettings Settings;
  SetAlwaysOnTop((OnTopMode)Settings.value("AlwaysOnTop", (int)WhenConnected).toInt());

  Settings.beginGroup("MainWindow");

  restoreGeometry(Settings.value("WindowGeometry", saveGeometry()).toByteArray());

  ezQtLogDockWidget* pLogWidget = new ezQtLogDockWidget(this);
  ezQtMemoryWidget* pMemoryWidget = new ezQtMemoryWidget(this);
  ezQtTimeWidget* pTimeWidget = new ezQtTimeWidget(this);
  ezQtInputWidget* pInputWidget = new ezQtInputWidget(this);
  ezQtCVarsWidget* pCVarsWidget = new ezQtCVarsWidget(this);
  ezQtSubsystemsWidget* pSubsystemsWidget = new ezQtSubsystemsWidget(this);
  ezQtFileWidget* pFileWidget = new ezQtFileWidget(this);
  ezQtPluginsWidget* pPluginsWidget = new ezQtPluginsWidget(this);
  ezQtGlobalEventsWidget* pGlobalEventesWidget = new ezQtGlobalEventsWidget(this);
  ezQtReflectionWidget* pReflectionWidget = new ezQtReflectionWidget(this);
  ezQtDataWidget* pDataWidget = new ezQtDataWidget(this);
  ezQtResourceWidget* pResourceWidget = new ezQtResourceWidget(this);

  EZ_VERIFY(nullptr != QWidget::connect(pLogWidget, SIGNAL(visibilityChanged(bool)), this, SLOT(DockWidgetVisibilityChanged(bool))), "");
  EZ_VERIFY(nullptr != QWidget::connect(pTimeWidget, SIGNAL(visibilityChanged(bool)), this, SLOT(DockWidgetVisibilityChanged(bool))), "");
  EZ_VERIFY(nullptr != QWidget::connect(pMemoryWidget, SIGNAL(visibilityChanged(bool)), this, SLOT(DockWidgetVisibilityChanged(bool))), "");
  EZ_VERIFY(nullptr != QWidget::connect(pInputWidget, SIGNAL(visibilityChanged(bool)), this, SLOT(DockWidgetVisibilityChanged(bool))), "");
  EZ_VERIFY(nullptr != QWidget::connect(pCVarsWidget, SIGNAL(visibilityChanged(bool)), this, SLOT(DockWidgetVisibilityChanged(bool))), "");
  EZ_VERIFY(nullptr != QWidget::connect(pReflectionWidget, SIGNAL(visibilityChanged(bool)), this, SLOT(DockWidgetVisibilityChanged(bool))),
            "");
  EZ_VERIFY(nullptr != QWidget::connect(pSubsystemsWidget, SIGNAL(visibilityChanged(bool)), this, SLOT(DockWidgetVisibilityChanged(bool))),
            "");
  EZ_VERIFY(nullptr != QWidget::connect(pFileWidget, SIGNAL(visibilityChanged(bool)), this, SLOT(DockWidgetVisibilityChanged(bool))), "");
  EZ_VERIFY(nullptr != QWidget::connect(pPluginsWidget, SIGNAL(visibilityChanged(bool)), this, SLOT(DockWidgetVisibilityChanged(bool))),
            "");
  EZ_VERIFY(nullptr !=
                QWidget::connect(pGlobalEventesWidget, SIGNAL(visibilityChanged(bool)), this, SLOT(DockWidgetVisibilityChanged(bool))),
            "");
  EZ_VERIFY(nullptr != QWidget::connect(pDataWidget, SIGNAL(visibilityChanged(bool)), this, SLOT(DockWidgetVisibilityChanged(bool))), "");
  EZ_VERIFY(nullptr != QWidget::connect(pResourceWidget, SIGNAL(visibilityChanged(bool)), this, SLOT(DockWidgetVisibilityChanged(bool))),
            "");

  addDockWidget(Qt::BottomDockWidgetArea, pMemoryWidget);
  addDockWidget(Qt::BottomDockWidgetArea, pFileWidget);
  addDockWidget(Qt::BottomDockWidgetArea, pTimeWidget);
  tabifyDockWidget(pMemoryWidget, pFileWidget);
  tabifyDockWidget(pMemoryWidget, pTimeWidget);

  QMenu* pHistoryMenu = new QMenu;
  pHistoryMenu->setTearOffEnabled(true);
  pHistoryMenu->setTitle(QLatin1String("Stat Histories"));
  pHistoryMenu->setIcon(QIcon(":/Icons/Icons/StatHistory.png"));

  for (ezUInt32 i = 0; i < 10; ++i)
  {
    m_pStatHistoryWidgets[i] = new ezQtStatVisWidget(this, i);
    addDockWidget(Qt::BottomDockWidgetArea, m_pStatHistoryWidgets[i]);
    tabifyDockWidget(pMemoryWidget, m_pStatHistoryWidgets[i]);

    EZ_VERIFY(nullptr != QWidget::connect(m_pStatHistoryWidgets[i], SIGNAL(visibilityChanged(bool)), this,
                                          SLOT(DockWidgetVisibilityChanged(bool))),
              "");

    pHistoryMenu->addAction(&m_pStatHistoryWidgets[i]->m_ShowWindowAction);

    m_pStatHistoryWidgets[i]->hide();

    m_pActionShowStatIn[i] = new QAction(this);

    EZ_VERIFY(nullptr != QWidget::connect(m_pActionShowStatIn[i], SIGNAL(triggered()), this, SLOT(ShowStatIn())), "");
  }

  setContextMenuPolicy(Qt::NoContextMenu);

  TreeStats->setContextMenuPolicy(Qt::CustomContextMenu);

  menuWindows->addMenu(pHistoryMenu);

  pMemoryWidget->raise();

  addDockWidget(Qt::RightDockWidgetArea, pCVarsWidget);
  splitDockWidget(pCVarsWidget, pLogWidget, Qt::Horizontal);

  addDockWidget(Qt::RightDockWidgetArea, pSubsystemsWidget);
  tabifyDockWidget(pLogWidget, pSubsystemsWidget);

  addDockWidget(Qt::RightDockWidgetArea, pInputWidget);
  tabifyDockWidget(pLogWidget, pInputWidget);

  addDockWidget(Qt::RightDockWidgetArea, pPluginsWidget);
  tabifyDockWidget(pLogWidget, pPluginsWidget);

  addDockWidget(Qt::RightDockWidgetArea, pGlobalEventesWidget);
  tabifyDockWidget(pLogWidget, pGlobalEventesWidget);

  addDockWidget(Qt::RightDockWidgetArea, pReflectionWidget);
  tabifyDockWidget(pLogWidget, pReflectionWidget);

  addDockWidget(Qt::RightDockWidgetArea, pDataWidget);
  tabifyDockWidget(pLogWidget, pDataWidget);

  addDockWidget(Qt::RightDockWidgetArea, pResourceWidget);
  tabifyDockWidget(pLogWidget, pResourceWidget);


  pLogWidget->raise();

  move(Settings.value("WindowPosition", pos()).toPoint());
  resize(Settings.value("WindowSize", size()).toSize());
  if (Settings.value("IsMaximized", isMaximized()).toBool())
    showMaximized();

  splitter->restoreState(Settings.value("SplitterState", splitter->saveState()).toByteArray());
  splitter->restoreGeometry(Settings.value("SplitterSize", splitter->saveGeometry()).toByteArray());

  restoreState(Settings.value("WindowState", saveState()).toByteArray());

  Settings.endGroup();

  ResetStats();

  LoadFavourites();

  for (ezInt32 i = 0; i < 10; ++i)
    m_pStatHistoryWidgets[i]->Load();

  SetupNetworkTimer();
}

ezQtMainWindow::~ezQtMainWindow()
{
  SaveFavourites();

  for (ezInt32 i = 0; i < 10; ++i)
  {
    m_pStatHistoryWidgets[i]->Save();
    delete m_pStatHistoryWidgets[i];
  }
}

void ezQtMainWindow::closeEvent(QCloseEvent* event)
{
  const bool bMaximized = isMaximized();

  if (bMaximized)
    showNormal();

  QSettings Settings;

  Settings.beginGroup("MainWindow");

  Settings.setValue("WindowGeometry", saveGeometry());
  Settings.setValue("WindowState", saveState());
  Settings.setValue("IsMaximized", bMaximized);

  Settings.setValue("SplitterState", splitter->saveState());
  Settings.setValue("SplitterGeometry", splitter->saveGeometry());

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

      bConnected = true;
    }
    else
    {
      if (bConnected)
      {
        ezQtLogDockWidget::s_pWidget->Log("Lost Connection to Server.");
      }

      bConnected = false;
    }
  }

  if (bResetStats)
  {
    ResetStats();

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

  UpdateStats();

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
  ActionShowWindowLog->setChecked(ezQtLogDockWidget::s_pWidget->isVisible());
  ActionShowWindowMemory->setChecked(ezQtMemoryWidget::s_pWidget->isVisible());
  ActionShowWindowTime->setChecked(ezQtTimeWidget::s_pWidget->isVisible());
  ActionShowWindowInput->setChecked(ezQtInputWidget::s_pWidget->isVisible());
  ActionShowWindowCVar->setChecked(ezQtCVarsWidget::s_pWidget->isVisible());
  ActionShowWindowReflection->setChecked(ezQtReflectionWidget::s_pWidget->isVisible());
  ActionShowWindowSubsystems->setChecked(ezQtSubsystemsWidget::s_pWidget->isVisible());
  ActionShowWindowFile->setChecked(ezQtFileWidget::s_pWidget->isVisible());
  ActionShowWindowPlugins->setChecked(ezQtPluginsWidget::s_pWidget->isVisible());
  ActionShowWindowGlobalEvents->setChecked(ezQtGlobalEventsWidget::s_pWidget->isVisible());
  ActionShowWindowData->setChecked(ezQtDataWidget::s_pWidget->isVisible());
  ActionShowWindowResource->setChecked(ezQtResourceWidget::s_pWidget->isVisible());

  for (ezInt32 i = 0; i < 10; ++i)
    m_pStatHistoryWidgets[i]->m_ShowWindowAction.setChecked(m_pStatHistoryWidgets[i]->isVisible());
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

double ExtractValue(const char* szString);

void ezQtMainWindow::ProcessTelemetry(void* pUnuseed)
{
  if (!s_pWidget)
    return;

  ezTelemetryMessage Msg;

  while (ezTelemetry::RetrieveMessage('STAT', Msg) == EZ_SUCCESS)
  {
    switch (Msg.GetMessageID())
    {
      case ' DEL':
      {
        ezString sStatName;
        Msg.GetReader() >> sStatName;

        ezMap<ezString, StatData>::Iterator it = s_pWidget->m_Stats.Find(sStatName);

        if (!it.IsValid())
          break;

        if (it.Value().m_pItem)
          delete it.Value().m_pItem;

        if (it.Value().m_pItemFavourite)
          delete it.Value().m_pItemFavourite;

        s_pWidget->m_Stats.Remove(it);
      }
      break;

      case ' SET':
      {
        ezString sStatName;
        Msg.GetReader() >> sStatName;

        StatData& sd = s_pWidget->m_Stats[sStatName];

        Msg.GetReader() >> sd.m_sValue;

        StatSample ss;
        ss.m_Value = ExtractValue(sd.m_sValue.GetData());
        Msg.GetReader() >> ss.m_AtGlobalTime;

        sd.m_History.PushBack(ss);

        s_pWidget->m_MaxGlobalTime = ezMath::Max(s_pWidget->m_MaxGlobalTime, ss.m_AtGlobalTime);

        // remove excess samples
        if (sd.m_History.GetCount() > s_pWidget->m_uiMaxStatSamples)
          sd.m_History.PopFront(sd.m_History.GetCount() - s_pWidget->m_uiMaxStatSamples);

        if (sd.m_pItem == nullptr)
        {
          sd.m_pItem = s_pWidget->CreateStat(sStatName.GetData(), false);

          if (s_pWidget->m_Favourites.Find(sStatName).IsValid())
            sd.m_pItem->setCheckState(0, Qt::Checked);
        }

        sd.m_pItem->setData(1, Qt::DisplayRole, sd.m_sValue.GetData());

        if (sd.m_pItemFavourite)
          sd.m_pItemFavourite->setData(1, Qt::DisplayRole, sd.m_sValue.GetData());
      }
      break;
    }
  }

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
