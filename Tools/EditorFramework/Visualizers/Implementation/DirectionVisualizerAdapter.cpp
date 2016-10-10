#include <PCH.h>
#include <EditorFramework/Visualizers/DirectionVisualizerAdapter.h>
#include <EditorFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezDirectionVisualizerAdapter::ezDirectionVisualizerAdapter()
{
}

ezDirectionVisualizerAdapter::~ezDirectionVisualizerAdapter()
{
}

void ezDirectionVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument();
  const ezAssetDocument* pAssetDocument = ezDynamicCast<const ezAssetDocument*>(pDoc);
  EZ_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in ezAssetDocument.");

  const ezDirectionVisualizerAttribute* pAttr = static_cast<const ezDirectionVisualizerAttribute*>(m_pVisualizerAttr);

  m_Gizmo.Configure(nullptr, ezEngineGizmoHandleType::Arrow, pAttr->m_Color, false, false, true);

  m_Gizmo.SetOwner(pAssetDocument);
  m_Gizmo.SetVisible(m_bVisualizerIsVisible);
}

void ezDirectionVisualizerAdapter::Update()
{
  m_Gizmo.SetVisible(m_bVisualizerIsVisible);
  ezObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  const ezDirectionVisualizerAttribute* pAttr = static_cast<const ezDirectionVisualizerAttribute*>(m_pVisualizerAttr);

  if (!pAttr->GetColorProperty().IsEmpty())
  {
    ezVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetColorProperty()), value);

    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<ezColor>(), "Invalid property bound to ezDirectionVisualizerAttribute 'color'");
    m_Gizmo.SetColor(value.ConvertTo<ezColor>());
  }
}

void ezDirectionVisualizerAdapter::UpdateGizmoTransform()
{
  const float fScale = static_cast<const ezDirectionVisualizerAttribute*>(m_pVisualizerAttr)->m_fScale;
  ezMat4 mScale;
  mScale.SetScalingMatrix(ezVec3(fScale));
  mScale.SetTranslationVector(ezVec3(fScale, 0, 0));
  m_Gizmo.SetTransformation(GetObjectTransform().GetAsMat4() * mScale);
}


