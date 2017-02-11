#include <PCH.h>
#include <EditorFramework/Assets/AssetBrowserModel.moc.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QMimeData>
#include <QUrl>

////////////////////////////////////////////////////////////////////////
// ezQtAssetBrowserModel public functions
////////////////////////////////////////////////////////////////////////

struct AssetComparer
{
  AssetComparer(ezQtAssetBrowserModel* model, const ezHashTable<ezUuid, ezAssetInfo*>& allAssets)
    : m_Model(model), m_AllAssets(allAssets)
  {
  }

  bool Less(const ezQtAssetBrowserModel::AssetEntry& a, const ezQtAssetBrowserModel::AssetEntry& b) const
  {
    ezAssetInfo* pInfoA = nullptr;
    ezAssetInfo* pInfoB = nullptr;
    m_AllAssets.TryGetValue(a.m_Guid, pInfoA);
    m_AllAssets.TryGetValue(b.m_Guid, pInfoB);
    if (pInfoA && pInfoB && pInfoA->m_LastAccess.GetSeconds() != pInfoB->m_LastAccess.GetSeconds())
    {
      return pInfoA->m_LastAccess > pInfoB->m_LastAccess;
    }

    ezInt32 iValue = a.m_sSortingKey.Compare(b.m_sSortingKey);
    if (iValue == 0)
    {
      return a.m_Guid < b.m_Guid;
    }
    return iValue < 0;
  }

  EZ_ALWAYS_INLINE bool operator()(const ezQtAssetBrowserModel::AssetEntry& a, const ezQtAssetBrowserModel::AssetEntry& b) const
  {
    return Less(a, b);
  }

  ezQtAssetBrowserModel* m_Model;
  const ezHashTable<ezUuid, ezAssetInfo*>& m_AllAssets;
};

ezQtAssetBrowserModel::ezQtAssetBrowserModel(QObject* pParent)
  : QAbstractItemModel(pParent)
{
  ezAssetCurator::GetSingleton()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtAssetBrowserModel::AssetCuratorEventHandler, this));

  resetModel();
  SetIconMode(true);
  m_bShowItemsInSubFolders = true;

  EZ_VERIFY(connect(ezQtImageCache::GetSingleton(), &ezQtImageCache::ImageLoaded, this, &ezQtAssetBrowserModel::ThumbnailLoaded) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(ezQtImageCache::GetSingleton(), &ezQtImageCache::ImageInvalidated, this, &ezQtAssetBrowserModel::ThumbnailInvalidated) != nullptr, "signal/slot connection failed");
}

ezQtAssetBrowserModel::~ezQtAssetBrowserModel()
{
  ezAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtAssetBrowserModel::AssetCuratorEventHandler, this));
}

void ezQtAssetBrowserModel::AssetCuratorEventHandler(const ezAssetCuratorEvent& e)
{
  switch (e.m_Type)
  {
  case ezAssetCuratorEvent::Type::AssetAdded:
    HandleAsset(e.m_pInfo, true);
    break;
  case ezAssetCuratorEvent::Type::AssetRemoved:
    HandleAsset(e.m_pInfo, false);
    break;
  case ezAssetCuratorEvent::Type::AssetListReset:
    resetModel();
    break;
  case ezAssetCuratorEvent::Type::AssetUpdated:
    QModelIndex idx = index(FindAssetIndex(e.m_AssetGuid), 0);
    if (idx.isValid())
    {
      emit dataChanged(idx, idx);
    }
    break;
  }
}


ezInt32 ezQtAssetBrowserModel::FindAssetIndex(const ezUuid& assetGuid) const
{
  for (ezUInt32 i = 0; i < m_AssetsToDisplay.GetCount(); ++i)
  {
    if (m_AssetsToDisplay[i].m_Guid == assetGuid)
    {
      return i;
    }
  }

  return -1;
}

void ezQtAssetBrowserModel::SetShowItemsInSubFolders(bool bShow)
{
  if (m_bShowItemsInSubFolders == bShow)
    return;

  m_bShowItemsInSubFolders = bShow;

  resetModel();
  emit ShowSubFolderItemsChanged();
}

void ezQtAssetBrowserModel::SetSortByRecentUse(bool bSort)
{
  if (m_bSortByRecentUse == bSort)
    return;

  m_bSortByRecentUse = bSort;

  resetModel();
  emit SortByRecentUseChanged();
}


void ezQtAssetBrowserModel::SetTextFilter(const char* szText)
{
  ezStringBuilder sCleanText = szText;
  sCleanText.MakeCleanPath();

  if (m_sTextFilter == sCleanText)
    return;

  m_sTextFilter = sCleanText;

  resetModel();
  emit TextFilterChanged();
}

void ezQtAssetBrowserModel::SetPathFilter(const char* szPath)
{
  ezStringBuilder sCleanText = szPath;
  sCleanText.MakeCleanPath();

  if (m_sPathFilter == sCleanText)
    return;

  m_sPathFilter = sCleanText;

  resetModel();

  emit PathFilterChanged();
}

void ezQtAssetBrowserModel::SetTypeFilter(const char* szTypes)
{
  if (m_sTypeFilter == szTypes)
    return;

  m_sTypeFilter = szTypes;

  resetModel();

  emit TypeFilterChanged();
}

void ezQtAssetBrowserModel::resetModel()
{
  beginResetModel();

  ezAssetCurator::ezLockedAssetTable AllAssetsLocked = ezAssetCurator::GetSingleton()->GetKnownAssets();
  const ezHashTable<ezUuid, ezAssetInfo*>& AllAssets = *(AllAssetsLocked.operator->());

  m_AssetsToDisplay.Clear();
  m_AssetsToDisplay.Reserve(AllAssets.GetCount());

  AssetEntry ae;
  // last access > filename
  for (auto it = AllAssets.GetIterator(); it.IsValid(); ++it)
  {
    ezAssetInfo* pInfo = it.Value();
    if (IsAssetFiltered(pInfo))
      continue;

    Init(ae, pInfo);

    m_AssetsToDisplay.PushBack(ae);
  }

  AssetComparer cmp(this, AllAssets);
  m_AssetsToDisplay.Sort(cmp);

  endResetModel();
}


bool ezQtAssetBrowserModel::IsAssetFiltered(const ezAssetInfo* pInfo) const
{
  if (!m_sPathFilter.IsEmpty())
  {
    // if the string is not found in the path, ignore this asset
    if (!pInfo->m_sDataDirRelativePath.StartsWith_NoCase(m_sPathFilter))
      return true;

    if (!m_bShowItemsInSubFolders)
    {
      // do we find another path separator after the prefix path?
      // if so, there is a sub-folder, and thus we ignore it
      if (ezStringUtils::FindSubString(pInfo->m_sDataDirRelativePath.GetData() + m_sPathFilter.GetElementCount() + 1, "/") != nullptr)
        return true;
    }
  }

  if (!m_sTextFilter.IsEmpty())
  {
    // if the string is not found in the path, ignore this asset
    if (pInfo->m_sDataDirRelativePath.FindSubString_NoCase(m_sTextFilter) == nullptr)
      return true;
  }

  if (!m_sTypeFilter.IsEmpty())
  {
    ezStringBuilder sTemp;
    sTemp.Set(";", pInfo->m_Info.m_sAssetTypeName, ";");

    if (!m_sTypeFilter.FindSubString(sTemp))
      return true;
  }
  return false;
}

void ezQtAssetBrowserModel::HandleAsset(const ezAssetInfo* pInfo, bool bAdd)
{
  if (IsAssetFiltered(pInfo))
    return;

  ezAssetCurator::ezLockedAssetTable AllAssetsLocked = ezAssetCurator::GetSingleton()->GetKnownAssets();
  const ezHashTable<ezUuid, ezAssetInfo*>& AllAssets = *(AllAssetsLocked.operator->());

  AssetEntry ae;
  Init(ae, pInfo);

  AssetComparer cmp(this, AllAssets);
  AssetEntry* pLB = std::lower_bound(begin(m_AssetsToDisplay), end(m_AssetsToDisplay), ae, cmp);
  ezUInt32 uiInsertIndex = pLB - m_AssetsToDisplay.GetData();

  if (bAdd)
  {
    // Equal?
    if (uiInsertIndex < m_AssetsToDisplay.GetCount() && !cmp.Less(*pLB, ae) && !cmp.Less(ae, *pLB))
      return;

    beginInsertRows(QModelIndex(), uiInsertIndex, uiInsertIndex);
    m_AssetsToDisplay.Insert(ae, uiInsertIndex);
    endInsertRows();
  }
  else
  {
    // Equal?
    if (uiInsertIndex < m_AssetsToDisplay.GetCount() && !cmp.Less(*pLB, ae) && !cmp.Less(ae, *pLB))
    {
      beginRemoveRows(QModelIndex(), uiInsertIndex, uiInsertIndex);
      m_AssetsToDisplay.RemoveAt(uiInsertIndex);
      endRemoveRows();
    }
  }
}


////////////////////////////////////////////////////////////////////////
// ezQtAssetBrowserModel QAbstractItemModel functions
////////////////////////////////////////////////////////////////////////

void ezQtAssetBrowserModel::ThumbnailLoaded(QString sPath, QModelIndex index, QVariant UserData1, QVariant UserData2)
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

void ezQtAssetBrowserModel::ThumbnailInvalidated(QString sPath, ezUInt32 uiImageID)
{
  for (ezUInt32 i = 0; i < m_AssetsToDisplay.GetCount(); ++i)
  {
    if (m_AssetsToDisplay[i].m_uiThumbnailID == uiImageID)
    {
      QModelIndex idx = createIndex(i, 0);
      emit dataChanged(idx, idx);
      return;
    }
  }
}

QVariant ezQtAssetBrowserModel::data(const QModelIndex& index, int role) const
{
  if (!index.isValid() || index.column() != 0)
    return QVariant();

  const ezInt32 iRow = index.row();
  if (iRow < 0 || iRow >= (ezInt32)m_AssetsToDisplay.GetCount())
    return QVariant();

  const auto& asset = m_AssetsToDisplay[iRow];
  const ezUuid AssetGuid = asset.m_Guid;
  const ezAssetCurator::ezLockedAssetInfo pAssetInfo = ezAssetCurator::GetSingleton()->GetAssetInfo2(AssetGuid);

  // this can happen when a file was just changed on disk, e.g. got deleted
  if (pAssetInfo == nullptr)
    return QVariant();

  switch (role)
  {
  case Qt::DisplayRole:
    {
      ezStringBuilder sFilename = ezPathUtils::GetFileName(pAssetInfo->m_sDataDirRelativePath);
      return QString::fromUtf8(sFilename);
    }
    break;

  case Qt::ToolTipRole:
    return QString::fromUtf8(pAssetInfo->m_sDataDirRelativePath.GetData());

  case Qt::DecorationRole:
    {
      if (m_bIconMode)
      {
        ezStringBuilder sThumbnailPath = ezAssetDocumentManager::GenerateResourceThumbnailPath(pAssetInfo->m_sAbsolutePath);

        ezUInt64 uiUserData1, uiUserData2;
        AssetGuid.GetValues(uiUserData1, uiUserData2);

        const QPixmap* pThumbnailPixmap = ezQtImageCache::GetSingleton()->QueryPixmapForType(pAssetInfo->m_Info.m_sAssetTypeName, sThumbnailPath, index, QVariant(uiUserData1), QVariant(uiUserData2), &asset.m_uiThumbnailID);

        return *pThumbnailPixmap;
      }
    }
    break;

  case UserRoles::AssetGuid:
    {
      //QVariant var;
      //var.setValue(pAssetInfo->m_Info.m_DocumentID);
      return qVariantFromValue(pAssetInfo->m_Info.m_DocumentID);// QString::fromUtf8(ezConversionUtils::ToString(pAssetInfo->m_Info.m_DocumentID).GetData());
    }
  case UserRoles::AbsolutePath:
    return QString::fromUtf8(pAssetInfo->m_sAbsolutePath);

  case UserRoles::RelativePath:
    return QString::fromUtf8(pAssetInfo->m_sDataDirRelativePath);

  case UserRoles::AssetIconPath:
    {
      ezStringBuilder sIconName;
      sIconName.Set(":/AssetIcons/", pAssetInfo->m_Info.m_sAssetTypeName);
      sIconName.ReplaceAll(" ", "_");
      sIconName.ReplaceAll("(", "");
      sIconName.ReplaceAll(")", "");

      return ezQtUiServices::GetCachedPixmapResource(sIconName.GetData());
    }

  case UserRoles::TransformState:
    return (int)pAssetInfo->m_TransformState;
  }

  return QVariant();
}

Qt::ItemFlags ezQtAssetBrowserModel::flags(const QModelIndex& index) const
{
  if (!index.isValid())
    return 0;

  return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
}

QVariant ezQtAssetBrowserModel::headerData(int section, Qt::Orientation orientation, int role) const
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

QModelIndex ezQtAssetBrowserModel::index(int row, int column, const QModelIndex& parent) const
{
  if (parent.isValid() || column != 0)
    return QModelIndex();

  return createIndex(row, column);
}

QModelIndex ezQtAssetBrowserModel::parent(const QModelIndex& index) const
{
  return QModelIndex();
}

int ezQtAssetBrowserModel::rowCount(const QModelIndex& parent) const
{
  if (parent.isValid())
    return 0;

  return (int)m_AssetsToDisplay.GetCount();
}

int ezQtAssetBrowserModel::columnCount(const QModelIndex& parent) const
{
  return 1;
}

QStringList ezQtAssetBrowserModel::mimeTypes() const
{
  QStringList types;
  types << "application/ezEditor.AssetGuid";
  return types;
}

QMimeData* ezQtAssetBrowserModel::mimeData(const QModelIndexList& indexes) const
{
  QMimeData* mimeData = new QMimeData();
  QByteArray encodedData;
  QDataStream stream(&encodedData, QIODevice::WriteOnly);

  QString sGuids;
  QList<QUrl> urls;

  stream << indexes.size();
  for (int i = 0; i < indexes.size(); ++i)
  {
    QString sGuid(ezConversionUtils::ToString(data(indexes[i], UserRoles::AssetGuid).value<ezUuid>()).GetData());
    QString sPath = data(indexes[i], UserRoles::AbsolutePath).toString();

    stream << sGuid;
    sGuids += sPath + "\n";

    urls.push_back(QUrl::fromLocalFile(sPath));
  }

  mimeData->setData("application/ezEditor.AssetGuid", encodedData);
  mimeData->setText(sGuids);
  mimeData->setUrls(urls);
  return mimeData;
}

void ezQtAssetBrowserModel::Init(AssetEntry& ae, const ezAssetInfo* pInfo)
{
  ae.m_Guid = pInfo->m_Info.m_DocumentID;
  ae.m_uiThumbnailID = (ezUInt32)-1;

  ezStringBuilder sTemp = ezPathUtils::GetFileName(pInfo->m_sDataDirRelativePath.GetData(), pInfo->m_sDataDirRelativePath.GetData() + pInfo->m_sDataDirRelativePath.GetElementCount());
  sTemp.ToLower();
  ae.m_sSortingKey = sTemp;
}
