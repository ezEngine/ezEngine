#include <InspectorPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <Inspector/MainWindow.moc.h>
#include <qdir.h>
#include <qstandardpaths.h>

void ezQtMainWindow::SaveFavourites()
{
  QString sFile = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
  QDir d;
  d.mkpath(sFile);

  sFile.append("/Favourites.stats");

  QFile f(sFile);
  if (!f.open(QIODevice::WriteOnly))
    return;

  QDataStream stream(&f);

  const ezUInt32 uiNumFavourites = m_Favourites.GetCount();
  stream << uiNumFavourites;

  for (ezSet<ezString>::Iterator it = m_Favourites.GetIterator(); it.IsValid(); ++it)
  {
    const QString s = it.Key().GetData();
    stream << s;
  }

  f.close();
}

void ezQtMainWindow::LoadFavourites()
{
  QString sFile = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
  QDir d;
  d.mkpath(sFile);
  sFile.append("/Favourites.stats");

  QFile f(sFile);
  if (!f.open(QIODevice::ReadOnly))
    return;

  m_Favourites.Clear();

  QDataStream stream(&f);

  ezUInt32 uiNumFavourites = 0;
  stream >> uiNumFavourites;

  for (ezUInt32 i = 0; i < uiNumFavourites; ++i)
  {
    QString s;
    stream >> s;

    ezString ezs = s.toUtf8().data();

    m_Favourites.Insert(ezs);
  }

  f.close();
}

void ezQtMainWindow::ResetStats()
{
  m_Stats.Clear();
  TreeStats->clear();
  TreeFavourites->clear();
}

void ezQtMainWindow::UpdateStats()
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
    LabelStatus->setText(
        "<p><span style=\" font-weight:600;\">Status: </span><span style=\" font-weight:600; color:#00aa00;\">Connected</span></p>");
    LabelServer->setText(QString::fromUtf8("<p>Server: %1:%2</p>").arg(ezTelemetry::GetServerIP()).arg(ezTelemetry::s_uiPort));
  }

  UpdateAlwaysOnTop();
}

QTreeWidgetItem* ezQtMainWindow::CreateStat(const char* szPath, bool bParent)
{
  ezStringBuilder sCleanPath = szPath;
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

void ezQtMainWindow::SetFavourite(const ezString& sStat, bool bFavourite)
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
      sd.m_pItemFavourite->setData(1, Qt::DisplayRole, sd.m_Value.ConvertTo<ezString>().GetData());
      sd.m_pItemFavourite->setIcon(0, QIcon(":/Icons/Icons/StatFavourite.png"));

      TreeFavourites->resizeColumnToContents(0);
    }
  }
  else
  {
    if (sd.m_pItemFavourite)
    {
      m_Favourites.Remove(sStat);

      delete sd.m_pItemFavourite;
      sd.m_pItemFavourite = nullptr;
    }
  }
}

void ezQtMainWindow::on_TreeStats_itemChanged(QTreeWidgetItem* item, int column)
{
  if (column == 0)
  {
    ezString sPath = item->data(0, Qt::UserRole).toString().toUtf8().data();

    SetFavourite(sPath, (item->checkState(0) == Qt::Checked));
  }
}

void ezQtMainWindow::on_TreeStats_customContextMenuRequested(const QPoint& p)
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
    m_pActionShowStatIn[i]->setText(m_pStatHistoryWidgets[i]->LineName->text());
    mSub.addAction(m_pActionShowStatIn[i]);
  }

  if (TreeStats->currentItem()->childCount() > 0)
    mSub.setEnabled(false);

  m.exec(TreeStats->viewport()->mapToGlobal(p));
}


void ezQtMainWindow::ShowStatIn()
{
  if (!TreeStats->currentItem())
    return;

  QAction* pAction = (QAction*)sender();

  ezInt32 iHistoryWidget = 0;
  for (iHistoryWidget = 0; iHistoryWidget < 10; ++iHistoryWidget)
  {
    if (m_pActionShowStatIn[iHistoryWidget] == pAction)
      goto found;
  }

  return;

found:

  ezString sPath = TreeStats->currentItem()->data(0, Qt::UserRole).toString().toUtf8().data();

  m_pStatHistoryWidgets[iHistoryWidget]->AddStat(sPath);
}
