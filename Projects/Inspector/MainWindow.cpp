#include <Inspector/MainWindow.moc.h>
#include <Inspector/MemoryWidget.moc.h>
#include <Inspector/InputWidget.moc.h>
#include <Inspector/CVarsWidget.moc.h>
#include <Inspector/LogWidget.moc.h>
#include <Inspector/SubsystemsWidget.moc.h>
#include <Inspector/FileWidget.moc.h>
#include <Inspector/PluginsWidget.moc.h>
#include <Foundation/Communication/Telemetry.h>
#include <qlistwidget.h>
#include <qinputdialog.h>
#include <qfile.h>
#include <qstandardpaths.h>
#include <qdir.h>
#include <qsettings.h>

ezMainWindow* ezMainWindow::s_pWidget = NULL;

ezMainWindow::ezMainWindow() : QMainWindow()
{
  s_pWidget = this;

  setupUi(this);

  QSettings qsettings;
  qsettings.beginGroup( "mainwindow" );

  restoreGeometry(qsettings.value( "geometry", saveGeometry() ).toByteArray());

  ezLogWidget*        pLogWidget        = new ezLogWidget(this);
  ezMemoryWidget*     pMemoryWidget     = new ezMemoryWidget(this);
  ezInputWidget*      pInputWidget      = new ezInputWidget(this);
  ezCVarsWidget*      pCVarsWidget      = new ezCVarsWidget(this);
  ezSubsystemsWidget* pSubsystemsWidget = new ezSubsystemsWidget(this);
  ezFileWidget*       pFileWidget       = new ezFileWidget(this);
  ezPluginsWidget*    pPluginsWidget    = new ezPluginsWidget(this);

  EZ_VERIFY(QWidget::connect(pLogWidget, SIGNAL(visibilityChanged(bool)), this, SLOT(DockWidgetVisibilityChanged(bool))), "Bla");
  EZ_VERIFY(QWidget::connect(pMemoryWidget, SIGNAL(visibilityChanged(bool)), this, SLOT(DockWidgetVisibilityChanged(bool))), "Bla");
  EZ_VERIFY(QWidget::connect(pInputWidget, SIGNAL(visibilityChanged(bool)), this, SLOT(DockWidgetVisibilityChanged(bool))), "Bla");
  EZ_VERIFY(QWidget::connect(pCVarsWidget, SIGNAL(visibilityChanged(bool)), this, SLOT(DockWidgetVisibilityChanged(bool))), "Bla");
  EZ_VERIFY(QWidget::connect(pSubsystemsWidget, SIGNAL(visibilityChanged(bool)), this, SLOT(DockWidgetVisibilityChanged(bool))), "Bla");
  EZ_VERIFY(QWidget::connect(pFileWidget, SIGNAL(visibilityChanged(bool)), this, SLOT(DockWidgetVisibilityChanged(bool))), "Bla");
  EZ_VERIFY(QWidget::connect(pPluginsWidget, SIGNAL(visibilityChanged(bool)), this, SLOT(DockWidgetVisibilityChanged(bool))), "Bla");

  addDockWidget(Qt::BottomDockWidgetArea, pMemoryWidget);
  addDockWidget(Qt::BottomDockWidgetArea, pFileWidget);
  tabifyDockWidget(pMemoryWidget, pFileWidget);
  pMemoryWidget->raise();

  addDockWidget(Qt::RightDockWidgetArea, pCVarsWidget);
  splitDockWidget(pCVarsWidget, pLogWidget, Qt::Horizontal);

  addDockWidget(Qt::RightDockWidgetArea, pSubsystemsWidget);
  tabifyDockWidget(pLogWidget, pSubsystemsWidget);

  addDockWidget(Qt::RightDockWidgetArea, pInputWidget);
  tabifyDockWidget(pLogWidget, pInputWidget);

  addDockWidget(Qt::RightDockWidgetArea, pPluginsWidget);
  tabifyDockWidget(pLogWidget, pPluginsWidget);

  pLogWidget->raise();

  splitter->restoreState(qsettings.value("SplitterState", splitter->saveState()).toByteArray());
  splitter->restoreGeometry(qsettings.value("SplitterGeometry", splitter->saveGeometry()).toByteArray());

  restoreState(qsettings.value( "savestate", saveState() ).toByteArray());
  move(qsettings.value( "pos", pos() ).toPoint());
  resize(qsettings.value( "size", size() ).toSize());
  if ( qsettings.value( "maximized", isMaximized() ).toBool() )
    showMaximized();

  qsettings.endGroup();

  ResetStats();

  LoadFavourites();
}

ezMainWindow::~ezMainWindow()
{
  SaveFavourites();
}

void ezMainWindow::closeEvent(QCloseEvent* event) 
{
  QSettings qsettings;

  qsettings.beginGroup("mainwindow");

  qsettings.setValue("geometry", saveGeometry());
  qsettings.setValue("savestate", saveState());
  qsettings.setValue("maximized", isMaximized());

  qsettings.setValue("SplitterState", splitter->saveState());
  qsettings.setValue("SplitterGeometry", splitter->saveGeometry());

  if (!isMaximized()) 
  {
    qsettings.setValue("pos", pos());
    qsettings.setValue("size", size());
  }

  qsettings.endGroup();
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
  if (!ezTelemetry::IsConnectedToServer())
  {
    LabelStatus->setText("<p><span style=\" font-weight:600;\">Status: </span><span style=\" font-weight:600; color:#ff0000;\">Not Connected</span></p>");
    LabelServer->setText("<p>Server: N/A</p>");
    LabelPing->setText("<p>Ping: N/A</p>");
  }
  else
  {
    LabelStatus->setText("<p><span style=\" font-weight:600;\">Status: </span><span style=\" font-weight:600; color:#00aa00;\">Connected</span></p>");
    LabelServer->setText(QString::fromUtf8("<p>Server: %1 (%2)</p>").arg(ezTelemetry::GetServerName()).arg(ezTelemetry::GetServerIP()));
    LabelPing->setText(QString::fromUtf8("<p>Ping: %1ms</p>").arg((ezUInt32) ezTelemetry::GetPingToServer().GetMilliSeconds()));
  }
}

void ezMainWindow::paintEvent(QPaintEvent* event)
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

    if (ezLogWidget::s_pWidget)
      ezLogWidget::s_pWidget->ResetStats();

    if (ezMemoryWidget::s_pWidget)
      ezMemoryWidget::s_pWidget->ResetStats();

    if (ezInputWidget::s_pWidget)
      ezInputWidget::s_pWidget->ResetStats();

    if (ezCVarsWidget::s_pWidget)
      ezCVarsWidget::s_pWidget->ResetStats();

    if (ezFileWidget::s_pWidget)
      ezFileWidget::s_pWidget->ResetStats();

    if (ezPluginsWidget::s_pWidget)
      ezPluginsWidget::s_pWidget->ResetStats();

    if (ezSubsystemsWidget::s_pWidget)
      ezSubsystemsWidget::s_pWidget->ResetStats();
  }

  UpdateStats();

  if (ezPluginsWidget::s_pWidget)
    ezPluginsWidget::s_pWidget->UpdateStats();

  if (ezSubsystemsWidget::s_pWidget)
    ezSubsystemsWidget::s_pWidget->UpdateStats();

  if (ezMemoryWidget::s_pWidget)
    ezMemoryWidget::s_pWidget->UpdateStats();

  ezTelemetry::PerFrameUpdate();

  update();
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
  ActionShowWindowInput->setChecked(ezInputWidget::s_pWidget->isVisible());
  ActionShowWindowCVar->setChecked(ezCVarsWidget::s_pWidget->isVisible());
  ActionShowWindowSubsystems->setChecked(ezSubsystemsWidget::s_pWidget->isVisible());
  ActionShowWindowFile->setChecked(ezFileWidget::s_pWidget->isVisible());
  ActionShowWindowPlugins->setChecked(ezPluginsWidget::s_pWidget->isVisible());
}

void ezMainWindow::on_ActionShowWindowLog_triggered()
{
  ezLogWidget::s_pWidget->setVisible(ActionShowWindowLog->isChecked());
}

void ezMainWindow::on_ActionShowWindowMemory_triggered()
{
  ezMemoryWidget::s_pWidget->setVisible(ActionShowWindowMemory->isChecked());
}

void ezMainWindow::on_ActionShowWindowInput_triggered()
{
  ezInputWidget::s_pWidget->setVisible(ActionShowWindowInput->isChecked());
}

void ezMainWindow::on_ActionShowWindowCVar_triggered()
{
  ezCVarsWidget::s_pWidget->setVisible(ActionShowWindowCVar->isChecked());
}

void ezMainWindow::on_ActionShowWindowSubsystems_triggered()
{
  ezSubsystemsWidget::s_pWidget->setVisible(ActionShowWindowSubsystems->isChecked());
}

void ezMainWindow::on_ActionShowWindowPlugins_triggered()
{
  ezPluginsWidget::s_pWidget->setVisible(ActionShowWindowPlugins->isChecked());
}

void ezMainWindow::on_ActionShowWindowFile_triggered()
{
  ezFileWidget::s_pWidget->setVisible(ActionShowWindowFile->isChecked());
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
  bool bOk = false;
  QString sRes = QInputDialog::getText(this, "Input Server Name or IP Address", "", QLineEdit::Normal, "", &bOk);

  if (!bOk)
    return;

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
