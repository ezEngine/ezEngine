#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Visualizers/SphereVisualizerAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezSphereVisualizerAdapter::ezSphereVisualizerAdapter() = default;
ezSphereVisualizerAdapter::~ezSphereVisualizerAdapter() = default;

void ezSphereVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const ezAssetDocument* pAssetDocument = ezDynamicCast<const ezAssetDocument*>(pDoc);
  EZ_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in ezAssetDocument.");

  const ezSphereVisualizerAttribute* pAttr = static_cast<const ezSphereVisualizerAttribute*>(m_pVisualizerAttr);

  m_Gizmo.ConfigureHandle(nullptr, ezEngineGizmoHandleType::Sphere, pAttr->m_Color, ezGizmoFlags::ShowInOrtho | ezGizmoFlags::Visualizer);

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
    m_Gizmo.SetColor(value.ConvertTo<ezColor>() * pAttr->m_Color);
  }

  m_vPositionOffset = pAttr->m_vOffsetOrScale;

  if (!pAttr->GetOffsetProperty().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetOffsetProperty()), value);

    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<ezVec3>(), "Invalid property bound to ezSphereVisualizerAttribute 'offset'");

    if (m_vPositionOffset.IsZero())
      m_vPositionOffset = value.ConvertTo<ezVec3>();
    else
      m_vPositionOffset = m_vPositionOffset.CompMul(value.ConvertTo<ezVec3>());
  }

  m_Anchor = pAttr->m_Anchor;
}

void ezSphereVisualizerAdapter::UpdateGizmoTransform()
{
  ezTransform t;
  t.m_qRotation.SetIdentity();
  t.m_vScale.Set(m_Scale);
  t.m_vPosition = m_vPositionOffset;

  ezVec3 vOffset = ezVec3::ZeroVector();

  if (m_Anchor.IsSet(ezVisualizerAnchor::PosX))
    vOffset.x -= t.m_vScale.x;
  if (m_Anchor.IsSet(ezVisualizerAnchor::NegX))
    vOffset.x += t.m_vScale.x;
  if (m_Anchor.IsSet(ezVisualizerAnchor::PosY))
    vOffset.y -= t.m_vScale.y;
  if (m_Anchor.IsSet(ezVisualizerAnchor::NegY))
    vOffset.y += t.m_vScale.y;
  if (m_Anchor.IsSet(ezVisualizerAnchor::PosZ))
    vOffset.z -= t.m_vScale.z;
  if (m_Anchor.IsSet(ezVisualizerAnchor::NegZ))
    vOffset.z += t.m_vScale.z;

  t.m_vPosition += vOffset;

  m_Gizmo.SetTransformation(GetObjectTransform() * t);
}
