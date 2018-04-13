#include <PCH.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonModel.moc.h>

ezQtSkeletonModel::ezQtSkeletonModel(QWidget* parent, const ezSkeletonAssetDocument* pDocument)
  : QAbstractItemModel(parent)
  , m_pDocument(pDocument)
{
}

ezQtSkeletonModel::~ezQtSkeletonModel()
{
}

QModelIndex ezQtSkeletonModel::index(int row, int column, const QModelIndex& parent) const
{
  //if (!parent.isValid())
  //{
  //  const ezDocumentObject* pRoot = m_pDocumentTree->GetRootObject();

  //  if (pRoot->GetTypeAccessor().GetCount(m_sRootProperty) == 0)
  //    return QModelIndex();

  //  ezVariant value = pRoot->GetTypeAccessor().GetValue(m_sRootProperty, row);
  //  EZ_ASSERT_DEV(value.IsValid() && value.IsA<ezUuid>(), "Tree corruption!");

  //  const ezDocumentObject* pObject = m_pDocumentTree->GetObject(value.Get<ezUuid>());
  //  return createIndex(row, column, const_cast<ezDocumentObject*>(pObject));
  //}

  //const ezDocumentObject* pParent = (const ezDocumentObject*)parent.internalPointer();
  //ezVariant value = pParent->GetTypeAccessor().GetValue(m_sChildProperty, row);
  //EZ_ASSERT_DEV(value.IsValid() && value.IsA<ezUuid>(), "Tree corruption!");
  //const ezDocumentObject* pChild = m_pDocumentTree->GetObject(value.Get<ezUuid>());

  //return createIndex(row, column, const_cast<ezDocumentObject*>(pChild));

  return createIndex(row, column, nullptr);
  //return QModelIndex();
}
//
//ezInt32 ezQtSkeletonModel::ComputeIndex(const ezDocumentObject* pObject) const
//{
//  ezInt32 iIndex = pObject->GetPropertyIndex().ConvertTo<ezInt32>();
//  return iIndex;
//}
//
//QModelIndex ezQtSkeletonModel::ComputeModelIndex(const ezDocumentObject* pObject) const
//{
//  // Filter out objects that are not under the root property 'm_sRootProperty' or any of
//  // its children under the property 'm_sChildProperty'.
//  if (pObject == m_pDocumentTree->GetRootObject())
//    return QModelIndex();
//
//  if (pObject->GetParent() == m_pDocumentTree->GetRootObject())
//  {
//    if (m_sRootProperty != pObject->GetParentProperty())
//      return QModelIndex();
//  }
//  else
//  {
//    if (m_sChildProperty != pObject->GetParentProperty())
//      return QModelIndex();
//  }
//
//  return index(ComputeIndex(pObject), 0, ComputeParent(pObject));
//}
//
//QModelIndex ezQtSkeletonModel::ComputeParent(const ezDocumentObject* pObject) const
//{
//  const ezDocumentObject* pParent = pObject->GetParent();
//
//  if (pParent == m_pDocumentTree->GetRootObject())
//    return QModelIndex();
//
//  ezInt32 iIndex = ComputeIndex(pParent);
//
//  return createIndex(iIndex, 0, const_cast<ezDocumentObject*>(pParent));
//}

QModelIndex ezQtSkeletonModel::parent(const QModelIndex& child) const
{
  //const ezDocumentObject* pObject = (const ezDocumentObject*)child.internalPointer();
  //return ComputeParent(pObject);
  return QModelIndex();
}

int ezQtSkeletonModel::rowCount(const QModelIndex& parent) const
{
  int iCount = 0;

  if (!parent.isValid())
  {
    //iCount = m_pDocumentTree->GetRootObject()->GetTypeAccessor().GetCount(m_sRootProperty);
    iCount = 1;
  }
  //else
  //{
  //  const ezDocumentObject* pObject = (const ezDocumentObject*)parent.internalPointer();

  //  if (!m_sChildProperty.IsEmpty())
  //    iCount = pObject->GetTypeAccessor().GetCount(m_sChildProperty);
  //}

  return iCount;
}

int ezQtSkeletonModel::columnCount(const QModelIndex& parent) const
{
  return 1;
}

QVariant ezQtSkeletonModel::data(const QModelIndex& index, int role) const
{
  if (index.isValid())
  {
  //  const ezDocumentObject* pObject = (const ezDocumentObject*)index.internalPointer();

    switch (role)
    {
    case Qt::DisplayRole:
      {
        return QString("Tada");
  //      return QString::fromUtf8(pObject->GetTypeAccessor().GetValue("Name").ConvertTo<ezString>().GetData());
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

