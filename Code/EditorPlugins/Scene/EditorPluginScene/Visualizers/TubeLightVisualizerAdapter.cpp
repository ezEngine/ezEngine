#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorPluginScene/Visualizers/TubeLightVisualizerAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezTubeLightVisualizerAdapter::ezTubeLightVisualizerAdapter() = default;

ezTubeLightVisualizerAdapter::~ezTubeLightVisualizerAdapter() = default;

void ezTubeLightVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const ezAssetDocument* pAssetDocument = ezDynamicCast<const ezAssetDocument*>(pDoc);
  EZ_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in ezAssetDocument.");

  m_hCapsuleL.ConfigureHandle(nullptr, ezEngineGizmoHandleType::HalfSphereZ, ezColor::White, ezGizmoFlags::Visualizer);
  m_hCapsuleR.ConfigureHandle(nullptr, ezEngineGizmoHandleType::HalfSphereZ, ezColor::White, ezGizmoFlags::Visualizer);
  m_hCapsuleM.ConfigureHandle(nullptr, ezEngineGizmoHandleType::LineCylinderZ, ezColor::White, ezGizmoFlags::Visualizer);
  // Sphere gizmo shows total attenuation range: effective range + half tube length
  m_hRangeGizmo.ConfigureHandle(nullptr, ezEngineGizmoHandleType::Sphere, ezColor::White, ezGizmoFlags::ShowInOrtho | ezGizmoFlags::Visualizer);
  pAssetDocument->AddSyncObject(&m_hCapsuleL);
  pAssetDocument->AddSyncObject(&m_hCapsuleR);
  pAssetDocument->AddSyncObject(&m_hCapsuleM);
  pAssetDocument->AddSyncObject(&m_hRangeGizmo);
  m_hCapsuleL.SetVisible(m_bVisualizerIsVisible);
  m_hCapsuleR.SetVisible(m_bVisualizerIsVisible);
  m_hCapsuleM.SetVisible(m_bVisualizerIsVisible);
  m_hRangeGizmo.SetVisible(m_bVisualizerIsVisible);
}

void ezTubeLightVisualizerAdapter::Update()
{
  m_hCapsuleL.SetVisible(m_bVisualizerIsVisible);
  m_hCapsuleR.SetVisible(m_bVisualizerIsVisible);
  m_hCapsuleM.SetVisible(m_bVisualizerIsVisible);
  m_hRangeGizmo.SetVisible(m_bVisualizerIsVisible);

  ezObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  const ezTubeLightVisualizerAttribute* pAttr = static_cast<const ezTubeLightVisualizerAttribute*>(m_pVisualizerAttr);

  m_fScale = 1.0f;
  m_fLength = 1.0f;
  m_fRadius = 0.05f;

  if (!pAttr->GetRangeProperty().IsEmpty() && !pAttr->GetIntensityProperty().IsEmpty())
  {
    ezVariant range;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetRangeProperty()), range).AssertSuccess();
    EZ_ASSERT_DEBUG(range.CanConvertTo<float>(), "Invalid property bound to ezTubeLightVisualizerAttribute 'range'");

    ezVariant intensity;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetIntensityProperty()), intensity).AssertSuccess();
    EZ_ASSERT_DEBUG(intensity.CanConvertTo<float>(), "Invalid property bound to ezTubeLightVisualizerAttribute 'intensity'");

    m_fScale = ezLightComponent::CalculateEffectiveRange(range.ConvertTo<float>(), intensity.ConvertTo<float>());
  }

  if (!pAttr->GetLengthProperty().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetLengthProperty()), value).AssertSuccess();
    EZ_ASSERT_DEBUG(value.CanConvertTo<float>(), "Invalid property bound to ezTubeLightVisualizerAttribute 'length'");
    m_fLength = value.ConvertTo<float>();
  }

  if (!pAttr->GetRadiusProperty().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetRadiusProperty()), value).AssertSuccess();
    EZ_ASSERT_DEBUG(value.CanConvertTo<float>(), "Invalid property bound to ezTubeLightVisualizerAttribute 'radius'");
    m_fRadius = value.ConvertTo<float>();
  }

  if (!pAttr->GetColorProperty().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetColorProperty()), value).AssertSuccess();
    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<ezColor>(), "Invalid property bound to ezTubeLightVisualizerAdapter 'color'");
    const ezColor color = value.ConvertTo<ezColor>();
    m_hCapsuleL.SetColor(color);
    m_hCapsuleR.SetColor(color);
    m_hCapsuleM.SetColor(color);
    m_hRangeGizmo.SetColor(color);
  }
}

void ezTubeLightVisualizerAdapter::UpdateGizmoTransform()
{
  // The bounding sphere encompasses range + half tube length
  ezTransform t = GetObjectTransform();
  float fBoundingRadius = m_fScale + m_fLength * 0.5f;
  t.m_vScale *= fBoundingRadius;
  m_hRangeGizmo.SetTransformation(t);

  const ezQuat rotToX = ezBasisAxis::GetBasisRotation(ezBasisAxis::PositiveZ, ezBasisAxis::PositiveX);
  const ezQuat rot180 = ezBasisAxis::GetBasisRotation(ezBasisAxis::PositiveZ, ezBasisAxis::NegativeZ);

  ezTransform baseTransform = GetObjectTransform();
  baseTransform.m_qRotation = baseTransform.m_qRotation * rotToX;

  t = baseTransform;
  t.m_vScale.z = m_fLength;
  t.m_vScale.x = m_fRadius;
  t.m_vScale.y = m_fRadius;

  m_hCapsuleM.SetTransformation(t);

  t = baseTransform;
  t.m_vScale.z = m_fRadius;
  t.m_vScale.x = m_fRadius;
  t.m_vScale.y = m_fRadius;

  t.m_vPosition = baseTransform.m_vPosition + baseTransform.m_qRotation * ezVec3(0, 0, m_fLength * 0.5f);
  m_hCapsuleL.SetTransformation(t);

  t.m_vPosition = baseTransform.m_vPosition - baseTransform.m_qRotation * ezVec3(0, 0, m_fLength * 0.5f);
  t.m_qRotation = t.m_qRotation * rot180;
  m_hCapsuleR.SetTransformation(t);
}
