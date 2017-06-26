#include <PCH.h>
#include <EditorFramework/Visualizers/SphereVisualizerAdapter.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezSphereVisualizerAdapter::ezSphereVisualizerAdapter()
{
}

ezSphereVisualizerAdapter::~ezSphereVisualizerAdapter()
{
}

void ezSphereVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument();
  const ezAssetDocument* pAssetDocument = ezDynamicCast<const ezAssetDocument*>(pDoc);
  EZ_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in ezAssetDocument.");

  const ezSphereVisualizerAttribute* pAttr = static_cast<const ezSphereVisualizerAttribute*>(m_pVisualizerAttr);

  m_Gizmo.Configure(nullptr, ezEngineGizmoHandleType::Sphere, pAttr->m_Color, false, false, true);

  pAssetDocument->AddSyncObject(&m_Gizmo);
  m_Gizmo.SetVisible(m_bVisualizerIsVisible);
}

void ezSphereVisualizerAdapter::Update()
{
  m_Gizmo.SetVisible(m_bVisualizerIsVisible);
  ezObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  const ezSphereVisualizerAttribute* pAttr = static_cast<const ezSphereVisualizerAttribute*>(m_pVisualizerAttr);

  m_Scale = 1.0f;

  if (!pAttr->GetRadiusProperty().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetRadiusProperty()), value);

    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to ezSphereVisualizerAttribute 'radius'");
    m_Scale = value.ConvertTo<float>();
  }

  if (!pAttr->GetColorProperty().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetColorProperty()), value);

    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<ezColor>(), "Invalid property bound to ezSphereVisualizerAttribute 'color'");
    m_Gizmo.SetColor(value.ConvertTo<ezColor>());
  }
}

void ezSphereVisualizerAdapter::UpdateGizmoTransform()
{
  ezTransform t = GetObjectTransform();
  t.m_vScale = t.m_vScale * m_Scale;

  m_Gizmo.SetTransformation(t);
}


