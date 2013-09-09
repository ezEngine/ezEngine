#include <Inspector/StatsWidget.moc.h>
#include <Foundation/Communication/Telemetry.h>
#include <MainWindow.moc.h>
#include <qstandardpaths.h>
#include <qfile.h>
#include <qdir.h>

ezStatsWidget* ezStatsWidget::s_pWidget = NULL;

ezStatsWidget::ezStatsWidget(QWidget* parent) : QDockWidget (parent)
{
  s_pWidget = this;

  setupUi (this);

  ResetStats();

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
}

ezStatsWidget::~ezStatsWidget()
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

void ezStatsWidget::ResetStats()
{
  m_Stats.Clear();
  TreeStats->clear();
  TreeFavourites->clear();
}


void ezStatsWidget::UpdateStats()
{
}

QTreeWidgetItem* ezStatsWidget::CreateStat(const char* szPath, bool bParent)
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
      pParent->setExpanded(true);
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

void ezStatsWidget::ProcessTelemetry(void* pUnuseed)
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
}

void ezStatsWidget::SetFavourite(const ezString& sStat, bool bFavourite)
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

void ezStatsWidget::on_TreeStats_itemChanged(QTreeWidgetItem* item, int column)
{
  if (column == 0)
  {
    int i = 0;

    ezString sPath = item->data(0, Qt::UserRole).toString().toUtf8().data();

    SetFavourite(sPath, (item->checkState(0) == Qt::Checked));

  }
}



