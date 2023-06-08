#include <Inspector/InspectorPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <Inspector/MainWidget.moc.h>
#include <Inspector/MainWindow.moc.h>
#include <QDir>
#include <QMenu>
#include <QSettings>
#include <QStandardPaths>

ezQtMainWidget* ezQtMainWidget::s_pWidget = nullptr;

ezQtMainWidget::ezQtMainWidget(QWidget* pParent)
  : ads::CDockWidget("Main", pParent)
{
  s_pWidget = this;

  setupUi(this);
  setWidget(MainWidgetFrame);

  this->setFeature(ads::CDockWidget::DockWidgetClosable, false);

  m_uiMaxStatSamples = 20000; // should be enough for 5 minutes of history at 60 Hz

  setContextMenuPolicy(Qt::NoContextMenu);

  TreeStats->setContextMenuPolicy(Qt::CustomContextMenu);

  ResetStats();

  LoadFavorites();

  QSettings Settings;
  Settings.beginGroup("MainWidget");

  splitter->restoreState(Settings.value("SplitterState", splitter->saveState()).toByteArray());
  splitter->restoreGeometry(Settings.value("SplitterSize", splitter->saveGeometry()).toByteArray());

  Settings.endGroup();
}

ezQtMainWidget::~ezQtMainWidget()
{
  SaveFavorites();
}

void ezQtMainWidget::ProcessTelemetry(void* pUnuseed)
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

        if (it.Value().m_pItemFavorite)
          delete it.Value().m_pItemFavorite;

        s_pWidget->m_Stats.Remove(it);
      }
      break;

      case ' SET':
      {
        ezString sStatName;
        Msg.GetReader() >> sStatName;

        StatData& sd = s_pWidget->m_Stats[sStatName];

        Msg.GetReader() >> sd.m_Value;

        StatSample ss;
        ss.m_Value = sd.m_Value.ConvertTo<double>();
        Msg.GetReader() >> ss.m_AtGlobalTime;

        sd.m_History.PushBack(ss);

        s_pWidget->m_MaxGlobalTime = ezMath::Max(s_pWidget->m_MaxGlobalTime, ss.m_AtGlobalTime);

        // remove excess samples
        if (sd.m_History.GetCount() > s_pWidget->m_uiMaxStatSamples)
          sd.m_History.PopFront(sd.m_History.GetCount() - s_pWidget->m_uiMaxStatSamples);

        if (sd.m_pItem == nullptr)
        {
          sd.m_pItem = s_pWidget->CreateStat(sStatName.GetData(), false);

          if (s_pWidget->m_Favorites.Find(sStatName).IsValid())
            sd.m_pItem->setCheckState(0, Qt::Checked);
        }

        const ezString sValue = sd.m_Value.ConvertTo<ezString>();
        sd.m_pItem->setData(1, Qt::DisplayRole, sValue.GetData());

        if (sd.m_pItemFavorite)
          sd.m_pItemFavorite->setData(1, Qt::DisplayRole, sValue.GetData());
      }
      break;
    }
  }
}

void ezQtMainWidget::on_ButtonConnect_clicked()
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

void ezQtMainWidget::SaveFavorites()
{
  QString sFile = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
  QDir dir;
  dir.mkpath(sFile);

  sFile.append("/Favourites.stats");

  QFile f(sFile);
  if (!f.open(QIODevice::WriteOnly))
    return;

  QDataStream stream(&f);

  const ezUInt32 uiNumFavorites = m_Favorites.GetCount();
  stream << uiNumFavorites;

  for (ezSet<ezString>::Iterator it = m_Favorites.GetIterator(); it.IsValid(); ++it)
  {
    const QString s = it.Key().GetData();
    stream << s;
  }

  f.close();
}

void ezQtMainWidget::LoadFavorites()
{
  QString sFile = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
  QDir dir;
  dir.mkpath(sFile);
  sFile.append("/Favourites.stats");

  QFile f(sFile);
  if (!f.open(QIODevice::ReadOnly))
    return;

  m_Favorites.Clear();

  QDataStream stream(&f);

  ezUInt32 uiNumFavorites = 0;
  stream >> uiNumFavorites;

  for (ezUInt32 i = 0; i < uiNumFavorites; ++i)
  {
    QString s;
    stream >> s;

    ezString ezs = s.toUtf8().data();

    m_Favorites.Insert(ezs);
  }

  f.close();
}

void ezQtMainWidget::ResetStats()
{
  m_Stats.Clear();
  TreeStats->clear();
  TreeFavorites->clear();
}

void ezQtMainWidget::UpdateStats()
{
  static bool bWasConnected = false;
  const bool bIsConnected = ezTelemetry::IsConnectedToServer();

  if (bIsConnected)
    LabelPing->setText(QString::fromUtf8("<p>Ping: %1ms</p>").arg((ezUInt32)ezTelemetry::GetPingToServer().GetMilliseconds()));

  if (bWasConnected == bIsConnected)
    return;

  bWasConnected = bIsConnected;

  if (!bIsConnected)
  {
    LabelPing->setText("<p>Ping: N/A</p>");
    LabelStatus->setText(
      "<p><span style=\" font-weight:600;\">Status: </span><span style=\" font-weight:600; color:#ff0000;\">Not Connected</span></p>");
    LabelServer->setText("<p>Server: N/A</p>");
  }
  else
  {
    ezStringBuilder tmp;

    LabelStatus->setText("<p><span style=\" font-weight:600;\">Status: </span><span style=\" font-weight:600; color:#00aa00;\">Connected</span></p>");
    LabelServer->setText(QString::fromUtf8("<p>Server: %1:%2</p>").arg(ezTelemetry::GetServerIP().GetData(tmp)).arg(ezTelemetry::s_uiPort));
  }
}


void ezQtMainWidget::closeEvent(QCloseEvent* pEvent)
{
  QSettings Settings;

  Settings.beginGroup("MainWidget");

  Settings.setValue("SplitterState", splitter->saveState());
  Settings.setValue("SplitterGeometry", splitter->saveGeometry());

  Settings.endGroup();
}

QTreeWidgetItem* ezQtMainWidget::CreateStat(ezStringView sPath, bool bParent)
{
  ezStringBuilder sCleanPath = sPath;
  if (sCleanPath.EndsWith("/"))
    sCleanPath.Shrink(0, 1);

  ezMap<ezString, StatData>::Iterator it = m_Stats.Find(sCleanPath.GetData());

  if (it.IsValid() && it.Value().m_pItem != nullptr)
    return it.Value().m_pItem;

  QTreeWidgetItem* pParent = nullptr;
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

void ezQtMainWidget::SetFavorite(const ezString& sStat, bool bFavorite)
{
  StatData& sd = m_Stats[sStat];

  if (bFavorite)
  {
    m_Favorites.Insert(sStat);

    if (!sd.m_pItemFavorite)
    {
      sd.m_pItemFavorite = new QTreeWidgetItem();
      TreeFavorites->addTopLevelItem(sd.m_pItemFavorite);
      sd.m_pItemFavorite->setData(0, Qt::DisplayRole, sStat.GetData());
      sd.m_pItemFavorite->setData(1, Qt::DisplayRole, sd.m_Value.ConvertTo<ezString>().GetData());
      sd.m_pItemFavorite->setIcon(0, QIcon(":/Icons/Icons/StatFavorite.png"));

      TreeFavorites->resizeColumnToContents(0);
    }
  }
  else
  {
    if (sd.m_pItemFavorite)
    {
      m_Favorites.Remove(sStat);

      delete sd.m_pItemFavorite;
      sd.m_pItemFavorite = nullptr;
    }
  }
}

void ezQtMainWidget::on_TreeStats_itemChanged(QTreeWidgetItem* item, int column)
{
  if (column == 0)
  {
    ezString sPath = item->data(0, Qt::UserRole).toString().toUtf8().data();

    SetFavorite(sPath, (item->checkState(0) == Qt::Checked));
  }
}

void ezQtMainWidget::on_TreeStats_customContextMenuRequested(const QPoint& p)
{
  if (!TreeStats->currentItem())
    return;

  QMenu mSub;
  mSub.setTitle("Show in");
  mSub.setIcon(QIcon(":/Icons/Icons/StatHistory.png"));

  QMenu m;
  m.addMenu(&mSub);

  for (ezInt32 i = 0; i < 10; ++i)
  {
    ezQtMainWindow::s_pWidget->m_pActionShowStatIn[i]->setText(ezQtMainWindow::s_pWidget->m_pStatHistoryWidgets[i]->LineName->text());
    mSub.addAction(ezQtMainWindow::s_pWidget->m_pActionShowStatIn[i]);
  }

  if (TreeStats->currentItem()->childCount() > 0)
    mSub.setEnabled(false);

  m.exec(TreeStats->viewport()->mapToGlobal(p));
}


void ezQtMainWidget::ShowStatIn(bool)
{
  if (!TreeStats->currentItem())
    return;

  QAction* pAction = (QAction*)sender();

  ezInt32 iHistoryWidget = 0;
  for (iHistoryWidget = 0; iHistoryWidget < 10; ++iHistoryWidget)
  {
    if (ezQtMainWindow::s_pWidget->m_pActionShowStatIn[iHistoryWidget] == pAction)
      goto found;
  }

  return;

found:

  ezString sPath = TreeStats->currentItem()->data(0, Qt::UserRole).toString().toUtf8().data();

  ezQtMainWindow::s_pWidget->m_pStatHistoryWidgets[iHistoryWidget]->AddStat(sPath);
}
