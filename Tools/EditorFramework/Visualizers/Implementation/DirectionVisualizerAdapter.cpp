#include <PCH.h>
#include <EditorFramework/Visualizers/DirectionVisualizerAdapter.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow3D/DocumentWindow3D.moc.h>
#include <EditorFramework/Gizmos/GizmoHandle.h>

ezDirectionVisualizerAdapter::ezDirectionVisualizerAdapter()
{
}

ezDirectionVisualizerAdapter::~ezDirectionVisualizerAdapter()
{
}

void ezDirectionVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument();

  auto* pWindow = ezQtDocumentWindow::FindWindowByDocument(pDoc);

  ezQtEngineDocumentWindow* pEngineWindow = qobject_cast<ezQtEngineDocumentWindow*>(pWindow);
  EZ_ASSERT_DEV(pEngineWindow != nullptr, "Visualizers are only supported in engine document windows");

  const ezDirectionVisualizerAttribute* pAttr = static_cast<const ezDirectionVisualizerAttribute*>(m_pVisualizerAttr);

  m_Gizmo.Configure(nullptr, ezEngineGizmoHandleType::Arrow, pAttr->m_Color, false, false, true);

  m_Gizmo.SetOwner(pEngineWindow);
  m_Gizmo.SetVisible(true);
}

void ezDirectionVisualizerAdapter::Update()
{
  const ezDirectionVisualizerAttribute* pAttr = static_cast<const ezDirectionVisualizerAttribute*>(m_pVisualizerAttr);

  if (!pAttr->GetColorProperty().IsEmpty())
  {
    ezVariant value = m_pObject->GetTypeAccessor().GetValue(ezPropertyPath(pAttr->GetColorProperty()));

    EZ_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<ezColor>(), "Invalid property bound to ezDirectionVisualizerAttribute 'color'");
    m_Gizmo.SetColor(value.ConvertTo<ezColor>());
  }
}

void ezDirectionVisualizerAdapter::UpdateGizmoTransform()
{
  m_Gizmo.SetTransformation(GetObjectTransform().GetAsMat4());
}


