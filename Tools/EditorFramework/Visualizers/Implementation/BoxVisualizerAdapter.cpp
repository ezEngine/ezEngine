#include <PCH.h>
#include <EditorFramework/Visualizers/BoxVisualizerAdapter.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow3D/DocumentWindow3D.moc.h>
#include <EditorFramework/Gizmos/GizmoHandle.h>

ezBoxVisualizerAdapter::ezBoxVisualizerAdapter()
{
}

ezBoxVisualizerAdapter::~ezBoxVisualizerAdapter()
{
}

void ezBoxVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument();

  auto* pWindow = ezQtDocumentWindow::FindWindowByDocument(pDoc);

  ezQtEngineDocumentWindow* pEngineWindow = qobject_cast<ezQtEngineDocumentWindow*>(pWindow);
  EZ_ASSERT_DEV(pEngineWindow != nullptr, "Visualizers are only supported in engine document windows");

  const ezBoxVisualizerAttribute* pAttr = static_cast<const ezBoxVisualizerAttribute*>(m_pVisualizerAttr);

  m_Gizmo.Configure(nullptr, ezEngineGizmoHandleType::Box, pAttr->m_Color, false, false, true);

  m_Gizmo.SetOwner(pEngineWindow);
  m_Gizmo.SetVisible(true);
}

void ezBoxVisualizerAdapter::Update()
{
  const ezBoxVisualizerAttribute* pAttr = static_cast<const ezBoxVisualizerAttribute*>(m_pVisualizerAttr);

  m_Scale.SetIdentity();

  if (!pAttr->GetSizeProperty().IsEmpty())
  {
    ezVariant value = m_pObject->GetTypeAccessor().GetValue(ezPropertyPath(pAttr->GetSizeProperty()));
    m_Scale.SetScalingMatrix(value.ConvertTo<ezVec3>());
  }

  if (!pAttr->GetColorProperty().IsEmpty())
  {
    ezVariant value = m_pObject->GetTypeAccessor().GetValue(ezPropertyPath(pAttr->GetColorProperty()));

    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<ezColor>(), "Invalid property bound to ezBoxVisualizerAttribute 'color'");
    m_Gizmo.SetColor(value.ConvertTo<ezColor>());
  }
}

void ezBoxVisualizerAdapter::UpdateGizmoTransform()
{
  m_Gizmo.SetTransformation(GetObjectTransform().GetAsMat4() * m_Scale);
}


