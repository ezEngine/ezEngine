#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserModel.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

ezQtAssetFilter::ezQtAssetFilter(QObject* pParent)
  : QObject(pParent)
{
}

////////////////////////////////////////////////////////////////////////
// ezQtAssetBrowserModel public functions
////////////////////////////////////////////////////////////////////////

struct FileComparer
{
  FileComparer(ezQtAssetBrowserModel* pModel, const ezHashTable<ezUuid, ezSubAsset>& allAssets)
    : m_pModel(pModel)
    , m_AllAssets(allAssets)
  {
    m_bSortByRecentlyUsed = m_pModel->m_pFilter->GetSortByRecentUse();
  }

  bool Less(const ezQtAssetBrowserModel::VisibleEntry& a, const ezQtAssetBrowserModel::VisibleEntry& b) const
  {
    if (a.m_Flags.IsAnySet(ezAssetBrowserItemFlags::Folder | ezAssetBrowserItemFlags::DataDirectory) != b.m_Flags.IsAnySet(ezAssetBrowserItemFlags::Folder | ezAssetBrowserItemFlags::DataDirectory))
      return a.m_Flags.IsAnySet(ezAssetBrowserItemFlags::Folder | ezAssetBrowserItemFlags::DataDirectory);

    if (a.m_Flags.IsAnySet(ezAssetBrowserItemFlags::Folder | ezAssetBrowserItemFlags::DataDirectory))
    {
      return ezCompareDataDirPath::Less(a.m_sAbsFilePath, b.m_sAbsFilePath);
    }

    const ezSubAsset* pInfoA = nullptr;
    m_AllAssets.TryGetValue(a.m_Guid, pInfoA);

    const ezSubAsset* pInfoB = nullptr;
    m_AllAssets.TryGetValue(b.m_Guid, pInfoB);

    ezStringView sSortA;
    ezStringView sSortB;
    if (pInfoA && !pInfoA->m_bMainAsset)
    {
      sSortA = pInfoA->GetName();
    }
    else
    {
      sSortA = a.m_sAbsFilePath.GetAbsolutePath().GetFileNameAndExtension();
    }
    if (pInfoB && !pInfoB->m_bMainAsset)
    {
      sSortB = pInfoB->GetName();
    }
    else
    {
      sSortB = b.m_sAbsFilePath.GetAbsolutePath().GetFileNameAndExtension();
    }

    if (m_bSortByRecentlyUsed)
    {
      if (pInfoA && pInfoB)
      {
        if (pInfoA->m_LastAccess != pInfoB->m_LastAccess)
        {
          return pInfoA->m_LastAccess > pInfoB->m_LastAccess;
        }
      }
      else if (pInfoA && pInfoA->m_LastAccess.IsPositive())
      {
        return true;
      }
      else if (pInfoB && pInfoB->m_LastAccess.IsPositive())
      {
        return false;
      }

      // in all other cases, fall through and do the file name comparison
    }

    ezInt32 iValue = sSortA.Compare_NoCase(sSortB);
    if (iValue == 0)
    {
      if (!pInfoA && !pInfoB)
        return ezCompareDataDirPath::Less(a.m_sAbsFilePath, b.m_sAbsFilePath);
      else if (pInfoA && pInfoB)
        return pInfoA->m_Data.m_Guid < pInfoB->m_Data.m_Guid;
      else
        return pInfoA == nullptr;
    }
    return iValue < 0;
  }

  EZ_ALWAYS_INLINE bool operator()(const ezQtAssetBrowserModel::VisibleEntry& a, const ezQtAssetBrowserModel::VisibleEntry& b) const
  {
    return Less(a, b);
  }

  ezQtAssetBrowserModel* m_pModel = nullptr;
  const ezHashTable<ezUuid, ezSubAsset>& m_AllAssets;
  bool m_bSortByRecentlyUsed = false;
};

ezQtAssetBrowserModel::ezQtAssetBrowserModel(QObject* pParent, ezQtAssetFilter* pFilter)
  : QAbstractItemModel(pParent)
  , m_pFilter(pFilter)
{
  EZ_ASSERT_DEBUG(pFilter != nullptr, "ezQtAssetBrowserModel requires a valid filter.");
  connect(pFilter, &ezQtAssetFilter::FilterChanged, this, [this]()
    { resetModel(); });

  ezAssetCurator::GetSingleton()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtAssetBrowserModel::AssetCuratorEventHandler, this));

  resetModel();
  SetIconMode(true);

  EZ_VERIFY(connect(ezQtImageCache::GetSingleton(), &ezQtImageCache::ImageLoaded, this, &ezQtAssetBrowserModel::ThumbnailLoaded) != nullptr,
    "signal/slot connection failed");
  EZ_VERIFY(connect(ezQtImageCache::GetSingleton(), &ezQtImageCache::ImageInvalidated, this, &ezQtAssetBrowserModel::ThumbnailInvalidated) != nullptr,
    "signal/slot connection failed");

  ezFileSystemModel::GetSingleton()->m_FileChangedEvents.AddEventHandler(ezMakeDelegate(&ezQtAssetBrowserModel::FileSystemFileEventHandler, this));
  ezFileSystemModel::GetSingleton()->m_FolderChangedEvents.AddEventHandler(ezMakeDelegate(&ezQtAssetBrowserModel::FileSystemFolderEventHandler, this));
}

ezQtAssetBrowserModel::~ezQtAssetBrowserModel()
{
  ezFileSystemModel::GetSingleton()->m_FileChangedEvents.RemoveEventHandler(ezMakeDelegate(&ezQtAssetBrowserModel::FileSystemFileEventHandler, this));
  ezFileSystemModel::GetSingleton()->m_FolderChangedEvents.RemoveEventHandler(ezMakeDelegate(&ezQtAssetBrowserModel::FileSystemFolderEventHandler, this));
  ezAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtAssetBrowserModel::AssetCuratorEventHandler, this));
}

void ezQtAssetBrowserModel::AssetCuratorEventHandler(const ezAssetCuratorEvent& e)
{
  switch (e.m_Type)
  {
    case ezAssetCuratorEvent::Type::AssetUpdated:
    {
      VisibleEntry ve;
      ve.m_Guid = e.m_AssetGuid;
      ve.m_sAbsFilePath = e.m_pInfo->m_pAssetInfo->m_Path;

      HandleEntry(ve, AssetOp::Updated);
      break;
    }
    case ezAssetCuratorEvent::Type::AssetListReset:
    {
      m_ImportExtensions.Clear();
      ezAssetDocumentGenerator::GetSupportsFileTypes(m_ImportExtensions);
      break;
    }
    default:
      break;
  }
}


ezInt32 ezQtAssetBrowserModel::FindAssetIndex(const ezUuid& assetGuid) const
{
  if (!m_DisplayedEntries.Contains(assetGuid))
    return -1;

  for (ezUInt32 i = 0; i < m_EntriesToDisplay.GetCount(); ++i)
  {
    if (m_EntriesToDisplay[i].m_Guid == assetGuid)
    {
      return i;
    }
  }

  return -1;
}


ezInt32 ezQtAssetBrowserModel::FindIndex(ezStringView sAbsPath) const
{
  for (ezUInt32 i = 0; i < m_EntriesToDisplay.GetCount(); ++i)
  {
    if (m_EntriesToDisplay[i].m_sAbsFilePath.GetAbsolutePath() == sAbsPath)
    {
      return i;
    }
  }
  return -1;
}

void ezQtAssetBrowserModel::resetModel()
{
  beginResetModel();

  m_EntriesToDisplay.Clear();
  m_DisplayedEntries.Clear();

  // Get Curator Mutex first to prevent deadlocks
  ezAssetCurator::ezLockedSubAssetTable AllAssetsLocked = ezAssetCurator::GetSingleton()->GetKnownSubAssets();
  const ezHashTable<ezUuid, ezSubAsset>& AllAssets = *(AllAssetsLocked.operator->());

  auto allFiles = ezFileSystemModel::GetSingleton()->GetFiles();
  auto allFolders = ezFileSystemModel::GetSingleton()->GetFolders();

  for (const auto& folder : *allFolders)
  {
    if (m_pFilter->IsAssetFiltered(folder.Key().GetDataDirParentRelativePath(), true, nullptr))
      continue;

    auto& entry = m_EntriesToDisplay.ExpandAndGetRef();
    entry.m_Flags = folder.Key().GetDataDirRelativePath().IsEmpty() ? ezAssetBrowserItemFlags::DataDirectory : ezAssetBrowserItemFlags::Folder;
    entry.m_sAbsFilePath = folder.Key();
  }

  for (const auto& file : *allFiles)
  {
    if (file.Value().m_DocumentID.IsValid())
    {
      auto mainAsset = ezAssetCurator::GetSingleton()->GetSubAsset(file.Value().m_DocumentID);

      if (!mainAsset)
        continue;

      if (!m_pFilter->IsAssetFiltered(file.Key().GetDataDirParentRelativePath(), false, &(*mainAsset)))
      {
        auto& entry = m_EntriesToDisplay.ExpandAndGetRef();
        entry.m_sAbsFilePath = file.Key();
        entry.m_Guid = file.Value().m_DocumentID;
        entry.m_Flags = ezAssetBrowserItemFlags::File | ezAssetBrowserItemFlags::Asset;
        m_DisplayedEntries.Insert(entry.m_Guid);
      }

      for (const auto& subAssetGuid : mainAsset->m_pAssetInfo->m_SubAssets)
      {
        auto subAsset = ezAssetCurator::GetSingleton()->GetSubAsset(subAssetGuid);

        if (subAsset && m_pFilter->IsAssetFiltered(file.Key().GetDataDirParentRelativePath(), false, &(*subAsset)))
          continue;

        auto& entry = m_EntriesToDisplay.ExpandAndGetRef();
        entry.m_sAbsFilePath = file.Key();
        entry.m_Guid = subAssetGuid;
        entry.m_Flags |= ezAssetBrowserItemFlags::SubAsset;
        m_DisplayedEntries.Insert(entry.m_Guid);
      }
    }
    else
    {
      if (m_pFilter->IsAssetFiltered(file.Key().GetDataDirParentRelativePath(), false, nullptr))
        continue;

      auto& entry = m_EntriesToDisplay.ExpandAndGetRef();
      entry.m_sAbsFilePath = file.Key();
      entry.m_Flags = ezAssetBrowserItemFlags::File;
    }
  }


  FileComparer cmp(this, AllAssets);
  m_EntriesToDisplay.Sort(cmp);

  endResetModel();
}

void ezQtAssetBrowserModel::HandleEntry(const VisibleEntry& entry, AssetOp op)
{
  auto subAsset = ezAssetCurator::GetSingleton()->GetSubAsset(entry.m_Guid);

  if (m_pFilter->IsAssetFiltered(entry.m_sAbsFilePath.GetDataDirParentRelativePath(), entry.m_Flags.IsAnySet(ezAssetBrowserItemFlags::Folder | ezAssetBrowserItemFlags::DataDirectory), subAsset.Borrow()))
  {
    if (!m_DisplayedEntries.Contains(entry.m_Guid))
    {
      return;
    }

    // Filtered but still exists, remove it.
    op = AssetOp::Remove;
  }

  ezAssetCurator::ezLockedSubAssetTable AllAssetsLocked = ezAssetCurator::GetSingleton()->GetKnownSubAssets();
  const ezHashTable<ezUuid, ezSubAsset>& AllAssets = *AllAssetsLocked.Borrow();

  FileComparer cmp(this, AllAssets);
  VisibleEntry* pLB = std::lower_bound(begin(m_EntriesToDisplay), end(m_EntriesToDisplay), entry, cmp);
  ezUInt32 uiInsertIndex = pLB - m_EntriesToDisplay.GetData();
  // TODO: Due to sorting issues the above can fail (we need to add a sorting model on top of this as we use mutable data (name) for sorting.
  if (uiInsertIndex >= m_EntriesToDisplay.GetCount())
  {
    for (ezUInt32 i = 0; i < m_EntriesToDisplay.GetCount(); i++)
    {
      VisibleEntry& displayEntry = m_EntriesToDisplay[i];
      if (!cmp.Less(displayEntry, entry) && !cmp.Less(entry, displayEntry))
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
    if (uiInsertIndex < m_EntriesToDisplay.GetCount() && !cmp.Less(*pLB, entry) && !cmp.Less(entry, *pLB))
      return;

    beginInsertRows(QModelIndex(), uiInsertIndex, uiInsertIndex);
    m_EntriesToDisplay.InsertAt(uiInsertIndex, entry);
    if (entry.m_Guid.IsValid())
      m_DisplayedEntries.Insert(entry.m_Guid);
    endInsertRows();
  }
  else if (op == AssetOp::Remove)
  {
    // Equal?
    if (uiInsertIndex < m_EntriesToDisplay.GetCount() && !cmp.Less(*pLB, entry) && !cmp.Less(entry, *pLB))
    {
      beginRemoveRows(QModelIndex(), uiInsertIndex, uiInsertIndex);
      m_EntriesToDisplay.RemoveAtAndCopy(uiInsertIndex);
      if (entry.m_Guid.IsValid())
        m_DisplayedEntries.Remove(entry.m_Guid);
      endRemoveRows();
    }
  }
  else
  {
    // Equal?
    if (uiInsertIndex < m_EntriesToDisplay.GetCount() && !cmp.Less(*pLB, entry) && !cmp.Less(entry, *pLB))
    {
      QModelIndex idx = index(uiInsertIndex, 0);
      Q_EMIT dataChanged(idx, idx);
    }
    else
    {
      ezInt32 oldIndex = FindAssetIndex(entry.m_Guid);
      if (oldIndex != -1)
      {
        // Name has changed, remove old entry
        beginRemoveRows(QModelIndex(), oldIndex, oldIndex);
        m_EntriesToDisplay.RemoveAtAndCopy(oldIndex);
        m_DisplayedEntries.Remove(entry.m_Guid);
        endRemoveRows();
      }
      HandleEntry(entry, AssetOp::Add);
    }
  }
}


////////////////////////////////////////////////////////////////////////
// ezQtAssetBrowserModel QAbstractItemModel functions
////////////////////////////////////////////////////////////////////////

void ezQtAssetBrowserModel::ThumbnailLoaded(QString sPath, QModelIndex index, QVariant userData1, QVariant userData2)
{
  const ezUuid guid(userData1.toULongLong(), userData2.toULongLong());

  for (ezUInt32 i = 0; i < m_EntriesToDisplay.GetCount(); ++i)
  {
    if (m_EntriesToDisplay[i].m_Guid == guid)
    {
      QModelIndex idx = createIndex(i, 0);
      Q_EMIT dataChanged(idx, idx);
      return;
    }
  }
}

void ezQtAssetBrowserModel::ThumbnailInvalidated(QString sPath, ezUInt32 uiImageID)
{
  for (ezUInt32 i = 0; i < m_EntriesToDisplay.GetCount(); ++i)
  {
    if (m_EntriesToDisplay[i].m_uiThumbnailID == uiImageID)
    {
      QModelIndex idx = createIndex(i, 0);
      Q_EMIT dataChanged(idx, idx);
      return;
    }
  }
}

void ezQtAssetBrowserModel::OnFileSystemUpdate()
{
  ezDynamicArray<FsEvent> events;

  {
    EZ_LOCK(m_Mutex);
    events.Swap(m_QueuedFileSystemEvents);
  }

  for (const auto& e : events)
  {
    if (e.m_FileEvent.m_Type == ezFileChangedEvent::Type::ModelReset)
    {
      resetModel();
      return;
    }
  }

  for (const auto& e : events)
  {
    if (e.m_FileEvent.m_Type != ezFileChangedEvent::Type::None)
      HandleFile(e.m_FileEvent);
    else
      HandleFolder(e.m_FolderEvent);
  }
}

QVariant ezQtAssetBrowserModel::data(const QModelIndex& index, int iRole) const
{
  if (!index.isValid() || index.column() != 0)
    return QVariant();

  const ezInt32 iRow = index.row();
  if (iRow < 0 || iRow >= (ezInt32)m_EntriesToDisplay.GetCount())
    return QVariant();

  const VisibleEntry& entry = m_EntriesToDisplay[iRow];

  // Common properties shared among all item types.
  switch (iRole)
  {
    case ezQtAssetBrowserModel::UserRoles::ItemFlags:
      return (int)entry.m_Flags.GetValue();
    case ezQtAssetBrowserModel::UserRoles::Importable:
    {
      if (entry.m_Flags.IsSet(ezAssetBrowserItemFlags::File) && !entry.m_Flags.IsSet(ezAssetBrowserItemFlags::Asset))
      {
        ezStringBuilder sExt = entry.m_sAbsFilePath.GetAbsolutePath().GetFileExtension();
        sExt.ToLower();
        const bool bImportable = m_ImportExtensions.Contains(sExt);
        return bImportable;
      }
      return false;
    }
    case ezQtAssetBrowserModel::UserRoles::RelativePath:
      return ezMakeQString(entry.m_sAbsFilePath.GetDataDirParentRelativePath());
    case ezQtAssetBrowserModel::UserRoles::AbsolutePath:
      return ezMakeQString(entry.m_sAbsFilePath.GetAbsolutePath());
  }

  if (entry.m_Flags.IsAnySet(ezAssetBrowserItemFlags::Folder | ezAssetBrowserItemFlags::DataDirectory))
  {
    switch (iRole)
    {
      case Qt::DisplayRole:
      {
        ezStringView sFilename = entry.m_sAbsFilePath.GetAbsolutePath().GetFileNameAndExtension();

        return ezMakeQString(sFilename);
      }
      break;

      case Qt::EditRole:
      {
        ezStringView sFilename = entry.m_sAbsFilePath.GetAbsolutePath().GetFileNameAndExtension();
        return ezMakeQString(sFilename);
      }

      case Qt::ToolTipRole:
      {
        return ezMakeQString(entry.m_sAbsFilePath.GetAbsolutePath());
      }
      break;

      case ezQtAssetBrowserModel::UserRoles::AssetIcon:
      {
        return ezQtUiServices::GetCachedIconResource(entry.m_Flags.IsSet(ezAssetBrowserItemFlags::DataDirectory) ? ":/EditorFramework/Icons/DataDirectory.svg" : ":/EditorFramework/Icons/Folder.svg");
      }
    }
  }
  else if (!entry.m_Guid.IsValid()) // Normal file
  {
    switch (iRole)
    {
      case Qt::DisplayRole:
      {
        return ezMakeQString(entry.m_sAbsFilePath.GetAbsolutePath().GetFileNameAndExtension());
      }
      break;

      case Qt::EditRole:
      {
        ezStringView sFilename = entry.m_sAbsFilePath.GetAbsolutePath().GetFileName(); // remove the file extension
        return ezMakeQString(sFilename);
      }

      case Qt::ToolTipRole:
      {
        return ezMakeQString(entry.m_sAbsFilePath.GetAbsolutePath());
      }
      break;

      case ezQtAssetBrowserModel::UserRoles::AssetIcon:
      {
        ezStringBuilder sExt = entry.m_sAbsFilePath.GetAbsolutePath().GetFileExtension();
        sExt.ToLower();
        const bool bImportable = m_ImportExtensions.Contains(sExt);
        const bool bIsReferenced = ezAssetCurator::GetSingleton()->IsReferenced(entry.m_sAbsFilePath.GetAbsolutePath());
        if (bImportable)
        {
          return ezQtUiServices::GetCachedIconResource(bIsReferenced ? ":/EditorFramework/Icons/ImportedFile.svg" : ":/EditorFramework/Icons/ImportableFile.svg");
        }
        return ezQtUiServices::GetCachedIconResource(":/GuiFoundation/Icons/Document.svg");
      }

      case Qt::DecorationRole:
      {
        QFileInfo fi(ezMakeQString(entry.m_sAbsFilePath));
        return m_IconProvider.icon(fi);
      }
    }
  }
  else if (entry.m_Guid.IsValid()) // Asset or sub-asset
  {
    const ezUuid AssetGuid = entry.m_Guid;
    const ezAssetCurator::ezLockedSubAsset pSubAsset = ezAssetCurator::GetSingleton()->GetSubAsset(AssetGuid);

    // this can happen when a file was just changed on disk, e.g. got deleted
    if (pSubAsset == nullptr)
      return QVariant();

    switch (iRole)
    {
      case Qt::DisplayRole:
      {
        ezStringBuilder sFilename = pSubAsset->GetName();
        return ezMakeQString(sFilename);
      }
      break;

      case Qt::EditRole:
      {
        if (entry.m_Flags.IsSet(ezAssetBrowserItemFlags::Asset))
        {
          // Don't allow changing extensions of assets
          ezStringView sFilename = entry.m_sAbsFilePath.GetAbsolutePath().GetFileName();
          return ezMakeQString(sFilename);
        }
      }
      break;

      case Qt::ToolTipRole:
      {
        ezStringBuilder sToolTip = pSubAsset->GetName();
        sToolTip.Append("\n", pSubAsset->m_pAssetInfo->m_Path.GetDataDirParentRelativePath());
        sToolTip.Append("\nTransform State: ");
        switch (pSubAsset->m_pAssetInfo->m_TransformState)
        {
          case ezAssetInfo::Unknown:
            sToolTip.Append("Unknown");
            break;
          case ezAssetInfo::UpToDate:
            sToolTip.Append("Up To Date");
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
          case ezAssetInfo::MissingTransformDependency:
            sToolTip.Append("Missing Transform Dependency");
            break;
          case ezAssetInfo::MissingPackageDependency:
            sToolTip.Append("Missing Package Dependency");
            break;
          case ezAssetInfo::MissingThumbnailDependency:
            sToolTip.Append("Missing Thumbnail Dependency");
            break;
          case ezAssetInfo::CircularDependency:
            sToolTip.Append("Circular Dependency");
            break;
          default:
            break;
        }

        return QString::fromUtf8(sToolTip, sToolTip.GetElementCount());
      }
      case Qt::DecorationRole:
      {
        if (m_bIconMode)
        {
          ezString sThumbnailPath = pSubAsset->m_pAssetInfo->GetManager()->GenerateResourceThumbnailPath(pSubAsset->m_pAssetInfo->m_Path, pSubAsset->m_Data.m_sName);

          ezUInt64 uiUserData1, uiUserData2;
          AssetGuid.GetValues(uiUserData1, uiUserData2);

          const QPixmap* pThumbnailPixmap = ezQtImageCache::GetSingleton()->QueryPixmapForType(pSubAsset->m_Data.m_sSubAssetsDocumentTypeName,
            sThumbnailPath, index, QVariant(uiUserData1), QVariant(uiUserData2), &entry.m_uiThumbnailID);

          return *pThumbnailPixmap;
        }
        else
        {
          return ezQtUiServices::GetCachedIconResource(pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_sIcon, ezColorScheme::GetCategoryColor(pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_sAssetCategory, ezColorScheme::CategoryColorUsage::OverlayIcon));
        }
      }
      break;

      case UserRoles::SubAssetGuid:
        return QVariant::fromValue(pSubAsset->m_Data.m_Guid);
      case UserRoles::AssetGuid:
        return QVariant::fromValue(pSubAsset->m_pAssetInfo->m_Info->m_DocumentID);
      case UserRoles::AssetIcon:
        return ezQtUiServices::GetCachedIconResource(pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_sIcon, ezColorScheme::GetCategoryColor(pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_sAssetCategory, ezColorScheme::CategoryColorUsage::OverlayIcon));
      case UserRoles::TransformState:
        return (int)pSubAsset->m_pAssetInfo->m_TransformState;
    }
  }
  else
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
  }
  return QVariant();
}

bool ezQtAssetBrowserModel::setData(const QModelIndex& index, const QVariant& value, int iRole)
{
  if (!index.isValid())
    return false;

  const ezInt32 iRow = index.row();
  if (iRow < 0 || iRow >= (ezInt32)m_EntriesToDisplay.GetCount())
    return false;

  const VisibleEntry& entry = m_EntriesToDisplay[iRow];
  const bool bIsAsset = entry.m_Guid.IsValid();
  if (entry.m_Flags.IsAnySet(ezAssetBrowserItemFlags::Folder | ezAssetBrowserItemFlags::File))
  {
    const ezString& sAbsPath = entry.m_sAbsFilePath.GetAbsolutePath();
    emit editingFinished(ezMakeQString(sAbsPath), value.toString(), bIsAsset);

    return true;
  }

  return false;
}

Qt::ItemFlags ezQtAssetBrowserModel::flags(const QModelIndex& index) const
{
  if (!index.isValid())
    return Qt::ItemFlags();

  const ezInt32 iRow = index.row();
  if (iRow < 0 || iRow >= (ezInt32)m_EntriesToDisplay.GetCount())
    return Qt::ItemFlags();

  const VisibleEntry& entry = m_EntriesToDisplay[iRow];

  Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

  if (entry.m_Flags.IsAnySet(ezAssetBrowserItemFlags::File | ezAssetBrowserItemFlags::Folder | ezAssetBrowserItemFlags::Asset))
  {
    flags |= Qt::ItemIsDragEnabled | Qt::ItemIsEditable;
  }

  if (entry.m_Flags.IsAnySet(ezAssetBrowserItemFlags::SubAsset))
  {
    flags |= Qt::ItemIsDragEnabled;
  }

  return flags;
}

QVariant ezQtAssetBrowserModel::headerData(int iSection, Qt::Orientation orientation, int iRole) const
{
  if (orientation == Qt::Horizontal && iRole == Qt::DisplayRole)
  {
    switch (iSection)
    {
      case 0:
        return QString("Files");
    }
  }
  return QVariant();
}

QModelIndex ezQtAssetBrowserModel::index(int iRow, int iColumn, const QModelIndex& parent) const
{
  if (parent.isValid() || iColumn != 0)
    return QModelIndex();

  return createIndex(iRow, iColumn);
}

QModelIndex ezQtAssetBrowserModel::parent(const QModelIndex& index) const
{
  return QModelIndex();
}

int ezQtAssetBrowserModel::rowCount(const QModelIndex& parent) const
{
  if (parent.isValid())
    return 0;

  return (int)m_EntriesToDisplay.GetCount();
}

int ezQtAssetBrowserModel::columnCount(const QModelIndex& parent) const
{
  return 1;
}

QStringList ezQtAssetBrowserModel::mimeTypes() const
{
  QStringList types;
  types << "application/ezEditor.AssetGuid";
  types << "application/ezEditor.files";
  return types;
}

QMimeData* ezQtAssetBrowserModel::mimeData(const QModelIndexList& indexes) const
{
  QString sGuids;
  QList<QUrl> urls;
  ezHybridArray<QString, 1> guids;
  ezHybridArray<QString, 1> files;

  ezStringBuilder tmp;

  for (ezUInt32 i = 0; i < (ezUInt32)indexes.size(); ++i)
  {
    QString sGuid(ezConversionUtils::ToString(data(indexes[i], UserRoles::SubAssetGuid).value<ezUuid>(), tmp).GetData());
    QString sPath = data(indexes[i], UserRoles::AbsolutePath).toString();
    guids.PushBack(sGuid);
    if (i == 0)
      sGuids += sPath;
    else
      sGuids += "\n" + sPath;

    files.PushBack(sPath);
    urls.push_back(QUrl::fromLocalFile(sPath));
  }

  QByteArray encodedData;
  QDataStream stream(&encodedData, QIODevice::WriteOnly);
  stream << guids;

  QByteArray encodedData2;
  QDataStream stream2(&encodedData2, QIODevice::WriteOnly);
  stream2 << files;

  QMimeData* mimeData = new QMimeData();
  mimeData->setData("application/ezEditor.AssetGuid", encodedData);
  mimeData->setData("application/ezEditor.files", encodedData2);
  mimeData->setText(sGuids);
  mimeData->setUrls(urls);
  return mimeData;
}

Qt::DropActions ezQtAssetBrowserModel::supportedDropActions() const
{
  return Qt::MoveAction | Qt::LinkAction;
}

void ezQtAssetBrowserModel::FileSystemFileEventHandler(const ezFileChangedEvent& e)
{
  bool bFire = false;

  {
    EZ_LOCK(m_Mutex);

    bFire = m_QueuedFileSystemEvents.IsEmpty();

    auto& res = m_QueuedFileSystemEvents.ExpandAndGetRef();
    res.m_FileEvent = e;
  }

  if (bFire)
  {
    QMetaObject::invokeMethod(this, "OnFileSystemUpdate", Qt::ConnectionType::QueuedConnection);
  }
}

void ezQtAssetBrowserModel::FileSystemFolderEventHandler(const ezFolderChangedEvent& e)
{
  bool bFire = false;

  {
    EZ_LOCK(m_Mutex);

    bFire = m_QueuedFileSystemEvents.IsEmpty();

    auto& res = m_QueuedFileSystemEvents.ExpandAndGetRef();
    res.m_FolderEvent = e;
  }

  if (bFire)
  {
    QMetaObject::invokeMethod(this, "OnFileSystemUpdate", Qt::ConnectionType::QueuedConnection);
  }
}

void ezQtAssetBrowserModel::HandleFile(const ezFileChangedEvent& e)
{
  VisibleEntry ve;
  ve.m_Guid = e.m_Status.m_DocumentID;
  ve.m_sAbsFilePath = e.m_Path;
  ve.m_Flags = ezAssetBrowserItemFlags::File;
  if (ve.m_Guid.IsValid())
  {
    ve.m_Flags |= ezAssetBrowserItemFlags::Asset;
  }

  switch (e.m_Type)
  {
    case ezFileChangedEvent::Type::ModelReset:
      resetModel();
      return;

    case ezFileChangedEvent::Type::FileAdded:
      HandleEntry(ve, AssetOp::Add);
      return;
    case ezFileChangedEvent::Type::DocumentLinked:
    {
      ve.m_Guid = ezUuid::MakeInvalid();
      HandleEntry(ve, AssetOp::Remove);
      ve.m_Guid = e.m_Status.m_DocumentID;
      HandleEntry(ve, AssetOp::Add);
      return;
    }
    case ezFileChangedEvent::Type::DocumentUnlinked:
    {
      ve.m_Guid = e.m_Status.m_DocumentID;
      HandleEntry(ve, AssetOp::Remove);
      ve.m_Guid = ezUuid::MakeInvalid();
      HandleEntry(ve, AssetOp::Add);
      return;
    }
    case ezFileChangedEvent::Type::FileRemoved:
      HandleEntry(ve, AssetOp::Remove);
      return;
    default:
      return;
  }
}

void ezQtAssetBrowserModel::HandleFolder(const ezFolderChangedEvent& e)
{
  VisibleEntry ve;
  ve.m_Flags = ezAssetBrowserItemFlags::Folder;
  ve.m_sAbsFilePath = e.m_Path;

  switch (e.m_Type)
  {
    case ezFolderChangedEvent::Type::FolderAdded:
      HandleEntry(ve, AssetOp::Add);
      return;
    case ezFolderChangedEvent::Type::FolderRemoved:
      HandleEntry(ve, AssetOp::Remove);
      return;
    default:
      return;
  }
}
