#include <PCH.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonModel.moc.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QIcon>


ezQtBoneAdapter::ezQtBoneAdapter(const ezSkeletonAssetDocument* pDocument)
  : ezQtNamedAdapter(pDocument->GetObjectManager(), ezGetStaticRTTI<ezEditableSkeletonBone>(), "Children", "Name")
  , m_pDocument(pDocument)
{
}

ezQtBoneAdapter::~ezQtBoneAdapter()
{
}

QVariant ezQtBoneAdapter::data(const ezDocumentObject* pObject, int column, int role) const
{
  switch (role)
  {
  case Qt::DecorationRole:
    {
      QIcon icon = ezQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorPluginAssets/CurveY.png"); // Giv ICon Plez!
      return icon;
    }
    break;
  }
  return ezQtNamedAdapter::data(pObject, column, role);
}
