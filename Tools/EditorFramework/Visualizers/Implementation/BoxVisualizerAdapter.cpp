#include <PCH.h>
#include <EditorFramework/Visualizers/BoxVisualizerAdapter.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezBoxVisualizerAdapter::ezBoxVisualizerAdapter()
{
}

ezBoxVisualizerAdapter::~ezBoxVisualizerAdapter()
{
}

void ezBoxVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument();
  const ezAssetDocument* pAssetDocument = ezDynamicCast<const ezAssetDocument*>(pDoc);
  EZ_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in ezAssetDocument.");

  const ezBoxVisualizerAttribute* pAttr = static_cast<const ezBoxVisualizerAttribute*>(m_pVisualizerAttr);

  m_Gizmo.Configure(nullptr, ezEngineGizmoHandleType::LineBox, pAttr->m_Color, false, false, true);

  pAssetDocument->AddSyncObject(&m_Gizmo);
  m_Gizmo.SetVisible(m_bVisualizerIsVisible);
}

void ezBoxVisualizerAdapter::Update()
{
  const ezBoxVisualizerAttribute* pAttr = static_cast<const ezBoxVisualizerAttribute*>(m_pVisualizerAttr);
  ezObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  m_Gizmo.SetVisible(m_bVisualizerIsVisible);

  m_Scale.Set(1.0f);

  if (!pAttr->GetSizeProperty().IsEmpty())
  {

    m_Scale = pObjectAccessor->Get<ezVec3>(m_pObject, GetProperty(pAttr->GetSizeProperty()));
  }

  if (!pAttr->GetColorProperty().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetColorProperty()), value);
    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<ezColor>(), "Invalid property bound to ezBoxVisualizerAttribute 'color'");
    m_Gizmo.SetColor(value.ConvertTo<ezColor>());
  }
}

void ezBoxVisualizerAdapter::UpdateGizmoTransform()
{
  ezTransform t = GetObjectTransform();
  t.m_vScale = t.m_vScale.CompMul(m_Scale);
  m_Gizmo.SetTransformation(t);
}


