#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorPluginScene/Visualizers/BoxReflectionProbeVisualizerAdapter.h>
#include <RendererCore/Lights/BoxReflectionProbeComponent.h>

#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezBoxReflectionProbeVisualizerAdapter::ezBoxReflectionProbeVisualizerAdapter() {}
ezBoxReflectionProbeVisualizerAdapter::~ezBoxReflectionProbeVisualizerAdapter() {}

void ezBoxReflectionProbeVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const ezAssetDocument* pAssetDocument = ezDynamicCast<const ezAssetDocument*>(pDoc);
  EZ_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in ezAssetDocument.");

  m_Gizmo.ConfigureHandle(nullptr, ezEngineGizmoHandleType::LineBox, ezColor::Yellow, ezGizmoFlags::ShowInOrtho | ezGizmoFlags::Visualizer);

  pAssetDocument->AddSyncObject(&m_Gizmo);
  m_Gizmo.SetVisible(m_bVisualizerIsVisible);
}

void ezBoxReflectionProbeVisualizerAdapter::Update()
{
  const ezBoxReflectionProbeVisualizerAttribute* pAttr = static_cast<const ezBoxReflectionProbeVisualizerAttribute*>(m_pVisualizerAttr);
  ezObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  m_Gizmo.SetVisible(m_bVisualizerIsVisible);

  m_Scale.Set(1.0f);
  ezVec3 influenceScale;
  ezVec3 influenceShift;

  if (!pAttr->GetExtentsProperty().IsEmpty())
  {
    m_Scale = pObjectAccessor->Get<ezVec3>(m_pObject, GetProperty(pAttr->GetExtentsProperty()));
  }

  if (!pAttr->GetInfluenceScaleProperty().IsEmpty())
  {
    influenceScale = pObjectAccessor->Get<ezVec3>(m_pObject, GetProperty(pAttr->GetInfluenceScaleProperty()));
  }

  if (!pAttr->GetInfluenceShiftProperty().IsEmpty())
  {
    influenceShift = pObjectAccessor->Get<ezVec3>(m_pObject, GetProperty(pAttr->GetInfluenceShiftProperty()));
  }

  m_vPositionOffset = m_Scale.CompMul(influenceShift.CompMul(ezVec3(1.0f) - influenceScale)) * 0.5f;
  m_Scale *= influenceScale;

  m_Rotation.SetIdentity();
}

void ezBoxReflectionProbeVisualizerAdapter::UpdateGizmoTransform()
{
  ezTransform t;
  t.m_vScale = m_Scale;
  t.m_vPosition = m_vPositionOffset;
  t.m_qRotation = m_Rotation;

  m_Gizmo.SetTransformation(GetObjectTransform() * t);
}
