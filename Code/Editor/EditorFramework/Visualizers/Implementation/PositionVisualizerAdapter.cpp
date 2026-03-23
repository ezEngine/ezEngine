#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Visualizers/PositionVisualizerAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezPositionVisualizerAdapter::ezPositionVisualizerAdapter() = default;
ezPositionVisualizerAdapter::~ezPositionVisualizerAdapter() = default;

void ezPositionVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const ezAssetDocument* pAssetDocument = ezDynamicCast<const ezAssetDocument*>(pDoc);
  EZ_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in ezAssetDocument.");

  const ezPositionVisualizerAttribute* pAttr = static_cast<const ezPositionVisualizerAttribute*>(m_pVisualizerAttr);

  m_hGizmo.ConfigureHandle(nullptr, ezEngineGizmoHandleType::Cross, pAttr->m_Color, ezGizmoFlags::ShowInOrtho | ezGizmoFlags::Visualizer);

  pAssetDocument->AddSyncObject(&m_hGizmo);
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);
}

void ezPositionVisualizerAdapter::Update()
{
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);
  ezObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  const ezPositionVisualizerAttribute* pAttr = static_cast<const ezPositionVisualizerAttribute*>(m_pVisualizerAttr);

  m_vPosition = ezVec3::MakeZero();
  m_fScale = pAttr->m_fSizeScale;

  if (!pAttr->GetPositionProperty().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetPositionProperty()), value).AssertSuccess();

    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<ezVec3>(), "Invalid property bound to ezPositionVisualizerAttribute 'position'");
    m_vPosition = value.ConvertTo<ezVec3>();
  }

  if (!pAttr->GetColorProperty().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetColorProperty()), value).AssertSuccess();

    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<ezColor>(), "Invalid property bound to ezPositionVisualizerAttribute 'color'");
    m_hGizmo.SetColor(value.ConvertTo<ezColor>() * pAttr->m_Color);
  }
}

void ezPositionVisualizerAdapter::UpdateGizmoTransform()
{
  ezTransform t;
  t.SetIdentity();
  t.m_vScale.Set(m_fScale);
  t.m_vPosition = m_vPosition;

  m_hGizmo.SetTransformation(GetObjectTransform() * t);
}
