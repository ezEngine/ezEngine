#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorPluginScene/Visualizers/BoxReflectionProbeVisualizerAdapter.h>
#include <RendererCore/Lights/BoxReflectionProbeComponent.h>

#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezBoxReflectionProbeVisualizerAdapter::ezBoxReflectionProbeVisualizerAdapter() = default;
ezBoxReflectionProbeVisualizerAdapter::~ezBoxReflectionProbeVisualizerAdapter() = default;

void ezBoxReflectionProbeVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const ezAssetDocument* pAssetDocument = ezDynamicCast<const ezAssetDocument*>(pDoc);
  EZ_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in ezAssetDocument.");

  m_hGizmo.ConfigureHandle(nullptr, ezEngineGizmoHandleType::LineBox, ezColorScheme::LightUI(ezColorScheme::Yellow), ezGizmoFlags::ShowInOrtho | ezGizmoFlags::Visualizer);

  pAssetDocument->AddSyncObject(&m_hGizmo);
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);
}

void ezBoxReflectionProbeVisualizerAdapter::Update()
{
  const ezBoxReflectionProbeVisualizerAttribute* pAttr = static_cast<const ezBoxReflectionProbeVisualizerAttribute*>(m_pVisualizerAttr);
  ezObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);

  m_vScale.Set(1.0f);
  ezVec3 influenceScale;
  ezVec3 influenceShift;

  if (!pAttr->GetExtentsProperty().IsEmpty())
  {
    m_vScale = pObjectAccessor->Get<ezVec3>(m_pObject, GetProperty(pAttr->GetExtentsProperty()));
  }

  if (!pAttr->GetInfluenceScaleProperty().IsEmpty())
  {
    influenceScale = pObjectAccessor->Get<ezVec3>(m_pObject, GetProperty(pAttr->GetInfluenceScaleProperty()));
  }

  if (!pAttr->GetInfluenceShiftProperty().IsEmpty())
  {
    influenceShift = pObjectAccessor->Get<ezVec3>(m_pObject, GetProperty(pAttr->GetInfluenceShiftProperty()));
  }

  m_vPositionOffset = m_vScale.CompMul(influenceShift.CompMul(ezVec3(1.0f) - influenceScale)) * 0.5f;
  m_vScale *= influenceScale;

  m_qRotation.SetIdentity();
}

void ezBoxReflectionProbeVisualizerAdapter::UpdateGizmoTransform()
{
  ezTransform t;
  t.m_vScale = m_vScale;
  t.m_vPosition = m_vPositionOffset;
  t.m_qRotation = m_qRotation;

  m_hGizmo.SetTransformation(GetObjectTransform() * t);
}
