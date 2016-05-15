#include <PCH.h>
#include <EditorFramework/Manipulators/SphereManipulatorAdapter.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow3D/DocumentWindow3D.moc.h>
#include <EditorFramework/Gizmos/GizmoBase.h>

ezSphereManipulatorAdapter::ezSphereManipulatorAdapter()
{
}

ezSphereManipulatorAdapter::~ezSphereManipulatorAdapter()
{
}

void ezSphereManipulatorAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument();

  auto* pWindow = ezQtDocumentWindow::FindWindowByDocument(pDoc);

  ezQtEngineDocumentWindow* pEngineWindow = qobject_cast<ezQtEngineDocumentWindow*>(pWindow);
  EZ_ASSERT_DEV(pEngineWindow != nullptr, "Manipulators are only supported in engine document windows");

  m_Gizmo.SetTransformation(GetObjectTransform().GetAsMat4());
  m_Gizmo.SetVisible(m_bManipulatorIsVisible);

  m_Gizmo.SetOwner(pEngineWindow, nullptr);

  m_Gizmo.m_GizmoEvents.AddEventHandler(ezMakeDelegate(&ezSphereManipulatorAdapter::GizmoEventHandler, this));
}

void ezSphereManipulatorAdapter::Update()
{
  m_Gizmo.SetVisible(m_bManipulatorIsVisible);

  const ezSphereManipulatorAttribute* pAttr = static_cast<const ezSphereManipulatorAttribute*>(m_pManipulatorAttr);

  if (!pAttr->GetInnerRadiusProperty().IsEmpty())
  {
    ezVariant value = m_pObject->GetTypeAccessor().GetValue(ezPropertyPath(pAttr->GetInnerRadiusProperty()));
    m_Gizmo.SetInnerSphere(true, value.ConvertTo<float>());
  }

  if (!pAttr->GetOuterRadiusProperty().IsEmpty())
  {
    ezVariant value = m_pObject->GetTypeAccessor().GetValue(ezPropertyPath(pAttr->GetOuterRadiusProperty()));
    m_Gizmo.SetOuterSphere(value.ConvertTo<float>());
  }

  m_Gizmo.SetTransformation(GetObjectTransform().GetAsMat4());
}

void ezSphereManipulatorAdapter::GizmoEventHandler(const ezGizmoEvent& e)
{
  switch (e.m_Type)
  {
  case ezGizmoEvent::Type::BeginInteractions:
    BeginTemporaryInteraction();
    break;

  case ezGizmoEvent::Type::CancelInteractions:
    CancelTemporayInteraction();
    break;

  case ezGizmoEvent::Type::EndInteractions:
    EndTemporaryInteraction();
    break;

  case ezGizmoEvent::Type::Interaction:
    {
      const ezSphereManipulatorAttribute* pAttr = static_cast<const ezSphereManipulatorAttribute*>(m_pManipulatorAttr);

      ChangeProperties(pAttr->GetInnerRadiusProperty(), m_Gizmo.GetInnerRadius(), pAttr->GetOuterRadiusProperty(), m_Gizmo.GetOuterRadius());
    }
    break;
  }
}

void ezSphereManipulatorAdapter::UpdateGizmoTransform()
{
  m_Gizmo.SetTransformation(GetObjectTransform().GetAsMat4());
}


