#include <PCH.h>

#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonModel.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QIcon>

ezQtJointAdapter::ezQtJointAdapter(const ezSkeletonAssetDocument* pDocument)
    : ezQtNamedAdapter(pDocument->GetObjectManager(), ezGetStaticRTTI<ezEditableSkeletonJoint>(), "Children", "Name")
    , m_pDocument(pDocument)
{
}

ezQtJointAdapter::~ezQtJointAdapter() {}

QVariant ezQtJointAdapter::data(const ezDocumentObject* pObject, int row, int column, int role) const
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
  return ezQtNamedAdapter::data(pObject, row, column, role);
}
