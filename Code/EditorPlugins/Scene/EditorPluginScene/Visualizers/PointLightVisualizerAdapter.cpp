#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorPluginScene/Visualizers/PointLightVisualizerAdapter.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezPointLightVisualizerAdapter::ezPointLightVisualizerAdapter() = default;

ezPointLightVisualizerAdapter::~ezPointLightVisualizerAdapter() = default;

void ezPointLightVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const ezAssetDocument* pAssetDocument = ezDynamicCast<const ezAssetDocument*>(pDoc);
  EZ_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in ezAssetDocument.");

  // Sphere gizmo shows the effective attenuation range. For tube lights it encompasses the full capsule.
  m_hRangeGizmo.ConfigureHandle(nullptr, ezEngineGizmoHandleType::Sphere, ezColor::White, ezGizmoFlags::ShowInOrtho | ezGizmoFlags::Visualizer);
  m_hCapsuleL.ConfigureHandle(nullptr, ezEngineGizmoHandleType::HalfSphereZ, ezColor::White, ezGizmoFlags::Visualizer);
  m_hCapsuleR.ConfigureHandle(nullptr, ezEngineGizmoHandleType::HalfSphereZ, ezColor::White, ezGizmoFlags::Visualizer);
  m_hCapsuleM.ConfigureHandle(nullptr, ezEngineGizmoHandleType::LineCylinderZ, ezColor::White, ezGizmoFlags::Visualizer);

  pAssetDocument->AddSyncObject(&m_hRangeGizmo);
  pAssetDocument->AddSyncObject(&m_hCapsuleL);
  pAssetDocument->AddSyncObject(&m_hCapsuleR);
  pAssetDocument->AddSyncObject(&m_hCapsuleM);

  m_hRangeGizmo.SetVisible(m_bVisualizerIsVisible);
  m_hCapsuleL.SetVisible(false);
  m_hCapsuleR.SetVisible(false);
  m_hCapsuleM.SetVisible(false);
}

void ezPointLightVisualizerAdapter::Update()
{
  ezObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  const ezPointLightVisualizerAttribute* pAttr = static_cast<const ezPointLightVisualizerAttribute*>(m_pVisualizerAttr);

  m_fEffectiveRange = 1.0f;
  m_fLength = 0.0f;
  m_fRadius = 0.0f;

  if (!pAttr->GetRangeProperty().IsEmpty() && !pAttr->GetIntensityProperty().IsEmpty())
  {
    ezVariant range;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetRangeProperty()), range).AssertSuccess();
    EZ_ASSERT_DEBUG(range.CanConvertTo<float>(), "Invalid property bound to ezPointLightVisualizerAttribute 'range'");

    ezVariant intensity;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetIntensityProperty()), intensity).AssertSuccess();
    EZ_ASSERT_DEBUG(intensity.CanConvertTo<float>(), "Invalid property bound to ezPointLightVisualizerAttribute 'intensity'");

    m_fEffectiveRange = ezLightComponent::CalculateEffectiveRange(range.ConvertTo<float>(), intensity.ConvertTo<float>());
  }

  if (!pAttr->GetLengthProperty().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetLengthProperty()), value).AssertSuccess();
    EZ_ASSERT_DEBUG(value.CanConvertTo<float>(), "Invalid property bound to ezPointLightVisualizerAttribute 'length'");
    m_fLength = value.ConvertTo<float>();
  }

  if (!pAttr->GetRadiusProperty().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetRadiusProperty()), value).AssertSuccess();
    EZ_ASSERT_DEBUG(value.CanConvertTo<float>(), "Invalid property bound to ezPointLightVisualizerAttribute 'radius'");
    m_fRadius = value.ConvertTo<float>();
  }

  ezColor color = ezColor::White;
  if (!pAttr->GetColorProperty().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetColorProperty()), value).AssertSuccess();
    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<ezColor>(), "Invalid property bound to ezPointLightVisualizerAdapter 'color'");
    color = value.ConvertTo<ezColor>();
  }

  m_bIsTube = (m_fLength > 0.0f || m_fRadius > 0.0f);

  m_hRangeGizmo.SetColor(color);
  m_hCapsuleL.SetColor(color);
  m_hCapsuleR.SetColor(color);
  m_hCapsuleM.SetColor(color);

  m_hRangeGizmo.SetVisible(m_bVisualizerIsVisible);
  m_hCapsuleL.SetVisible(m_bVisualizerIsVisible && m_bIsTube);
  m_hCapsuleR.SetVisible(m_bVisualizerIsVisible && m_bIsTube);
  m_hCapsuleM.SetVisible(m_bVisualizerIsVisible && m_bIsTube);
}

void ezPointLightVisualizerAdapter::UpdateGizmoTransform()
{
  // Range sphere encompasses the full attenuation volume (includes half the tube length when tubed).
  ezTransform t = GetObjectTransform();
  const float fBoundingRadius = m_fEffectiveRange + m_fLength * 0.5f;
  t.m_vScale *= fBoundingRadius;
  m_hRangeGizmo.SetTransformation(t);

  if (!m_bIsTube)
    return;

  const ezQuat rotToX = ezBasisAxis::GetBasisRotation(ezBasisAxis::PositiveZ, ezBasisAxis::PositiveX);
  const ezQuat rot180 = ezBasisAxis::GetBasisRotation(ezBasisAxis::PositiveZ, ezBasisAxis::NegativeZ);

  ezTransform baseTransform = GetObjectTransform();
  baseTransform.m_qRotation = baseTransform.m_qRotation * rotToX;

  // Clamp to tiny non-zero scales so the gizmo primitives remain renderable when only one of length/radius is set.
  const float fLength = ezMath::Max(m_fLength, 0.001f);
  const float fRadius = ezMath::Max(m_fRadius, 0.001f);

  ezTransform tMid = baseTransform;
  tMid.m_vScale.x = fRadius;
  tMid.m_vScale.y = fRadius;
  tMid.m_vScale.z = fLength;
  m_hCapsuleM.SetTransformation(tMid);

  ezTransform tCap = baseTransform;
  tCap.m_vScale.Set(fRadius);

  tCap.m_vPosition = baseTransform.m_vPosition + baseTransform.m_qRotation * ezVec3(0, 0, fLength * 0.5f);
  m_hCapsuleL.SetTransformation(tCap);

  tCap.m_vPosition = baseTransform.m_vPosition - baseTransform.m_qRotation * ezVec3(0, 0, fLength * 0.5f);
  tCap.m_qRotation = tCap.m_qRotation * rot180;
  m_hCapsuleR.SetTransformation(tCap);
}
