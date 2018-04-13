#include <PCH.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonModel.moc.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>

ezQtSkeletonModel::ezQtSkeletonModel(QWidget* parent, const ezSkeletonAssetDocument* pDocument)
  : QAbstractItemModel(parent)
  , m_pDocument(pDocument)
{
  m_pManager = m_pDocument->GetObjectManager();
}

ezQtSkeletonModel::~ezQtSkeletonModel()
{
}

QModelIndex ezQtSkeletonModel::index(int row, int column, const QModelIndex& parent) const
{
  if (!parent.isValid())
  {
    const ezDocumentObject* pRoot = m_pManager->GetRootObject();
    ezVariant value = pRoot->GetTypeAccessor().GetValue("Children", row);
    EZ_ASSERT_DEV(value.IsValid() && value.IsA<ezUuid>(), "Tree corruption!");
    const ezDocumentObject* pChild = m_pManager->GetObject(value.Get<ezUuid>());
    return createIndex(row, column, const_cast<ezDocumentObject*>(pChild));
  }
  else
  {
    const ezDocumentObject* pParent = (const ezDocumentObject*)parent.internalPointer();
    ezVariant value = pParent->GetTypeAccessor().GetValue("Children", row);
    EZ_ASSERT_DEV(value.IsValid() && value.IsA<ezUuid>(), "Tree corruption!");
    const ezDocumentObject* pChild = m_pManager->GetObject(value.Get<ezUuid>());
    return createIndex(row, column, const_cast<ezDocumentObject*>(pChild));
  }

  return createIndex(row, column, nullptr);
}

QModelIndex ezQtSkeletonModel::parent(const QModelIndex& child) const
{
  const ezDocumentObject* pObject = (const ezDocumentObject*)child.internalPointer();
  const ezDocumentObject* pParent = pObject->GetParent();

  if (pParent == m_pManager->GetRootObject())
    return QModelIndex();

  const ezInt32 iIndex = pParent->GetPropertyIndex().ConvertTo<ezInt32>();

  return createIndex(iIndex, 0, const_cast<ezDocumentObject*>(pParent));
}

int ezQtSkeletonModel::rowCount(const QModelIndex& parent) const
{
  if (parent.isValid())
  {
    const ezDocumentObject* pObject = (const ezDocumentObject*)parent.internalPointer();

    return pObject->GetTypeAccessor().GetCount("Children");
  }

  return 1; // top level object
}

int ezQtSkeletonModel::columnCount(const QModelIndex& parent) const
{
  return 1;
}

QVariant ezQtSkeletonModel::data(const QModelIndex& index, int role) const
{
  if (index.isValid())
  {
    const ezDocumentObject* pObject = (const ezDocumentObject*)index.internalPointer();

    switch (role)
    {
    case Qt::DisplayRole:
      {
        if (index.parent().isValid())
        {
          return QString::fromUtf8(pObject->GetTypeAccessor().GetValue("Name").ConvertTo<ezString>().GetData());
        }
        else
        {
          return QString("Skeleton");
        }
      }
      break;
    }
  }

  return QVariant();
}

Qt::ItemFlags ezQtSkeletonModel::flags(const QModelIndex &index) const
{
  if (index.column() == 0)
  {
    return (Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  }

  return Qt::ItemFlag::NoItemFlags;
}

