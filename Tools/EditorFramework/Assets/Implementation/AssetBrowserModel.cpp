#include <PCH.h>
#include <EditorFramework/Assets/AssetBrowserModel.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <Foundation/Types/Uuid.h>
#include <Foundation/Logging/Log.h>
#include <QPixmap>
#include <QMimeData>
#include <QUrl>

////////////////////////////////////////////////////////////////////////
// ezAssetBrowserModel public functions
////////////////////////////////////////////////////////////////////////

ezAssetBrowserModel::ezAssetBrowserModel(QObject* pParent)
  : QAbstractItemModel(pParent)
{
  ezAssetCurator::GetInstance()->m_Events.AddEventHandler(ezMakeDelegate(&ezAssetBrowserModel::AssetCuratorEventHandler, this));

  resetModel();
  SetIconMode(true);
  m_bShowItemsInSubFolders = true;

  EZ_VERIFY(connect(QtImageCache::GetInstance(), SIGNAL(ImageLoaded(QString, QModelIndex, QVariant, QVariant)), this, SLOT(ThumbnailLoaded(QString, QModelIndex, QVariant, QVariant))) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(QtImageCache::GetInstance(), SIGNAL(ImageInvalidated(QString, ezUInt32)), this, SLOT(ThumbnailInvalidated(QString, ezUInt32))) != nullptr, "signal/slot connection failed");
}

ezAssetBrowserModel::~ezAssetBrowserModel()
{
  ezAssetCurator::GetInstance()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezAssetBrowserModel::AssetCuratorEventHandler, this));
}

void ezAssetBrowserModel::AssetCuratorEventHandler(const ezAssetCurator::Event& e)
{
  switch (e.m_Type)
  {
  case ezAssetCurator::Event::Type::AssetListReset:
  case ezAssetCurator::Event::Type::AssetAdded:
  case ezAssetCurator::Event::Type::AssetRemoved:
    resetModel();
    break;
  }
}

void ezAssetBrowserModel::SetShowItemsInSubFolders(bool bShow)
{
  if (m_bShowItemsInSubFolders == bShow)
    return;

  m_bShowItemsInSubFolders = bShow;

  resetModel();
  emit ShowSubFolderItemsChanged();
}

void ezAssetBrowserModel::SetSortByRecentUse(bool bSort)
{
  if (m_bSortByRecentUse == bSort)
    return;

  m_bSortByRecentUse = bSort;

  resetModel();
  emit SortByRecentUseChanged();
}


void ezAssetBrowserModel::SetTextFilter(const char* szText)
{
  ezStringBuilder sCleanText = szText;
  sCleanText.MakeCleanPath();

  if (m_sTextFilter == sCleanText)
    return;

  m_sTextFilter = sCleanText;

  resetModel();
  emit TextFilterChanged();
}

void ezAssetBrowserModel::SetPathFilter(const char* szPath)
{
  ezStringBuilder sCleanText = szPath;
  sCleanText.MakeCleanPath();

  if (m_sPathFilter == sCleanText)
    return;

  m_sPathFilter = sCleanText;

  resetModel();

  emit PathFilterChanged();
}

void ezAssetBrowserModel::SetTypeFilter(const char* szTypes)
{
  if (m_sTypeFilter == szTypes)
    return;

  m_sTypeFilter = szTypes;

  resetModel();

  emit TypeFilterChanged();
}

void ezAssetBrowserModel::resetModel()
{
  beginResetModel();

  const auto& AllAssets = ezAssetCurator::GetInstance()->GetKnownAssets();

  m_AssetsToDisplay.Clear();
  m_AssetsToDisplay.Reserve(AllAssets.GetCount());

  ezStringBuilder sTemp, sTemp2;
  AssetEntry ae;

  const ezTime tNow = ezTime::Now();

  for (auto it = AllAssets.GetIterator(); it.IsValid(); ++it)
  {
    if (!m_sPathFilter.IsEmpty())
    {
      // if the string is not found in the path, ignore this asset
      if (!it.Value()->m_sRelativePath.StartsWith_NoCase(m_sPathFilter))
          continue;

      if (!m_bShowItemsInSubFolders)
      {
        // do we find another path separator after the prefix path?
        // if so, there is a sub-folder, and thus we ignore it
        if (ezStringUtils::FindSubString(it.Value()->m_sRelativePath.GetData() + m_sPathFilter.GetElementCount() + 1, "/") != nullptr)
          continue;
      }
    }

    if (!m_sTextFilter.IsEmpty())
    {
      // if the string is not found in the path, ignore this asset
      if (it.Value()->m_sRelativePath.FindSubString_NoCase(m_sTextFilter) == nullptr)
        continue;
    }

    if (!m_sTypeFilter.IsEmpty())
    {
      sTemp.Set(";", it.Value()->m_Info.m_sAssetTypeName, ";");

      if (!m_sTypeFilter.FindSubString(sTemp))
        continue;
    }

    ae.m_Guid = it.Key();

    sTemp2 = it.Value()->m_sRelativePath;
    sTemp = sTemp2.GetFileName();

    if (m_bSortByRecentUse)
    {
      sTemp2 = sTemp;
      sTemp.Format("%012.1f - %s", (tNow - it.Value()->m_LastAccess).GetSeconds(), sTemp2.GetData());
    }

    sTemp.ToLower();
    ae.m_sSortingKey = sTemp;

    m_AssetsToDisplay.PushBack(ae);
  }

  m_AssetsToDisplay.Sort();

  endResetModel();
}

////////////////////////////////////////////////////////////////////////
// ezAssetBrowserModel QAbstractItemModel functions
////////////////////////////////////////////////////////////////////////

void ezAssetBrowserModel::ThumbnailLoaded(QString sPath, QModelIndex index, QVariant UserData1, QVariant UserData2)
{
  const ezUuid guid(UserData1.toULongLong(), UserData2.toULongLong());

  for (ezUInt32 i = 0; i < m_AssetsToDisplay.GetCount(); ++i)
  {
    if (m_AssetsToDisplay[i].m_Guid == guid)
    {
      QModelIndex idx = createIndex(i, 0);
      emit dataChanged(idx, idx);
      return;
    }
  }
}

void ezAssetBrowserModel::ThumbnailInvalidated(QString sPath, ezUInt32 uiImageID)
{
  for (ezUInt32 i = 0; i < m_AssetsToDisplay.GetCount(); ++i)
  {
    if (m_AssetsToDisplay[i].m_uiThumbnailID = uiImageID)
    {
      QModelIndex idx = createIndex(i, 0);
      emit dataChanged(idx, idx);
      return;
    }
  }
}

QVariant ezAssetBrowserModel::data(const QModelIndex& index, int role) const
{
  if (!index.isValid() || index.column() != 0)
    return QVariant();

  const ezInt32 iRow = index.row();
  if (iRow < 0 || iRow >= (ezInt32)m_AssetsToDisplay.GetCount())
    return QVariant();
  
  const auto& asset = m_AssetsToDisplay[iRow];
  const ezUuid AssetGuid = asset.m_Guid;
  const ezAssetCurator::AssetInfo* pAssetInfo = ezAssetCurator::GetInstance()->GetAssetInfo(AssetGuid);

  EZ_ASSERT_DEV(pAssetInfo != nullptr, "Invalid Pointer !!!!`1`1sonceleven");

  switch (role)
  {
  case Qt::DisplayRole:
    {
      ezStringBuilder sFilename = ezPathUtils::GetFileName(pAssetInfo->m_sRelativePath);
      return QString::fromUtf8(sFilename);
    }
    break;

  case Qt::ToolTipRole:
    return QString::fromUtf8(pAssetInfo->m_sRelativePath.GetData());

  case Qt::DecorationRole:
    {
      if (m_bIconMode)
      {
        ezStringBuilder sThumbnailPath = ezAssetDocumentManager::GenerateResourceThumbnailPath(pAssetInfo->m_sAbsolutePath);

        ezUInt64 uiUserData1, uiUserData2;
        AssetGuid.GetValues(uiUserData1, uiUserData2);

        const QPixmap* pThumbnailPixmap = QtImageCache::QueryPixmap(sThumbnailPath, index, QVariant(uiUserData1), QVariant(uiUserData2), &asset.m_uiThumbnailID);

        return *pThumbnailPixmap;
      }
    }
    break;

  case Qt::UserRole + 0:
    return QString::fromUtf8(ezConversionUtils::ToString(pAssetInfo->m_Info.m_DocumentID).GetData());

  case Qt::UserRole + 1:
    return QString::fromUtf8(pAssetInfo->m_sAbsolutePath);

  case Qt::UserRole + 2:
    return QString::fromUtf8(pAssetInfo->m_sRelativePath);
  }

  return QVariant();
}

Qt::ItemFlags ezAssetBrowserModel::flags(const QModelIndex& index) const
{
  if (!index.isValid())
    return 0;

  return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
}

QVariant ezAssetBrowserModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
  {
    switch (section)
    {
    case 0:
      return QString("Asset");
    }
  }
  return QVariant();
}

QModelIndex ezAssetBrowserModel::index(int row, int column, const QModelIndex& parent) const
{
  if (parent.isValid() || column != 0)
    return QModelIndex();

  return createIndex(row, column);
}

QModelIndex ezAssetBrowserModel::parent(const QModelIndex& index) const
{
  return QModelIndex();
}

int ezAssetBrowserModel::rowCount(const QModelIndex& parent) const
{
  if (parent.isValid())
    return 0;

  return (int)m_AssetsToDisplay.GetCount();
}

int ezAssetBrowserModel::columnCount(const QModelIndex& parent) const
{
  return 1;
}

QStringList ezAssetBrowserModel::mimeTypes() const
{
  QStringList types;
  types << "application/ezEditor.AssetGuid";
  return types;
}

QMimeData* ezAssetBrowserModel::mimeData(const QModelIndexList& indexes) const
{
  QMimeData* mimeData = new QMimeData();
  QByteArray encodedData;
  QDataStream stream(&encodedData, QIODevice::WriteOnly);

  QString sGuids;
  QList<QUrl> urls;

  stream << indexes.size();
  for (int i = 0; i < indexes.size(); ++i)
  {
    QString sGuid = data(indexes[i], Qt::UserRole + 0).toString();
    QString sPath = data(indexes[i], Qt::UserRole + 1).toString();

    stream << sGuid;
    sGuids += sPath + "\n";

    urls.push_back(QUrl::fromLocalFile(sPath));
  }

  mimeData->setData("application/ezEditor.AssetGuid", encodedData);
  mimeData->setText(sGuids);
  mimeData->setUrls(urls);
  return mimeData;
}







