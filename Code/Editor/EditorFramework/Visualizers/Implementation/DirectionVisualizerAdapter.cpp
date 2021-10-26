#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Visualizers/DirectionVisualizerAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezDirectionVisualizerAdapter::ezDirectionVisualizerAdapter() {}

ezDirectionVisualizerAdapter::~ezDirectionVisualizerAdapter() {}

void ezDirectionVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const ezAssetDocument* pAssetDocument = ezDynamicCast<const ezAssetDocument*>(pDoc);
  EZ_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in ezAssetDocument.");

  const ezDirectionVisualizerAttribute* pAttr = static_cast<const ezDirectionVisualizerAttribute*>(m_pVisualizerAttr);

  m_Gizmo.Configure(nullptr, ezEngineGizmoHandleType::Arrow, pAttr->m_Color, false, false, true);

  pAssetDocument->AddSyncObject(&m_Gizmo);
  m_Gizmo.SetVisible(m_bVisualizerIsVisible);
}

void ezDirectionVisualizerAdapter::Update()
{
  m_Gizmo.SetVisible(m_bVisualizerIsVisible);
  const ezDirectionVisualizerAttribute* pAttr = static_cast<const ezDirectionVisualizerAttribute*>(m_pVisualizerAttr);

  if (!pAttr->GetColorProperty().IsEmpty())
  {
    ezObjectAccessorBase* pObjectAccessor = GetObjectAccessor();

    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetColorProperty()), value);

    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<ezColor>(), "Invalid property bound to ezDirectionVisualizerAttribute 'color'");
    m_Gizmo.SetColor(value.ConvertTo<ezColor>() * pAttr->m_Color);
  }
}

void ezDirectionVisualizerAdapter::UpdateGizmoTransform()
{
  const ezDirectionVisualizerAttribute* pAttr = static_cast<const ezDirectionVisualizerAttribute*>(m_pVisualizerAttr);
  float fScale = pAttr->m_fScale;

  if (!pAttr->GetLengthProperty().IsEmpty())
  {
    ezObjectAccessorBase* pObjectAccessor = GetObjectAccessor();

    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetLengthProperty()), value);

    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to ezDirectionVisualizerAttribute 'length'");
    fScale *= value.ConvertTo<float>();
  }

  ezBasisAxis::Enum axis = pAttr->m_Axis;

  if (!pAttr->GetAxisProperty().IsEmpty())
  {
    ezObjectAccessorBase* pObjectAccessor = GetObjectAccessor();

    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetAxisProperty()), value);

    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<ezInt32>(), "Invalid property bound to ezDirectionVisualizerAttribute 'length'");

    axis = static_cast<ezBasisAxis::Enum>(value.ConvertTo<ezInt32>());
  }

  const ezQuat axisRotation = ezBasisAxis::GetBasisRotation_PosX(axis);

  ezTransform t;
  t.m_qRotation = axisRotation;
  t.m_vScale = ezVec3(fScale);
  t.m_vPosition = axisRotation * ezVec3(fScale * 0.5f, 0, 0);

  ezTransform tObject = GetObjectTransform();
  tObject.m_vScale.Set(1.0f);

  m_Gizmo.SetTransformation(tObject * t);
}
