#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonModel.moc.h>

ezQtJointAdapter::ezQtJointAdapter(const ezSkeletonAssetDocument* pDocument)
  : ezQtNamedAdapter(pDocument->GetObjectManager(), ezGetStaticRTTI<ezEditableSkeletonJoint>(), "Children", "Name")
  , m_pDocument(pDocument)
{
}

ezQtJointAdapter::~ezQtJointAdapter() = default;

QVariant ezQtJointAdapter::data(const ezDocumentObject* pObject, int iRow, int iColumn, int iRole) const
{
  switch (iRole)
  {
    case Qt::DecorationRole:
    {
      QIcon icon = ezQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorPluginAssets/CurveY.svg"); // Giv ICon Plez!
      return icon;
    }
    break;
  }
  return ezQtNamedAdapter::data(pObject, iRow, iColumn, iRole);
}
