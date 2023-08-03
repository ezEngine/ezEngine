#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Visualizers/CapsuleVisualizerAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezCapsuleVisualizerAdapter::ezCapsuleVisualizerAdapter() = default;
ezCapsuleVisualizerAdapter::~ezCapsuleVisualizerAdapter() = default;

void ezCapsuleVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const ezAssetDocument* pAssetDocument = ezDynamicCast<const ezAssetDocument*>(pDoc);
  EZ_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in ezAssetDocument.");
  EZ_MSVC_ANALYSIS_ASSUME(pAssetDocument != nullptr);

  const ezCapsuleVisualizerAttribute* pAttr = static_cast<const ezCapsuleVisualizerAttribute*>(m_pVisualizerAttr);

  m_hCylinder.ConfigureHandle(nullptr, ezEngineGizmoHandleType::CylinderZ, pAttr->m_Color, ezGizmoFlags::Visualizer | ezGizmoFlags::ShowInOrtho);
  m_hSphereTop.ConfigureHandle(nullptr, ezEngineGizmoHandleType::HalfSphereZ, pAttr->m_Color, ezGizmoFlags::Visualizer | ezGizmoFlags::ShowInOrtho);
  m_hSphereBottom.ConfigureHandle(nullptr, ezEngineGizmoHandleType::HalfSphereZ, pAttr->m_Color, ezGizmoFlags::Visualizer | ezGizmoFlags::ShowInOrtho);

  pAssetDocument->AddSyncObject(&m_hCylinder);
  pAssetDocument->AddSyncObject(&m_hSphereTop);
  pAssetDocument->AddSyncObject(&m_hSphereBottom);

  m_hCylinder.SetVisible(m_bVisualizerIsVisible);
  m_hSphereTop.SetVisible(m_bVisualizerIsVisible);
  m_hSphereBottom.SetVisible(m_bVisualizerIsVisible);
}

void ezCapsuleVisualizerAdapter::Update()
{
  const ezCapsuleVisualizerAttribute* pAttr = static_cast<const ezCapsuleVisualizerAttribute*>(m_pVisualizerAttr);
  ezObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  m_hCylinder.SetVisible(m_bVisualizerIsVisible);
  m_hSphereTop.SetVisible(m_bVisualizerIsVisible);
  m_hSphereBottom.SetVisible(m_bVisualizerIsVisible);

  m_fRadius = 1.0f;
  m_fHeight = 0.0f;
  m_Anchor = pAttr->m_Anchor;

  if (!pAttr->GetRadiusProperty().IsEmpty())
  {
    auto pProp = GetProperty(pAttr->GetRadiusProperty());
    EZ_ASSERT_DEBUG(pProp != nullptr, "Invalid property '{0}' bound to ezCapsuleVisualizerAttribute 'radius'", pAttr->GetRadiusProperty());

    if (pProp == nullptr)
      return;

    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, pProp, value).AssertSuccess();

    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property '{0}' bound to ezCapsuleVisualizerAttribute 'radius'", pAttr->GetRadiusProperty());
    m_fRadius = value.ConvertTo<float>();
  }

  if (!pAttr->GetHeightProperty().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetHeightProperty()), value).AssertSuccess();

    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to ezCapsuleVisualizerAttribute 'height'");
    m_fHeight = value.ConvertTo<float>();
  }

  if (!pAttr->GetColorProperty().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetColorProperty()), value).AssertSuccess();

    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<ezColor>(), "Invalid property bound to ezCapsuleVisualizerAttribute 'color'");
    m_hSphereTop.SetColor(value.ConvertTo<ezColor>() * pAttr->m_Color);
    m_hSphereBottom.SetColor(value.ConvertTo<ezColor>() * pAttr->m_Color);
    m_hCylinder.SetColor(value.ConvertTo<ezColor>() * pAttr->m_Color);
  }
}

void ezCapsuleVisualizerAdapter::UpdateGizmoTransform()
{
  ezVec3 vOffset = ezVec3::MakeZero();

  if (m_Anchor.IsSet(ezVisualizerAnchor::PosX))
    vOffset.x -= m_fRadius;
  if (m_Anchor.IsSet(ezVisualizerAnchor::NegX))
    vOffset.x += m_fRadius;
  if (m_Anchor.IsSet(ezVisualizerAnchor::PosY))
    vOffset.y -= m_fRadius;
  if (m_Anchor.IsSet(ezVisualizerAnchor::NegY))
    vOffset.y += m_fRadius;
  if (m_Anchor.IsSet(ezVisualizerAnchor::PosZ))
    vOffset.z -= m_fRadius + 0.5f * m_fHeight;
  if (m_Anchor.IsSet(ezVisualizerAnchor::NegZ))
    vOffset.z += m_fRadius + 0.5f * m_fHeight;

  ezTransform tSphereTop;
  tSphereTop.SetIdentity();
  tSphereTop.m_vScale = ezVec3(m_fRadius);
  tSphereTop.m_vPosition.z = m_fHeight * 0.5f;
  tSphereTop.m_vPosition += vOffset;

  ezTransform tSphereBottom;
  tSphereBottom.SetIdentity();
  tSphereBottom.m_vScale = ezVec3(m_fRadius, -m_fRadius, -m_fRadius);
  tSphereBottom.m_vPosition.z = -m_fHeight * 0.5f;
  tSphereBottom.m_vPosition += vOffset;

  ezTransform tCylinder;
  tCylinder.SetIdentity();
  tCylinder.m_vScale = ezVec3(m_fRadius, m_fRadius, m_fHeight);
  tCylinder.m_vPosition += vOffset;

  m_hSphereTop.SetTransformation(GetObjectTransform() * tSphereTop);
  m_hSphereBottom.SetTransformation(GetObjectTransform() * tSphereBottom);
  m_hCylinder.SetTransformation(GetObjectTransform() * tCylinder);
}
