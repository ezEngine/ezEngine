#include <PCH.h>

#include <EditorFramework/Assets/AssetBrowserModel.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QMimeData>
#include <QUrl>

ezQtAssetFilter::ezQtAssetFilter(QObject* pParent)
    : QObject(pParent)
{
}

////////////////////////////////////////////////////////////////////////
// ezQtAssetBrowserModel public functions
////////////////////////////////////////////////////////////////////////

struct AssetComparer
{
  AssetComparer(ezQtAssetBrowserModel* model, const ezMap<ezUuid, ezSubAsset>& allAssets)
      : m_Model(model)
      , m_AllAssets(allAssets)
  {
  }

  EZ_ALWAYS_INLINE bool Less(const ezQtAssetBrowserModel::AssetEntry& a, const ezQtAssetBrowserModel::AssetEntry& b) const
  {
    const ezSubAsset* pInfoA = &m_AllAssets.Find(a.m_Guid).Value();
    const ezSubAsset* pInfoB = &m_AllAssets.Find(b.m_Guid).Value();

    return m_Model->m_pFilter->Less(pInfoA, pInfoB);
  }

  EZ_ALWAYS_INLINE bool operator()(const ezQtAssetBrowserModel::AssetEntry& a, const ezQtAssetBrowserModel::AssetEntry& b) const
  {
    return Less(a, b);
  }

  ezQtAssetBrowserModel* m_Model;
  const ezMap<ezUuid, ezSubAsset>& m_AllAssets;
};

ezQtAssetBrowserModel::ezQtAssetBrowserModel(QObject* pParent, ezQtAssetFilter* pFilter)
    : QAbstractItemModel(pParent)
    , m_pFilter(pFilter)
{
  EZ_ASSERT_DEBUG(pFilter != nullptr, "ezQtAssetBrowserModel requires a valid filter.");
  connect(pFilter, &ezQtAssetFilter::FilterChanged, this, [this]() { resetModel(); });

  ezAssetCurator::GetSingleton()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtAssetBrowserModel::AssetCuratorEventHandler, this));

  resetModel();
  SetIconMode(true);

  EZ_VERIFY(connect(ezQtImageCache::GetSingleton(), &ezQtImageCache::ImageLoaded, this, &ezQtAssetBrowserModel::ThumbnailLoaded) != nullptr,
            "signal/slot connection failed");
  EZ_VERIFY(connect(ezQtImageCache::GetSingleton(), &ezQtImageCache::ImageInvalidated, this,
                    &ezQtAssetBrowserModel::ThumbnailInvalidated) != nullptr,
            "signal/slot connection failed");
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
      HandleAsset(e.m_pInfo, AssetOp::Add);
      break;
    case ezAssetCuratorEvent::Type::AssetRemoved:
      HandleAsset(e.m_pInfo, AssetOp::Remove);
      break;
    case ezAssetCuratorEvent::Type::AssetListReset:
      resetModel();
      break;
    case ezAssetCuratorEvent::Type::AssetUpdated:
      HandleAsset(e.m_pInfo, AssetOp::Updated);
      break;
  }
}


ezInt32 ezQtAssetBrowserModel::FindAssetIndex(const ezUuid& assetGuid) const
{
  if (!m_DisplayedEntries.Contains(assetGuid))
    return -1;

  for (ezUInt32 i = 0; i < m_AssetsToDisplay.GetCount(); ++i)
  {
    if (m_AssetsToDisplay[i].m_Guid == assetGuid)
    {
      return i;
    }
  }

  return -1;
}

void ezQtAssetBrowserModel::resetModel()
{
  beginResetModel();

  ezAssetCurator::ezLockedSubAssetTable AllAssetsLocked = ezAssetCurator::GetSingleton()->GetKnownSubAssets();
  const ezMap<ezUuid, ezSubAsset>& AllAssets = *(AllAssetsLocked.operator->());

  m_AssetsToDisplay.Clear();
  m_AssetsToDisplay.Reserve(AllAssets.GetCount());
  m_DisplayedEntries.Clear();

  AssetEntry ae;
  // last access > filename
  for (auto it = AllAssets.GetIterator(); it.IsValid(); ++it)
  {
    const ezSubAsset* pSub = &it.Value();
    if (m_pFilter->IsAssetFiltered(pSub))
      continue;

    Init(ae, pSub);

    m_AssetsToDisplay.PushBack(ae);
    m_DisplayedEntries.Insert(ae.m_Guid);
  }

  AssetComparer cmp(this, AllAssets);
  m_AssetsToDisplay.Sort(cmp);

  endResetModel();
  EZ_ASSERT_DEBUG(m_AssetsToDisplay.GetCount() == m_DisplayedEntries.GetCount(), "Implementation error: Set and sorted list diverged");
}

void ezQtAssetBrowserModel::HandleAsset(const ezSubAsset* pInfo, AssetOp op)
{
  if (m_pFilter->IsAssetFiltered(pInfo))
  {
    // TODO: Due to file system watcher weirdness the m_sDataDirRelativePath can be empty at this point when renaming files
    // really rare haven't reproed it yet but that case crashes when getting the name so early out that.
    if (!m_DisplayedEntries.Contains(pInfo->m_Data.m_Guid) || pInfo->m_pAssetInfo->m_sDataDirRelativePath.IsEmpty())
    {
      return;
    }

    // Filtered but still exists, remove it.
    op = AssetOp::Remove;
  }

  ezAssetCurator::ezLockedSubAssetTable AllAssetsLocked = ezAssetCurator::GetSingleton()->GetKnownSubAssets();
  const ezMap<ezUuid, ezSubAsset>& AllAssets = *(AllAssetsLocked.operator->());

  AssetEntry ae;
  Init(ae, pInfo);

  AssetComparer cmp(this, AllAssets);
  AssetEntry* pLB = std::lower_bound(begin(m_AssetsToDisplay), end(m_AssetsToDisplay), ae, cmp);
  ezUInt32 uiInsertIndex = pLB - m_AssetsToDisplay.GetData();
  // TODO: Due to sorting issues the above can fail (we need to add a sorting model ontop of this as we use mutable data (name) for sorting.
  if (uiInsertIndex >= m_AssetsToDisplay.GetCount())
  {
    for (ezUInt32 i = 0; i < m_AssetsToDisplay.GetCount(); i++)
    {
      AssetEntry& displayEntry = m_AssetsToDisplay[i];
      if (!cmp.Less(displayEntry, ae) && !cmp.Less(ae, displayEntry))
      {
        uiInsertIndex = i;
        pLB = &displayEntry;
        break;
      }
    }
  }

  if (op == AssetOp::Add)
  {
    // Equal?
    if (uiInsertIndex < m_AssetsToDisplay.GetCount() && !cmp.Less(*pLB, ae) && !cmp.Less(ae, *pLB))
      return;

    beginInsertRows(QModelIndex(), uiInsertIndex, uiInsertIndex);
    m_AssetsToDisplay.Insert(ae, uiInsertIndex);
    m_DisplayedEntries.Insert(pInfo->m_Data.m_Guid);
    endInsertRows();
  }
  else if (op == AssetOp::Remove)
  {
    // Equal?
    if (uiInsertIndex < m_AssetsToDisplay.GetCount() && !cmp.Less(*pLB, ae) && !cmp.Less(ae, *pLB))
    {
      beginRemoveRows(QModelIndex(), uiInsertIndex, uiInsertIndex);
      m_AssetsToDisplay.RemoveAtAndCopy(uiInsertIndex);
      m_DisplayedEntries.Remove(pInfo->m_Data.m_Guid);
      endRemoveRows();
    }
  }
  else
  {
    // Equal?
    if (uiInsertIndex < m_AssetsToDisplay.GetCount() && !cmp.Less(*pLB, ae) && !cmp.Less(ae, *pLB))
    {
      QModelIndex idx = index(uiInsertIndex, 0);
      Q_EMIT dataChanged(idx, idx);
    }
    else
    {
      ezInt32 oldIndex = FindAssetIndex(pInfo->m_Data.m_Guid);
      if (oldIndex != -1)
      {
        // Name has changed, remove old entry
        beginRemoveRows(QModelIndex(), oldIndex, oldIndex);
        m_AssetsToDisplay.RemoveAtAndCopy(oldIndex);
        m_DisplayedEntries.Remove(pInfo->m_Data.m_Guid);
        endRemoveRows();
      }
      HandleAsset(pInfo, AssetOp::Add);
    }
  }
  EZ_ASSERT_DEBUG(m_AssetsToDisplay.GetCount() == m_DisplayedEntries.GetCount(), "Implementation error: Set and sorted list diverged");
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
      Q_EMIT dataChanged(idx, idx);
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
      Q_EMIT dataChanged(idx, idx);
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
  const ezAssetCurator::ezLockedSubAsset pSubAsset = ezAssetCurator::GetSingleton()->GetSubAsset(AssetGuid);

  // this can happen when a file was just changed on disk, e.g. got deleted
  if (pSubAsset == nullptr)
    return QVariant();

  switch (role)
  {
    case Qt::DisplayRole:
    {
      ezStringBuilder sFilename = pSubAsset->GetName();
      return QString::fromUtf8(sFilename);
    }
    break;

    case Qt::ToolTipRole:
    {
      ezStringBuilder sToolTip = pSubAsset->GetName();
      sToolTip.Append("\n", pSubAsset->m_pAssetInfo->m_sDataDirRelativePath);
      sToolTip.Append("\nTransform State: ");
      switch (pSubAsset->m_pAssetInfo->m_TransformState)
      {
        case ezAssetInfo::Unknown:
          sToolTip.Append("Unknown");
          break;
        case ezAssetInfo::UpToDate:
          sToolTip.Append("Up To Date");
          break;
        case ezAssetInfo::Updating:
          sToolTip.Append("Updating");
          break;
        case ezAssetInfo::NeedsTransform:
          sToolTip.Append("Needs Transform");
          break;
        case ezAssetInfo::NeedsThumbnail:
          sToolTip.Append("Needs Thumbnail");
          break;
        case ezAssetInfo::TransformError:
          sToolTip.Append("Transform Error");
          break;
        case ezAssetInfo::MissingDependency:
          sToolTip.Append("Missing Dependency");
          break;
        case ezAssetInfo::MissingReference:
          sToolTip.Append("Missing Reference");
          break;
        default:
          break;
      }

      return QString::fromUtf8(sToolTip);
    }
    case Qt::DecorationRole:
    {
      if (m_bIconMode)
      {
        ezStringBuilder sThumbnailPath = ezAssetDocumentManager::GenerateResourceThumbnailPath(pSubAsset->m_pAssetInfo->m_sAbsolutePath);

        ezUInt64 uiUserData1, uiUserData2;
        AssetGuid.GetValues(uiUserData1, uiUserData2);

        const QPixmap* pThumbnailPixmap =
            ezQtImageCache::GetSingleton()->QueryPixmapForType(pSubAsset->m_Data.m_sAssetTypeName, sThumbnailPath, index,
                                                               QVariant(uiUserData1), QVariant(uiUserData2), &asset.m_uiThumbnailID);

        return *pThumbnailPixmap;
      }
      else
      {
        ezStringBuilder sIconName;
        sIconName.Set(":/AssetIcons/", pSubAsset->m_Data.m_sAssetTypeName);
        sIconName.ReplaceAll(" ", "_");
        sIconName.ReplaceAll("(", "");
        sIconName.ReplaceAll(")", "");

        return ezQtUiServices::GetCachedPixmapResource(sIconName.GetData());
      }
    }
    break;

    case UserRoles::SubAssetGuid:
    {
      return qVariantFromValue(pSubAsset->m_Data.m_Guid);
    }
    case UserRoles::AssetGuid:
    {
      return qVariantFromValue(pSubAsset->m_pAssetInfo->m_Info->m_DocumentID);
    }
    case UserRoles::AbsolutePath:
      return QString::fromUtf8(pSubAsset->m_pAssetInfo->m_sAbsolutePath);

    case UserRoles::RelativePath:
      return QString::fromUtf8(pSubAsset->m_pAssetInfo->m_sDataDirRelativePath);

    case UserRoles::AssetIconPath:
    {
      ezStringBuilder sIconName;
      sIconName.Set(":/AssetIcons/", pSubAsset->m_Data.m_sAssetTypeName);
      sIconName.ReplaceAll(" ", "_");
      sIconName.ReplaceAll("(", "");
      sIconName.ReplaceAll(")", "");

      return ezQtUiServices::GetCachedPixmapResource(sIconName.GetData());
    }

    case UserRoles::TransformState:
      return (int)pSubAsset->m_pAssetInfo->m_TransformState;
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

  ezStringBuilder tmp;

  stream << indexes.size();
  for (int i = 0; i < indexes.size(); ++i)
  {
    QString sGuid(ezConversionUtils::ToString(data(indexes[i], UserRoles::SubAssetGuid).value<ezUuid>(), tmp).GetData());
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

void ezQtAssetBrowserModel::Init(AssetEntry& ae, const ezSubAsset* pInfo)
{
  ae.m_Guid = pInfo->m_Data.m_Guid;
  ae.m_uiThumbnailID = (ezUInt32)-1;
}
