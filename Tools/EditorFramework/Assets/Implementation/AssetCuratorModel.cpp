#include <PCH.h>
#include <EditorFramework/Assets/AssetCuratorModel.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <Foundation/Types/Uuid.h>
#include <QPixmap>

////////////////////////////////////////////////////////////////////////
// ezAssetCuratorModel public functions
////////////////////////////////////////////////////////////////////////

ezAssetCuratorModel::ezAssetCuratorModel(QObject* pParent)
  : QAbstractItemModel(pParent)
{
  ezAssetCurator::GetInstance()->m_Events.AddEventHandler(ezMakeDelegate(&ezAssetCuratorModel::AssetCuratorEventHandler, this));

  resetModel();
  SetIconMode(true);
}

ezAssetCuratorModel::~ezAssetCuratorModel()
{
  ezAssetCurator::GetInstance()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezAssetCuratorModel::AssetCuratorEventHandler, this));
}

void ezAssetCuratorModel::AssetCuratorEventHandler(const ezAssetCurator::Event& e)
{
  switch (e.m_Type)
  {
  case ezAssetCurator::Event::Type::AssetListReset:
    resetModel();
    break;
  }
}

void ezAssetCuratorModel::SetTextFilter(const char* szText)
{
  ezStringBuilder sCleanText = szText;
  sCleanText.MakeCleanPath();

  if (m_sTextFilter == sCleanText)
    return;

  m_sTextFilter = sCleanText;

  resetModel();

  emit TextFilterChanged();
}

void ezAssetCuratorModel::SetTypeFilter(const char* szTypes)
{
  if (m_sTypeFilter == szTypes)
    return;

  m_sTypeFilter = szTypes;

  resetModel();

  emit TypeFilterChanged();
}

void ezAssetCuratorModel::resetModel()
{
  beginResetModel();

  const auto& AllAssets = ezAssetCurator::GetInstance()->GetKnownAssets();

  m_AssetsToDisplay.Clear();
  m_AssetsToDisplay.Reserve(AllAssets.GetCount());

  ezStringBuilder sTemp;

  for (auto it = AllAssets.GetIterator(); it.IsValid(); ++it)
  {
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

    m_AssetsToDisplay.PushBack(it.Key());
  }

  endResetModel();
}

////////////////////////////////////////////////////////////////////////
// ezAssetCuratorModel QAbstractItemModel functions
////////////////////////////////////////////////////////////////////////

void ezAssetCuratorModel::ThumbnailLoaded(QString sPath, QModelIndex index, QVariant UserData1, QVariant UserData2)
{
  const ezUuid guid(UserData1.toULongLong(), UserData2.toULongLong());

  for (ezUInt32 i = 0; i < m_AssetsToDisplay.GetCount(); ++i)
  {
    if (m_AssetsToDisplay[i] == guid)
    {
      QModelIndex idx = createIndex(i, 0);
      emit dataChanged(idx, idx);
      return;
    }
  }
}

QVariant ezAssetCuratorModel::data(const QModelIndex& index, int role) const
{
  if (!index.isValid() || index.column() != 0)
    return QVariant();

  const ezInt32 iRow = index.row();
  if (iRow < 0 || iRow >= (ezInt32)m_AssetsToDisplay.GetCount())
    return QVariant();

  const ezUuid AssetGuid = m_AssetsToDisplay[iRow];
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

        QPixmap* pThumbnailPixmap = QtImageCache::QueryPixmap(sThumbnailPath, this, SLOT(ThumbnailLoaded(QString, QModelIndex, QVariant, QVariant)), index, QVariant(uiUserData1), QVariant(uiUserData2));

        return *pThumbnailPixmap;
      }
    }
    break;

  case Qt::UserRole + 1:
    {
      return QString::fromUtf8(pAssetInfo->m_sAbsolutePath);
    }
    break;
  }

  return QVariant();
}

Qt::ItemFlags ezAssetCuratorModel::flags(const QModelIndex& index) const
{
  if (!index.isValid())
    return 0;

  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant ezAssetCuratorModel::headerData(int section, Qt::Orientation orientation, int role) const
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

QModelIndex ezAssetCuratorModel::index(int row, int column, const QModelIndex& parent) const
{
  if (parent.isValid() || column != 0)
    return QModelIndex();

  return createIndex(row, column);
}

QModelIndex ezAssetCuratorModel::parent(const QModelIndex& index) const
{
  return QModelIndex();
}

int ezAssetCuratorModel::rowCount(const QModelIndex& parent) const
{
  if (parent.isValid())
    return 0;

  return (int) m_AssetsToDisplay.GetCount();
}

int ezAssetCuratorModel::columnCount(const QModelIndex& parent) const
{
  return 1;
}

