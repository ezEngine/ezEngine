#include <PCH.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Visualizers/ConeVisualizerAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezConeVisualizerAdapter::ezConeVisualizerAdapter() {}

ezConeVisualizerAdapter::~ezConeVisualizerAdapter() {}

void ezConeVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument();
  const ezAssetDocument* pAssetDocument = ezDynamicCast<const ezAssetDocument*>(pDoc);
  EZ_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in ezAssetDocument.");

  const ezConeVisualizerAttribute* pAttr = static_cast<const ezConeVisualizerAttribute*>(m_pVisualizerAttr);

  m_Gizmo.Configure(nullptr, ezEngineGizmoHandleType::Cone, pAttr->m_Color, false, false, true);

  pAssetDocument->AddSyncObject(&m_Gizmo);
  m_Gizmo.SetVisible(m_bVisualizerIsVisible);
}

void ezConeVisualizerAdapter::Update()
{
  const ezConeVisualizerAttribute* pAttr = static_cast<const ezConeVisualizerAttribute*>(m_pVisualizerAttr);
  ezObjectAccessorBase* pObjectAccessor = GetObjectAccessor();

  m_fAngleScale = 1.0f;
  if (!pAttr->GetAngleProperty().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetAngleProperty()), value);

    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<ezAngle>(), "Invalid property bound to ezConeVisualizerAttribute 'angle'");
    m_fAngleScale = ezMath::Tan(value.ConvertTo<ezAngle>() * 0.5f);
  }

  if (!pAttr->GetColorProperty().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetColorProperty()), value);

    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<ezColor>(), "Invalid property bound to ezConeVisualizerAttribute 'color'");
    m_Gizmo.SetColor(value.ConvertTo<ezColor>());
  }

  m_fFinalScale = pAttr->m_fScale;
  if (!pAttr->GetRadiusProperty().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetRadiusProperty()), value);

    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to ezConeVisualizerAttribute 'radius'");
    m_fFinalScale *= value.ConvertTo<float>();
  }

  m_Gizmo.SetVisible(m_bVisualizerIsVisible && m_fAngleScale != 0.0f && m_fFinalScale != 0.0f);
}

void ezConeVisualizerAdapter::UpdateGizmoTransform()
{
  const ezConeVisualizerAttribute* pAttr = static_cast<const ezConeVisualizerAttribute*>(m_pVisualizerAttr);

  ezQuat rot;
  switch (pAttr->m_Axis)
  {
    case ezBasisAxis::PositiveX:
      rot.SetIdentity();
      break;
    case ezBasisAxis::PositiveY:
      rot.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(90));
      break;
    case ezBasisAxis::PositiveZ:
      rot.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(-90));
      break;
    case ezBasisAxis::NegativeX:
      rot.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(180));
      break;
    case ezBasisAxis::NegativeY:
      rot.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(-90));
      break;
    case ezBasisAxis::NegativeZ:
      rot.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(-90));
      break;
  }

  ezTransform t = GetObjectTransform();
  t.m_vScale = t.m_vScale.CompMul(ezVec3(1.0f, m_fAngleScale, m_fAngleScale) * m_fFinalScale);
  t.m_qRotation = rot * t.m_qRotation;
  m_Gizmo.SetTransformation(t);
}
