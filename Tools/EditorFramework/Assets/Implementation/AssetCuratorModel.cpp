#include <PCH.h>
#include <EditorFramework/Assets/AssetCuratorModel.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <QPixmapCache>

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

void ezAssetCuratorModel::resetModel()
{
  beginResetModel();

  const auto& AllAssets = ezAssetCurator::GetInstance()->GetKnownAssets();

  m_AssetsToDisplay.Clear();
  m_AssetsToDisplay.Reserve(AllAssets.GetCount());

  for (auto it = AllAssets.GetIterator(); it.IsValid(); ++it)
  {
    m_AssetsToDisplay.PushBack(it.Key());
  }

  endResetModel();
}

////////////////////////////////////////////////////////////////////////
// ezAssetCuratorModel QAbstractItemModel functions
////////////////////////////////////////////////////////////////////////

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
      ezStringBuilder sFilename = ezPathUtils::GetFileNameAndExtension(pAssetInfo->m_sPath);
      return QString::fromUtf8(sFilename);
    }
    break;

  case Qt::DecorationRole:
    {
      if (m_bIconMode)
      {
        ezStringBuilder sThumbnailPath = ezAssetDocumentManager::GenerateResourceThumbnailPath(pAssetInfo->m_sPath);

        QString sQtThumbnailPath = QString::fromUtf8(sThumbnailPath.GetData());
        
        QPixmap* pThumbnailPixmap = QPixmapCache::find(sQtThumbnailPath);

        if (pThumbnailPixmap == nullptr)
        {
          QImage img(sQtThumbnailPath);
          QPixmap PixmapThumbnail = QPixmap::fromImage(img);
          QPixmapCache::insert(sQtThumbnailPath, PixmapThumbnail);

          pThumbnailPixmap = QPixmapCache::find(sQtThumbnailPath);
        }

        EZ_ASSERT_DEV(pThumbnailPixmap != nullptr, "Pixmap should now be valid");

        return *pThumbnailPixmap;
      }
    }
    break;

  case Qt::UserRole + 1:
    {
      return QString::fromUtf8(pAssetInfo->m_sPath);
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

