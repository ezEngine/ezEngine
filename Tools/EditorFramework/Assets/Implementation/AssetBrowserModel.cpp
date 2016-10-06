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
#include <QIcon>
#include <GuiFoundation/UIServices/UIServices.moc.h>

////////////////////////////////////////////////////////////////////////
// ezQtAssetBrowserModel public functions
////////////////////////////////////////////////////////////////////////

ezQtAssetBrowserModel::ezQtAssetBrowserModel(QObject* pParent)
  : QAbstractItemModel(pParent)
{
  ezAssetCurator::GetSingleton()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtAssetBrowserModel::AssetCuratorEventHandler, this));

  resetModel();
  SetIconMode(true);
  m_bShowItemsInSubFolders = true;

  EZ_VERIFY(connect(ezQtImageCache::GetSingleton(), SIGNAL(ImageLoaded(QString, QModelIndex, QVariant, QVariant)), this, SLOT(ThumbnailLoaded(QString, QModelIndex, QVariant, QVariant))) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(ezQtImageCache::GetSingleton(), SIGNAL(ImageInvalidated(QString, ezUInt32)), this, SLOT(ThumbnailInvalidated(QString, ezUInt32))) != nullptr, "signal/slot connection failed");
}

ezQtAssetBrowserModel::~ezQtAssetBrowserModel()
{
  ezAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtAssetBrowserModel::AssetCuratorEventHandler, this));
}

void ezQtAssetBrowserModel::AssetCuratorEventHandler(const ezAssetCuratorEvent& e)
{
  switch (e.m_Type)
  {
  case ezAssetCuratorEvent::Type::AssetListReset:
  case ezAssetCuratorEvent::Type::AssetAdded:
  case ezAssetCuratorEvent::Type::AssetRemoved:
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

  ezStringBuilder sTemp, sTemp2;
  AssetEntry ae;

  const ezTime tNow = ezTime::Now();

  for (auto it = AllAssets.GetIterator(); it.IsValid(); ++it)
  {
    if (!m_sPathFilter.IsEmpty())
    {
      // if the string is not found in the path, ignore this asset
      if (!it.Value()->m_sDataDirRelativePath.StartsWith_NoCase(m_sPathFilter))
          continue;

      if (!m_bShowItemsInSubFolders)
      {
        // do we find another path separator after the prefix path?
        // if so, there is a sub-folder, and thus we ignore it
        if (ezStringUtils::FindSubString(it.Value()->m_sDataDirRelativePath.GetData() + m_sPathFilter.GetElementCount() + 1, "/") != nullptr)
          continue;
      }
    }

    if (!m_sTextFilter.IsEmpty())
    {
      // if the string is not found in the path, ignore this asset
      if (it.Value()->m_sDataDirRelativePath.FindSubString_NoCase(m_sTextFilter) == nullptr)
        continue;
    }

    if (!m_sTypeFilter.IsEmpty())
    {
      sTemp.Set(";", it.Value()->m_Info.m_sAssetTypeName, ";");

      if (!m_sTypeFilter.FindSubString(sTemp))
        continue;
    }

    ae.m_Guid = it.Key();

    sTemp2 = it.Value()->m_sDataDirRelativePath;
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
    if (m_AssetsToDisplay[i].m_uiThumbnailID = uiImageID)
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

  EZ_ASSERT_DEV(pAssetInfo != nullptr, "Invalid Pointer! This can happen when an asset has been overwritten by a new file with a new asset GUID.");

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







