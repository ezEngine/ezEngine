#include <PCH.h>
#include <EditorFramework/Manipulators/CapsuleManipulatorAdapter.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow3D/DocumentWindow3D.moc.h>
#include <EditorFramework/Gizmos/GizmoBase.h>

ezCapsuleManipulatorAdapter::ezCapsuleManipulatorAdapter()
{
}

ezCapsuleManipulatorAdapter::~ezCapsuleManipulatorAdapter()
{
}

void ezCapsuleManipulatorAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument();

  auto* pWindow = ezQtDocumentWindow::FindWindowByDocument(pDoc);

  ezQtEngineDocumentWindow* pEngineWindow = qobject_cast<ezQtEngineDocumentWindow*>(pWindow);
  EZ_ASSERT_DEV(pEngineWindow != nullptr, "Manipulators are only supported in engine document windows");

  m_Gizmo.SetTransformation(GetObjectTransform().GetAsMat4());
  m_Gizmo.SetVisible(true);

  m_Gizmo.SetOwner(pEngineWindow, nullptr);

  m_Gizmo.m_GizmoEvents.AddEventHandler(ezMakeDelegate(&ezCapsuleManipulatorAdapter::GizmoEventHandler, this));
}

void ezCapsuleManipulatorAdapter::Update()
{
  const ezCapsuleManipulatorAttribute* pAttr = static_cast<const ezCapsuleManipulatorAttribute*>(m_pManipulatorAttr);

  if (!pAttr->GetLengthProperty().IsEmpty())
  {
    ezVariant value = m_pObject->GetTypeAccessor().GetValue(ezPropertyPath(pAttr->GetLengthProperty()));
    m_Gizmo.SetLength(value.ConvertTo<float>());
  }

  if (!pAttr->GetRadiusProperty().IsEmpty())
  {
    ezVariant value = m_pObject->GetTypeAccessor().GetValue(ezPropertyPath(pAttr->GetRadiusProperty()));
    m_Gizmo.SetRadius(value.ConvertTo<float>());
  }

  m_Gizmo.SetTransformation(GetObjectTransform().GetAsMat4());
}

void ezCapsuleManipulatorAdapter::GizmoEventHandler(const ezGizmoEvent& e)
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
      const ezCapsuleManipulatorAttribute* pAttr = static_cast<const ezCapsuleManipulatorAttribute*>(m_pManipulatorAttr);

      ChangeProperties(pAttr->GetLengthProperty(), m_Gizmo.GetLength(), pAttr->GetRadiusProperty(), m_Gizmo.GetRadius());
    }
    break;
  }
}

void ezCapsuleManipulatorAdapter::UpdateGizmoTransform()
{
  m_Gizmo.SetTransformation(GetObjectTransform().GetAsMat4());
}


