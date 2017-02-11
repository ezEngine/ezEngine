#include <PCH.h>
#include <EditorFramework/Visualizers/BoxVisualizerAdapter.h>
#include <EditorFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezBoxVisualizerAdapter::ezBoxVisualizerAdapter()
{
}

ezBoxVisualizerAdapter::~ezBoxVisualizerAdapter()
{
}

void ezBoxVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument();
  const ezAssetDocument* pAssetDocument = ezDynamicCast<const ezAssetDocument*>(pDoc);
  EZ_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in ezAssetDocument.");

  const ezBoxVisualizerAttribute* pAttr = static_cast<const ezBoxVisualizerAttribute*>(m_pVisualizerAttr);

  m_Gizmo.Configure(nullptr, ezEngineGizmoHandleType::LineBox, pAttr->m_Color, false, false, true);

  m_Gizmo.SetOwner(pAssetDocument);
  m_Gizmo.SetVisible(m_bVisualizerIsVisible);
}

void ezBoxVisualizerAdapter::Update()
{
  const ezBoxVisualizerAttribute* pAttr = static_cast<const ezBoxVisualizerAttribute*>(m_pVisualizerAttr);
  ezObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  m_Gizmo.SetVisible(m_bVisualizerIsVisible);

  m_Scale.SetIdentity();

  if (!pAttr->GetSizeProperty().IsEmpty())
  {

    ezVec3 vValue = pObjectAccessor->Get<ezVec3>(m_pObject, GetProperty(pAttr->GetSizeProperty()));
    m_Scale.SetScalingMatrix(vValue);
  }

  if (!pAttr->GetColorProperty().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetColorProperty()), value);
    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<ezColor>(), "Invalid property bound to ezBoxVisualizerAttribute 'color'");
    m_Gizmo.SetColor(value.ConvertTo<ezColor>());
  }
}

void ezBoxVisualizerAdapter::UpdateGizmoTransform()
{
  m_Gizmo.SetTransformation(GetObjectTransform().GetAsMat4() * m_Scale);
}


