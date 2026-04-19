#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorPluginScene/Visualizers/DirectionalLightVisualizerAdapter.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezDirectionalLightVisualizerAdapter::ezDirectionalLightVisualizerAdapter() = default;

ezDirectionalLightVisualizerAdapter::~ezDirectionalLightVisualizerAdapter() = default;

void ezDirectionalLightVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const ezAssetDocument* pAssetDocument = ezDynamicCast<const ezAssetDocument*>(pDoc);
  EZ_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in ezAssetDocument.");

  m_hGizmo.ConfigureHandle(nullptr, ezEngineGizmoHandleType::Cone, ezColor::White, ezGizmoFlags::ShowInOrtho | ezGizmoFlags::Visualizer);

  pAssetDocument->AddSyncObject(&m_hGizmo);
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);
}

void ezDirectionalLightVisualizerAdapter::Update()
{
  ezObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  const ezDirectionalLightVisualizerAttribute* pAttr = static_cast<const ezDirectionalLightVisualizerAttribute*>(m_pVisualizerAttr);

  m_fAngleScale = 0.0f;

  if (!pAttr->GetAngleProperty().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetAngleProperty()), value).AssertSuccess();
    EZ_ASSERT_DEBUG(value.CanConvertTo<ezAngle>(), "Invalid property bound to ezDirectionalLightVisualizerAttribute 'angle'");
    m_fAngleScale = ezMath::Tan(value.ConvertTo<ezAngle>() * 0.5f);
  }

  if (!pAttr->GetColorProperty().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetColorProperty()), value).AssertSuccess();
    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<ezColor>(), "Invalid property bound to ezDirectionalLightVisualizerAdapter 'color'");
    m_hGizmo.SetColor(value.ConvertTo<ezColor>());
  }

  m_hGizmo.SetVisible(m_bVisualizerIsVisible && m_fAngleScale > 0.0f);
}

void ezDirectionalLightVisualizerAdapter::UpdateGizmoTransform()
{
  // The cone's length is a purely symbolic value since directional lights have no position.
  // The opening angle is what matters: tan(halfAngle) in the lateral axes scales the cone's
  // base radius relative to its length, which matches the way the spot light visualizer works.
  constexpr float fSymbolicLength = -1.0f;

  ezTransform t = GetObjectTransform();
  t.m_vScale = t.m_vScale.CompMul(ezVec3(1.0f, m_fAngleScale, m_fAngleScale) * fSymbolicLength);
  m_hGizmo.SetTransformation(t);
}
