#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Visualizers/BoxVisualizerAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezBoxVisualizerAdapter::ezBoxVisualizerAdapter() = default;
ezBoxVisualizerAdapter::~ezBoxVisualizerAdapter() = default;

void ezBoxVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const ezAssetDocument* pAssetDocument = ezDynamicCast<const ezAssetDocument*>(pDoc);
  EZ_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in ezAssetDocument.");

  const ezBoxVisualizerAttribute* pAttr = static_cast<const ezBoxVisualizerAttribute*>(m_pVisualizerAttr);

  m_hGizmo.ConfigureHandle(nullptr, ezEngineGizmoHandleType::LineBox, pAttr->m_Color, ezGizmoFlags::Visualizer | ezGizmoFlags::ShowInOrtho);

  pAssetDocument->AddSyncObject(&m_hGizmo);
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);
}

void ezBoxVisualizerAdapter::Update()
{
  const ezBoxVisualizerAttribute* pAttr = static_cast<const ezBoxVisualizerAttribute*>(m_pVisualizerAttr);
  ezObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);

  m_vScale.Set(pAttr->m_fSizeScale);

  if (!pAttr->GetSizeProperty().IsEmpty())
  {
    m_vScale *= pObjectAccessor->Get<ezVec3>(m_pObject, GetProperty(pAttr->GetSizeProperty()));
  }

  if (!pAttr->GetColorProperty().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetColorProperty()), value).AssertSuccess();
    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<ezColor>(), "Invalid property bound to ezBoxVisualizerAttribute 'color'");
    m_hGizmo.SetColor(value.ConvertTo<ezColor>() * pAttr->m_Color);
  }

  m_vPositionOffset = pAttr->m_vOffsetOrScale;

  if (!pAttr->GetOffsetProperty().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetOffsetProperty()), value).AssertSuccess();

    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<ezVec3>(), "Invalid property bound to ezBoxVisualizerAttribute 'offset'");

    if (m_vPositionOffset.IsZero())
      m_vPositionOffset = value.ConvertTo<ezVec3>();
    else
      m_vPositionOffset = m_vPositionOffset.CompMul(value.ConvertTo<ezVec3>());
  }

  m_qRotation.SetIdentity();

  if (!pAttr->GetRotationProperty().IsEmpty())
  {
    m_qRotation = pObjectAccessor->Get<ezQuat>(m_pObject, GetProperty(pAttr->GetRotationProperty()));
  }

  m_Anchor = pAttr->m_Anchor;
}

void ezBoxVisualizerAdapter::UpdateGizmoTransform()
{
  ezTransform t;
  t.m_vScale = m_vScale;
  t.m_vPosition = m_vPositionOffset;
  t.m_qRotation = m_qRotation;

  ezVec3 vOffset = ezVec3::ZeroVector();

  if (m_Anchor.IsSet(ezVisualizerAnchor::PosX))
    vOffset.x -= t.m_vScale.x * 0.5f;
  if (m_Anchor.IsSet(ezVisualizerAnchor::NegX))
    vOffset.x += t.m_vScale.x * 0.5f;
  if (m_Anchor.IsSet(ezVisualizerAnchor::PosY))
    vOffset.y -= t.m_vScale.y * 0.5f;
  if (m_Anchor.IsSet(ezVisualizerAnchor::NegY))
    vOffset.y += t.m_vScale.y * 0.5f;
  if (m_Anchor.IsSet(ezVisualizerAnchor::PosZ))
    vOffset.z -= t.m_vScale.z * 0.5f;
  if (m_Anchor.IsSet(ezVisualizerAnchor::NegZ))
    vOffset.z += t.m_vScale.z * 0.5f;

  t.m_vPosition += vOffset;

  m_hGizmo.SetTransformation(GetObjectTransform() * t);
}
