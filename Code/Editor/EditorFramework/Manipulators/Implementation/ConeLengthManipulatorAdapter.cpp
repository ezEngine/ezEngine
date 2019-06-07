#include <EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <EditorFramework/Manipulators/ConeLengthManipulatorAdapter.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezConeLengthManipulatorAdapter::ezConeLengthManipulatorAdapter() {}

ezConeLengthManipulatorAdapter::~ezConeLengthManipulatorAdapter() {}

void ezConeLengthManipulatorAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument();

  auto* pWindow = ezQtDocumentWindow::FindWindowByDocument(pDoc);

  ezQtEngineDocumentWindow* pEngineWindow = qobject_cast<ezQtEngineDocumentWindow*>(pWindow);
  EZ_ASSERT_DEV(pEngineWindow != nullptr, "Manipulators are only supported in engine document windows");

  m_Gizmo.SetTransformation(GetObjectTransform());
  m_Gizmo.SetVisible(m_bManipulatorIsVisible);

  m_Gizmo.SetOwner(pEngineWindow, nullptr);

  m_Gizmo.m_GizmoEvents.AddEventHandler(ezMakeDelegate(&ezConeLengthManipulatorAdapter::GizmoEventHandler, this));
}

void ezConeLengthManipulatorAdapter::Update()
{
  m_Gizmo.SetVisible(m_bManipulatorIsVisible);
  ezObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  const ezConeLengthManipulatorAttribute* pAttr = static_cast<const ezConeLengthManipulatorAttribute*>(m_pManipulatorAttr);

  if (!pAttr->GetRadiusProperty().IsEmpty())
  {
    float fValue = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetRadiusProperty()));
    m_Gizmo.SetRadius(fValue);
  }

  m_Gizmo.SetTransformation(GetObjectTransform());
}

void ezConeLengthManipulatorAdapter::GizmoEventHandler(const ezGizmoEvent& e)
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
      const ezConeLengthManipulatorAttribute* pAttr = static_cast<const ezConeLengthManipulatorAttribute*>(m_pManipulatorAttr);

      ChangeProperties(pAttr->GetRadiusProperty(), m_Gizmo.GetRadius());
    }
    break;
  }
}

void ezConeLengthManipulatorAdapter::UpdateGizmoTransform()
{
  m_Gizmo.SetTransformation(GetObjectTransform());
}
