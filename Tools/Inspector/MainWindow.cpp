#include <PCH.h>
#include <Inspector/MainWindow.moc.h>
#include <Inspector/MemoryWidget.moc.h>
#include <Inspector/TimeWidget.moc.h>
#include <Inspector/InputWidget.moc.h>
#include <Inspector/CVarsWidget.moc.h>
#include <Inspector/ReflectionWidget.moc.h>
#include <Inspector/LogWidget.moc.h>
#include <Inspector/SubsystemsWidget.moc.h>
#include <Inspector/FileWidget.moc.h>
#include <Inspector/PluginsWidget.moc.h>
#include <Inspector/GlobalEventsWidget.moc.h>
#include <Inspector/StatVisWidget.moc.h>
#include <Inspector/DataTransferWidget.moc.h>
#include <Inspector/ResourceWidget.moc.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <qlistwidget.h>
#include <qinputdialog.h>
#include <qfile.h>
#include <qstandardpaths.h>
#include <qdir.h>
#include <QSettings>
#include <qtimer.h>

ezMainWindow* ezMainWindow::s_pWidget = nullptr;



ezMainWindow::ezMainWindow() : QMainWindow()
{
  s_pWidget = this;

  m_uiMaxStatSamples = 20000; // should be enough for 5 minutes of history at 60 Hz

  setupUi(this);

  QSettings Settings;
  SetAlwaysOnTop((OnTopMode) Settings.value("AlwaysOnTop", (int) WhenConnected).toInt());

  Settings.beginGroup("MainWindow");

  restoreGeometry(Settings.value("WindowGeometry", saveGeometry() ).toByteArray());

  ezLogWidget*          pLogWidget            = new ezLogWidget(this);
  ezMemoryWidget*       pMemoryWidget         = new ezMemoryWidget(this);
  ezTimeWidget*         pTimeWidget           = new ezTimeWidget(this);
  ezInputWidget*        pInputWidget          = new ezInputWidget(this);
  ezCVarsWidget*        pCVarsWidget          = new ezCVarsWidget(this);
  ezSubsystemsWidget*   pSubsystemsWidget     = new ezSubsystemsWidget(this);
  ezFileWidget*         pFileWidget           = new ezFileWidget(this);
  ezPluginsWidget*      pPluginsWidget        = new ezPluginsWidget(this);
  ezGlobalEventsWidget* pGlobalEventesWidget  = new ezGlobalEventsWidget(this);
  ezReflectionWidget*   pReflectionWidget     = new ezReflectionWidget(this);
  ezDataWidget*         pDataWidget           = new ezDataWidget(this);
  ezResourceWidget*     pResourceWidget       = new ezResourceWidget(this);

  EZ_VERIFY(nullptr != QWidget::connect(pLogWidget,            SIGNAL(visibilityChanged(bool)), this, SLOT(DockWidgetVisibilityChanged(bool))), "");
  EZ_VERIFY(nullptr != QWidget::connect(pTimeWidget,           SIGNAL(visibilityChanged(bool)), this, SLOT(DockWidgetVisibilityChanged(bool))), "");
  EZ_VERIFY(nullptr != QWidget::connect(pMemoryWidget,         SIGNAL(visibilityChanged(bool)), this, SLOT(DockWidgetVisibilityChanged(bool))), "");
  EZ_VERIFY(nullptr != QWidget::connect(pInputWidget,          SIGNAL(visibilityChanged(bool)), this, SLOT(DockWidgetVisibilityChanged(bool))), "");
  EZ_VERIFY(nullptr != QWidget::connect(pCVarsWidget,          SIGNAL(visibilityChanged(bool)), this, SLOT(DockWidgetVisibilityChanged(bool))), "");
  EZ_VERIFY(nullptr != QWidget::connect(pReflectionWidget,     SIGNAL(visibilityChanged(bool)), this, SLOT(DockWidgetVisibilityChanged(bool))), "");
  EZ_VERIFY(nullptr != QWidget::connect(pSubsystemsWidget,     SIGNAL(visibilityChanged(bool)), this, SLOT(DockWidgetVisibilityChanged(bool))), "");
  EZ_VERIFY(nullptr != QWidget::connect(pFileWidget,           SIGNAL(visibilityChanged(bool)), this, SLOT(DockWidgetVisibilityChanged(bool))), "");
  EZ_VERIFY(nullptr != QWidget::connect(pPluginsWidget,        SIGNAL(visibilityChanged(bool)), this, SLOT(DockWidgetVisibilityChanged(bool))), "");
  EZ_VERIFY(nullptr != QWidget::connect(pGlobalEventesWidget,  SIGNAL(visibilityChanged(bool)), this, SLOT(DockWidgetVisibilityChanged(bool))), "");
  EZ_VERIFY(nullptr != QWidget::connect(pDataWidget,           SIGNAL(visibilityChanged(bool)), this, SLOT(DockWidgetVisibilityChanged(bool))), "");
  EZ_VERIFY(nullptr != QWidget::connect(pResourceWidget,       SIGNAL(visibilityChanged(bool)), this, SLOT(DockWidgetVisibilityChanged(bool))), "");

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
    m_pStatHistoryWidgets[i] = new ezStatVisWidget(this, i);
    addDockWidget(Qt::BottomDockWidgetArea, m_pStatHistoryWidgets[i]);
    tabifyDockWidget(pMemoryWidget, m_pStatHistoryWidgets[i]);

    EZ_VERIFY(nullptr != QWidget::connect(m_pStatHistoryWidgets[i],  SIGNAL(visibilityChanged(bool)), this, SLOT(DockWidgetVisibilityChanged(bool))), "");

    pHistoryMenu->addAction(&m_pStatHistoryWidgets[i]->m_ShowWindowAction);

    m_pStatHistoryWidgets[i]->hide();

    m_pActionShowStatIn[i] = new QAction(this);

    EZ_VERIFY(nullptr != QWidget::connect(m_pActionShowStatIn[i],  SIGNAL(triggered()), this, SLOT(ShowStatIn())), "");
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

ezMainWindow::~ezMainWindow()
{
  SaveFavourites();

  for (ezInt32 i = 0; i < 10; ++i)
  {
    m_pStatHistoryWidgets[i]->Save();
    delete m_pStatHistoryWidgets[i];
  }
}

void ezMainWindow::closeEvent(QCloseEvent* event) 
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

void ezMainWindow::SetupNetworkTimer()
{
  // reset the timer to fire again
  if (m_pNetworkTimer == nullptr)
    m_pNetworkTimer = new QTimer(this);

  m_pNetworkTimer->singleShot(40, this, SLOT(UpdateNetworkTimeOut()));
}

void ezMainWindow::UpdateNetworkTimeOut()
{
  UpdateNetwork();

  SetupNetworkTimer();
}

void ezMainWindow::UpdateNetwork()
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
        s.Format("Connected to new Server with ID %i", uiServerID);

        ezLogWidget::s_pWidget->Log(s.GetData());
      }
      else
        if (!bConnected)
        {
          ezLogWidget::s_pWidget->Log("Reconnected to Server.");
        }

        bConnected = true;
    }
    else
    {
      if (bConnected)
      {
        ezLogWidget::s_pWidget->Log("Lost Connection to Server.");
      }

      bConnected = false;
    }
  }

  if (bResetStats)
  {
    ResetStats();

    ezLogWidget::s_pWidget->ResetStats();
    ezMemoryWidget::s_pWidget->ResetStats();
    ezTimeWidget::s_pWidget->ResetStats();
    ezInputWidget::s_pWidget->ResetStats();
    ezCVarsWidget::s_pWidget->ResetStats();
    ezReflectionWidget::s_pWidget->ResetStats();
    ezFileWidget::s_pWidget->ResetStats();
    ezPluginsWidget::s_pWidget->ResetStats();
    ezSubsystemsWidget::s_pWidget->ResetStats();
    ezGlobalEventsWidget::s_pWidget->ResetStats();
    ezDataWidget::s_pWidget->ResetStats();
    ezResourceWidget::s_pWidget->ResetStats();
  }
  
  UpdateStats();

  ezPluginsWidget::s_pWidget->UpdateStats();
  ezSubsystemsWidget::s_pWidget->UpdateStats();
  ezMemoryWidget::s_pWidget->UpdateStats();
  ezTimeWidget::s_pWidget->UpdateStats();
  ezFileWidget::s_pWidget->UpdateStats();
  ezResourceWidget::s_pWidget->UpdateStats();
  //ezDataWidget::s_pWidget->UpdateStats();

  for (ezInt32 i = 0; i < 10; ++i)
    m_pStatHistoryWidgets[i]->UpdateStats();

  ezTelemetry::PerFrameUpdate();
}

void ezMainWindow::DockWidgetVisibilityChanged(bool bVisible)
{
  ActionShowWindowLog->setChecked(ezLogWidget::s_pWidget->isVisible());
  ActionShowWindowMemory->setChecked(ezMemoryWidget::s_pWidget->isVisible());
  ActionShowWindowTime->setChecked(ezTimeWidget::s_pWidget->isVisible());
  ActionShowWindowInput->setChecked(ezInputWidget::s_pWidget->isVisible());
  ActionShowWindowCVar->setChecked(ezCVarsWidget::s_pWidget->isVisible());
  ActionShowWindowReflection->setChecked(ezReflectionWidget::s_pWidget->isVisible());
  ActionShowWindowSubsystems->setChecked(ezSubsystemsWidget::s_pWidget->isVisible());
  ActionShowWindowFile->setChecked(ezFileWidget::s_pWidget->isVisible());
  ActionShowWindowPlugins->setChecked(ezPluginsWidget::s_pWidget->isVisible());
  ActionShowWindowGlobalEvents->setChecked(ezGlobalEventsWidget::s_pWidget->isVisible());
  ActionShowWindowData->setChecked(ezDataWidget::s_pWidget->isVisible());
  ActionShowWindowResource->setChecked(ezResourceWidget::s_pWidget->isVisible());

  for (ezInt32 i = 0; i < 10; ++i)
    m_pStatHistoryWidgets[i]->m_ShowWindowAction.setChecked(m_pStatHistoryWidgets[i]->isVisible());
}


void ezMainWindow::SetAlwaysOnTop(OnTopMode Mode)
{
  m_OnTopMode = Mode;

  QSettings Settings;
  Settings.setValue("AlwaysOnTop", (int) m_OnTopMode);

  ActionNeverOnTop->setChecked((m_OnTopMode == Never) ? Qt::Checked : Qt::Unchecked);
  ActionAlwaysOnTop->setChecked((m_OnTopMode == Always) ? Qt::Checked : Qt::Unchecked);
  ActionOnTopWhenConnected->setChecked((m_OnTopMode == WhenConnected) ? Qt::Checked : Qt::Unchecked);

  UpdateAlwaysOnTop();
}

void ezMainWindow::UpdateAlwaysOnTop()
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

void ezMainWindow::ProcessTelemetry(void* pUnuseed)
{
  if (!s_pWidget)
    return;

  ezTelemetryMessage Msg;

  while (ezTelemetry::RetrieveMessage('STAT', Msg) == EZ_SUCCESS)
  {
    switch (Msg.GetMessageID())
    {
    case 'DEL':
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

    case 'SET':
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

  while (ezTelemetry::RetrieveMessage('APP', Msg) == EZ_SUCCESS)
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

        ezLogWidget::s_pWidget->Log("");
        ezLogWidget::s_pWidget->Log("<<< Application Assertion >>>");
        ezLogWidget::s_pWidget->Log("");

        ezLogWidget::s_pWidget->Log("    Expression: '%s'", sExpression.GetData());
        ezLogWidget::s_pWidget->Log("");

        ezLogWidget::s_pWidget->Log("    Message: '%s'", sMessage.GetData());
        ezLogWidget::s_pWidget->Log("");

        ezLogWidget::s_pWidget->Log("   File: '%s'", sSourceFile.GetData());

        ezLogWidget::s_pWidget->Log("   Line: %i", uiLine);

        ezLogWidget::s_pWidget->Log("   In Function: '%s'", sFunction.GetData());

        ezLogWidget::s_pWidget->Log("");

        ezLogWidget::s_pWidget->Log(">>> Application Assertion <<<");
        ezLogWidget::s_pWidget->Log("");
      }
      break;
    }
  }
}

