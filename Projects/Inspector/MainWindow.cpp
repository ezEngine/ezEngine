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
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <qlistwidget.h>
#include <qinputdialog.h>
#include <qfile.h>
#include <qstandardpaths.h>
#include <qdir.h>
#include <QSettings>
#include <qtimer.h>

ezMainWindow* ezMainWindow::s_pWidget = NULL;



ezMainWindow::ezMainWindow() : QMainWindow()
{
  s_pWidget = this;

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

  EZ_VERIFY(QWidget::connect(pLogWidget,            SIGNAL(visibilityChanged(bool)), this, SLOT(DockWidgetVisibilityChanged(bool))), "");
  EZ_VERIFY(QWidget::connect(pTimeWidget,           SIGNAL(visibilityChanged(bool)), this, SLOT(DockWidgetVisibilityChanged(bool))), "");
  EZ_VERIFY(QWidget::connect(pMemoryWidget,         SIGNAL(visibilityChanged(bool)), this, SLOT(DockWidgetVisibilityChanged(bool))), "");
  EZ_VERIFY(QWidget::connect(pInputWidget,          SIGNAL(visibilityChanged(bool)), this, SLOT(DockWidgetVisibilityChanged(bool))), "");
  EZ_VERIFY(QWidget::connect(pCVarsWidget,          SIGNAL(visibilityChanged(bool)), this, SLOT(DockWidgetVisibilityChanged(bool))), "");
  EZ_VERIFY(QWidget::connect(pReflectionWidget,     SIGNAL(visibilityChanged(bool)), this, SLOT(DockWidgetVisibilityChanged(bool))), "");
  EZ_VERIFY(QWidget::connect(pSubsystemsWidget,     SIGNAL(visibilityChanged(bool)), this, SLOT(DockWidgetVisibilityChanged(bool))), "");
  EZ_VERIFY(QWidget::connect(pFileWidget,           SIGNAL(visibilityChanged(bool)), this, SLOT(DockWidgetVisibilityChanged(bool))), "");
  EZ_VERIFY(QWidget::connect(pPluginsWidget,        SIGNAL(visibilityChanged(bool)), this, SLOT(DockWidgetVisibilityChanged(bool))), "");
  EZ_VERIFY(QWidget::connect(pGlobalEventesWidget,  SIGNAL(visibilityChanged(bool)), this, SLOT(DockWidgetVisibilityChanged(bool))), "");

  addDockWidget(Qt::BottomDockWidgetArea, pMemoryWidget);
  addDockWidget(Qt::BottomDockWidgetArea, pFileWidget);
  addDockWidget(Qt::BottomDockWidgetArea, pTimeWidget);
  tabifyDockWidget(pMemoryWidget, pFileWidget);
  tabifyDockWidget(pMemoryWidget, pTimeWidget);
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

  pLogWidget->raise();

  splitter->restoreState(Settings.value("SplitterState", splitter->saveState()).toByteArray());
  splitter->restoreGeometry(Settings.value("SplitterSize", splitter->saveGeometry()).toByteArray());

  restoreState(Settings.value("WindowState", saveState()).toByteArray());
  move(Settings.value("WindowPosition", pos()).toPoint());
  resize(Settings.value("WindowSize", size()).toSize());
  if (Settings.value("IsMaximized", isMaximized()).toBool())
    showMaximized();

  Settings.endGroup();

  ResetStats();

  LoadFavourites();

  SetupNetworkTimer();
}

ezMainWindow::~ezMainWindow()
{
  SaveFavourites();
}

void ezMainWindow::closeEvent(QCloseEvent* event) 
{
  QSettings Settings;

  Settings.beginGroup("MainWindow");

  Settings.setValue("WindowGeometry", saveGeometry());
  Settings.setValue("WindowState", saveState());
  Settings.setValue("IsMaximized", isMaximized());

  Settings.setValue("SplitterState", splitter->saveState());
  Settings.setValue("SplitterGeometry", splitter->saveGeometry());

  if (!isMaximized()) 
  {
    Settings.setValue("WindowPosition", pos());
    Settings.setValue("WindowSize", size());
  }

  Settings.endGroup();
}

void ezMainWindow::SaveFavourites()
{
  QString sFile = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
  QDir d; d.mkpath(sFile);

  sFile.append("/Favourites.stats");

  QFile f(sFile);
  if (!f.open(QIODevice::WriteOnly))
    return;

  QDataStream data(&f);

  const ezUInt32 uiNumFavourites = m_Favourites.GetCount();
  data << uiNumFavourites;

  for (ezSet<ezString>::Iterator it = m_Favourites.GetIterator(); it.IsValid(); ++it)
  {
    const QString s = it.Key().GetData();
    data << s;
  }

  f.close();
}

void ezMainWindow::LoadFavourites()
{
  QString sFile = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
  QDir d; d.mkpath(sFile);
  sFile.append("/Favourites.stats");

  ezString s = sFile.toUtf8().data();

  QFile f(sFile);
  if (!f.open(QIODevice::ReadOnly))
    return;

  m_Favourites.Clear();

  QDataStream data(&f);

  ezUInt32 uiNumFavourites = 0;
  data >> uiNumFavourites;

  for (ezUInt32 i = 0; i < uiNumFavourites; ++i)
  {
    QString s;
    data >> s;

    ezString ezs = s.toUtf8().data();

    m_Favourites.Insert(ezs);
  }

  f.close();
}

void ezMainWindow::ResetStats()
{
  m_Stats.Clear();
  TreeStats->clear();
  TreeFavourites->clear();
}

void ezMainWindow::UpdateStats()
{
  static bool bWasConnected = true;

  if (!ezTelemetry::IsConnectedToServer())
    LabelPing->setText("<p>Ping: N/A</p>");
  else
    LabelPing->setText(QString::fromUtf8("<p>Ping: %1ms</p>").arg((ezUInt32) ezTelemetry::GetPingToServer().GetMilliseconds()));

  if (bWasConnected == ezTelemetry::IsConnectedToServer())
    return;

  bWasConnected = ezTelemetry::IsConnectedToServer();

  if (!ezTelemetry::IsConnectedToServer())
  {
    LabelStatus->setText("<p><span style=\" font-weight:600;\">Status: </span><span style=\" font-weight:600; color:#ff0000;\">Not Connected</span></p>");
    LabelServer->setText("<p>Server: N/A</p>");
  }
  else
  {
    LabelStatus->setText("<p><span style=\" font-weight:600;\">Status: </span><span style=\" font-weight:600; color:#00aa00;\">Connected</span></p>");
    LabelServer->setText(QString::fromUtf8("<p>Server: %1 (%2:%3)</p>").arg(ezTelemetry::GetServerName()).arg(ezTelemetry::GetServerIP()).arg(ezTelemetry::s_uiPort));
  }

  UpdateAlwaysOnTop();
}

void ezMainWindow::SetupNetworkTimer()
{
  // reset the timer to fire again
  if (m_pNetworkTimer == NULL)
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
  }
  
  UpdateStats();

  ezPluginsWidget::s_pWidget->UpdateStats();
  ezSubsystemsWidget::s_pWidget->UpdateStats();
  ezMemoryWidget::s_pWidget->UpdateStats();
  ezTimeWidget::s_pWidget->UpdateStats();
  ezFileWidget::s_pWidget->UpdateStats();

  ezTelemetry::PerFrameUpdate();
}


QTreeWidgetItem* ezMainWindow::CreateStat(const char* szPath, bool bParent)
{
  ezStringBuilder sCleanPath = szPath;
  if (sCleanPath.EndsWith("/"))
    sCleanPath.Shrink(0, 1);

  ezMap<ezString, StatData>::Iterator it = m_Stats.Find(sCleanPath.GetData());

  if (it.IsValid() && it.Value().m_pItem != NULL)
    return it.Value().m_pItem;

  QTreeWidgetItem* pParent = NULL;
  StatData& sd = m_Stats[sCleanPath.GetData()];

  {
    ezStringBuilder sParentPath = sCleanPath.GetData();
    sParentPath.PathParentDirectory(1);

    sd.m_pItem = new QTreeWidgetItem();
    sd.m_pItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | (bParent ? Qt::NoItemFlags : Qt::ItemIsUserCheckable));
    sd.m_pItem->setData(0, Qt::UserRole, QString(sCleanPath.GetData()));

    if (bParent)
      sd.m_pItem->setIcon(0, QIcon(":/Icons/Icons/StatGroup.png"));
    else
      sd.m_pItem->setIcon(0, QIcon(":/Icons/Icons/Stat.png"));

    if (!bParent)
      sd.m_pItem->setCheckState(0, Qt::Unchecked);

    if (!sParentPath.IsEmpty())
    {
      pParent = CreateStat(sParentPath.GetData(), true);
      pParent->addChild(sd.m_pItem);
      pParent->setExpanded(false);
    }
    else
    {
      TreeStats->addTopLevelItem(sd.m_pItem);
    }
  }

  {
    ezString sFileName = sCleanPath.GetFileName();
    sd.m_pItem->setData(0, Qt::DisplayRole, sFileName.GetData());

    if (pParent)
      pParent->sortChildren(0, Qt::AscendingOrder);
    else
      TreeStats->sortByColumn(0, Qt::AscendingOrder);

    TreeStats->resizeColumnToContents(0);
  }

  return sd.m_pItem;
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
}

void ezMainWindow::on_ActionShowWindowLog_triggered()
{
  ezLogWidget::s_pWidget->setVisible(ActionShowWindowLog->isChecked());
  ezLogWidget::s_pWidget->raise();
}

void ezMainWindow::on_ActionShowWindowMemory_triggered()
{
  ezMemoryWidget::s_pWidget->setVisible(ActionShowWindowMemory->isChecked());
  ezMemoryWidget::s_pWidget->raise();
}

void ezMainWindow::on_ActionShowWindowTime_triggered()
{
  ezTimeWidget::s_pWidget->setVisible(ActionShowWindowTime->isChecked());
  ezTimeWidget::s_pWidget->raise();
}

void ezMainWindow::on_ActionShowWindowInput_triggered()
{
  ezInputWidget::s_pWidget->setVisible(ActionShowWindowInput->isChecked());
  ezInputWidget::s_pWidget->raise();
}

void ezMainWindow::on_ActionShowWindowCVar_triggered()
{
  ezCVarsWidget::s_pWidget->setVisible(ActionShowWindowCVar->isChecked());
  ezCVarsWidget::s_pWidget->raise();
}

void ezMainWindow::on_ActionShowWindowReflection_triggered()
{
  ezReflectionWidget::s_pWidget->setVisible(ActionShowWindowReflection->isChecked());
  ezReflectionWidget::s_pWidget->raise();
}

void ezMainWindow::on_ActionShowWindowSubsystems_triggered()
{
  ezSubsystemsWidget::s_pWidget->setVisible(ActionShowWindowSubsystems->isChecked());
  ezSubsystemsWidget::s_pWidget->raise();
}

void ezMainWindow::on_ActionShowWindowPlugins_triggered()
{
  ezPluginsWidget::s_pWidget->setVisible(ActionShowWindowPlugins->isChecked());
  ezPluginsWidget::s_pWidget->raise();
}

void ezMainWindow::on_ActionShowWindowFile_triggered()
{
  ezFileWidget::s_pWidget->setVisible(ActionShowWindowFile->isChecked());
  ezFileWidget::s_pWidget->raise();
}

void ezMainWindow::on_ActionShowWindowGlobalEvents_triggered()
{
  ezGlobalEventsWidget::s_pWidget->setVisible(ActionShowWindowGlobalEvents->isChecked());
  ezGlobalEventsWidget::s_pWidget->raise();
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

void ezMainWindow::on_ActionOnTopWhenConnected_triggered()
{
  SetAlwaysOnTop(WhenConnected);
}

void ezMainWindow::on_ActionAlwaysOnTop_triggered()
{
  SetAlwaysOnTop(Always);
}

void ezMainWindow::on_ActionNeverOnTop_triggered()
{
  SetAlwaysOnTop(Never);
}

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

        s_pWidget->m_Stats.Erase(it);
      }
      break;
    case 'SET':
      {
        ezString sStatName;
        Msg.GetReader() >> sStatName;

        StatData& sd = s_pWidget->m_Stats[sStatName];

        Msg.GetReader() >> sd.m_sValue;

        if (sd.m_pItem == NULL)
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

void ezMainWindow::on_ButtonConnect_clicked()
{
  QSettings Settings;
  const QString sServer = Settings.value("LastConnection", QLatin1String("localhost:1040")).toString();

  bool bOk = false;
  QString sRes = QInputDialog::getText(this, "Host", "Host Name or IP Address:\nDefault is 'localhost:1040'", QLineEdit::Normal, sServer, &bOk);

  if (!bOk)
    return;

  Settings.setValue("LastConnection", sRes);

  if (ezTelemetry::ConnectToServer(sRes.toUtf8().data()) == EZ_SUCCESS)
  {
  }
}

void ezMainWindow::SetFavourite(const ezString& sStat, bool bFavourite)
{
  StatData& sd = m_Stats[sStat];

  if (bFavourite)
  {
    m_Favourites.Insert(sStat);

    if (!sd.m_pItemFavourite)
    {
      sd.m_pItemFavourite = new QTreeWidgetItem();
      TreeFavourites->addTopLevelItem(sd.m_pItemFavourite);
      sd.m_pItemFavourite->setData(0, Qt::DisplayRole, sStat.GetData());
      sd.m_pItemFavourite->setData(1, Qt::DisplayRole, sd.m_sValue.GetData());
      sd.m_pItemFavourite->setIcon(0, QIcon(":/Icons/Icons/StatFavourite.png"));

      TreeFavourites->resizeColumnToContents(0);
    }
  }
  else
  {
    if (sd.m_pItemFavourite)
    {
      m_Favourites.Erase(sStat);

      delete sd.m_pItemFavourite;
      sd.m_pItemFavourite = NULL;
    }
  }
}

void ezMainWindow::on_TreeStats_itemChanged(QTreeWidgetItem* item, int column)
{
  if (column == 0)
  {
    int i = 0;

    ezString sPath = item->data(0, Qt::UserRole).toString().toUtf8().data();

    SetFavourite(sPath, (item->checkState(0) == Qt::Checked));

  }
}
