#include <PCH.h>

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorPluginScene/Visualizers/PointLightVisualizerAdapter.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezPointLightVisualizerAdapter::ezPointLightVisualizerAdapter() {}

ezPointLightVisualizerAdapter::~ezPointLightVisualizerAdapter() {}

void ezPointLightVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument();
  const ezAssetDocument* pAssetDocument = ezDynamicCast<const ezAssetDocument*>(pDoc);
  EZ_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in ezAssetDocument.");

  m_Gizmo.Configure(nullptr, ezEngineGizmoHandleType::Sphere, ezColor::White, false, false, true);

  pAssetDocument->AddSyncObject(&m_Gizmo);
  m_Gizmo.SetVisible(m_bVisualizerIsVisible);
}

void ezPointLightVisualizerAdapter::Update()
{
  m_Gizmo.SetVisible(m_bVisualizerIsVisible);
  ezObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  const ezPointLightVisualizerAttribute* pAttr = static_cast<const ezPointLightVisualizerAttribute*>(m_pVisualizerAttr);

  m_fScale = 1.0f;

  if (!pAttr->GetRangeProperty().IsEmpty() && !pAttr->GetIntensityProperty().IsEmpty())
  {
    ezVariant range;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetRangeProperty()), range);
    EZ_ASSERT_DEBUG(range.CanConvertTo<float>(), "Invalid property bound to ezPointLightVisualizerAttribute 'radius'");

    ezVariant intensity;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetIntensityProperty()), intensity);
    EZ_ASSERT_DEBUG(intensity.CanConvertTo<float>(), "Invalid property bound to ezPointLightVisualizerAttribute 'intensity'");

    m_fScale = ezLightComponent::CalculateEffectiveRange(range.ConvertTo<float>(), intensity.ConvertTo<float>());
  }

  if (!pAttr->GetColorProperty().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetColorProperty()), value);

    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<ezColor>(), "Invalid property bound to ezSphereVisualizerAttribute 'color'");
    m_Gizmo.SetColor(value.ConvertTo<ezColor>());
  }
}

void ezPointLightVisualizerAdapter::UpdateGizmoTransform()
{
  ezTransform t = GetObjectTransform();
  t.m_vScale *= m_fScale;

  m_Gizmo.SetTransformation(t);
}
