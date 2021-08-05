#include <EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Visualizers/CylinderVisualizerAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezCylinderVisualizerAdapter::ezCylinderVisualizerAdapter() {}

ezCylinderVisualizerAdapter::~ezCylinderVisualizerAdapter() {}

void ezCylinderVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument();
  const ezAssetDocument* pAssetDocument = ezDynamicCast<const ezAssetDocument*>(pDoc);
  EZ_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in ezAssetDocument.");

  const ezCylinderVisualizerAttribute* pAttr = static_cast<const ezCylinderVisualizerAttribute*>(m_pVisualizerAttr);

  m_Cylinder.Configure(nullptr, ezEngineGizmoHandleType::CylinderZ, pAttr->m_Color, false, false, true);

  pAssetDocument->AddSyncObject(&m_Cylinder);

  m_Cylinder.SetVisible(m_bVisualizerIsVisible);
}

void ezCylinderVisualizerAdapter::Update()
{
  const ezCylinderVisualizerAttribute* pAttr = static_cast<const ezCylinderVisualizerAttribute*>(m_pVisualizerAttr);
  ezObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  m_Cylinder.SetVisible(m_bVisualizerIsVisible);

  m_fRadius = 1.0f;
  m_fHeight = 0.0f;

  if (!pAttr->GetRadiusProperty().IsEmpty())
  {
    auto pProp = GetProperty(pAttr->GetRadiusProperty());
    EZ_ASSERT_DEBUG(pProp != nullptr, "Invalid property '{0}' bound to ezCylinderVisualizerAttribute 'radius'", pAttr->GetRadiusProperty());

    if (pProp == nullptr)
      return;

    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, pProp, value);

    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property '{0}' bound to ezCylinderVisualizerAttribute 'radius'",
      pAttr->GetRadiusProperty());
    m_fRadius = value.ConvertTo<float>();
  }

  if (!pAttr->GetHeightProperty().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetHeightProperty()), value);

    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to ezCylinderVisualizerAttribute 'height'");
    m_fHeight = value.ConvertTo<float>();
  }

  if (!pAttr->GetColorProperty().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetColorProperty()), value);

    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<ezColor>(), "Invalid property bound to ezCylinderVisualizerAttribute 'color'");
    m_Cylinder.SetColor(value.ConvertTo<ezColor>() * pAttr->m_Color);
  }

  m_vPositionOffset = pAttr->m_vOffsetOrScale;

  if (!pAttr->GetOffsetProperty().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetOffsetProperty()), value);

    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<ezVec3>(), "Invalid property bound to ezCylinderVisualizerAttribute 'offset'");

    if (m_vPositionOffset.IsZero())
      m_vPositionOffset = value.ConvertTo<ezVec3>();
    else
      m_vPositionOffset = m_vPositionOffset.CompMul(value.ConvertTo<ezVec3>());
  }

  m_Anchor = pAttr->m_Anchor;
}

void ezCylinderVisualizerAdapter::UpdateGizmoTransform()
{
  const ezCylinderVisualizerAttribute* pAttr = static_cast<const ezCylinderVisualizerAttribute*>(m_pVisualizerAttr);
  const ezQuat axisRotation = ezBasisAxis::GetBasisRotation(ezBasisAxis::PositiveZ, pAttr->m_Axis);

  ezTransform t;
  t.m_qRotation = axisRotation;
  t.m_vScale = ezVec3(m_fRadius, m_fRadius, m_fHeight);
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
    vOffset.z -= t.m_vScale.z * 0.5f;
  if (m_Anchor.IsSet(ezVisualizerAnchor::NegZ))
    vOffset.z += t.m_vScale.z * 0.5f;

  t.m_vPosition += vOffset;

  // ezTransform doesn't (can't) combine rotations with scales
  // however, here we know that the axisRotation is just an axis remapping, so we can combine them
  ezTransform parentTransform = GetObjectTransform();
  ezTransform newTrans = parentTransform * t;
  newTrans.m_vScale = (axisRotation * parentTransform.m_vScale).CompMul(t.m_vScale);

  m_Cylinder.SetTransformation(newTrans);
}
