#include <PCH.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <EditorFramework/Manipulators/BoxManipulatorAdapter.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezBoxManipulatorAdapter::ezBoxManipulatorAdapter() {}

ezBoxManipulatorAdapter::~ezBoxManipulatorAdapter() {}

void ezBoxManipulatorAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument();

  auto* pWindow = ezQtDocumentWindow::FindWindowByDocument(pDoc);

  ezQtEngineDocumentWindow* pEngineWindow = qobject_cast<ezQtEngineDocumentWindow*>(pWindow);
  EZ_ASSERT_DEV(pEngineWindow != nullptr, "Manipulators are only supported in engine document windows");

  m_Gizmo.SetTransformation(GetObjectTransform());

  m_Gizmo.SetOwner(pEngineWindow, nullptr);
  m_Gizmo.SetVisible(m_bManipulatorIsVisible);

  m_Gizmo.m_GizmoEvents.AddEventHandler(ezMakeDelegate(&ezBoxManipulatorAdapter::GizmoEventHandler, this));
}

void ezBoxManipulatorAdapter::Update()
{
  m_Gizmo.SetVisible(m_bManipulatorIsVisible);
  ezObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  const ezBoxManipulatorAttribute* pAttr = static_cast<const ezBoxManipulatorAttribute*>(m_pManipulatorAttr);

  if (!pAttr->GetSizeProperty().IsEmpty())
  {
    ezVec3 vSize = pObjectAccessor->Get<ezVec3>(m_pObject, GetProperty(pAttr->GetSizeProperty()));
    m_Gizmo.SetSize(vSize);
  }

  m_Gizmo.SetTransformation(GetObjectTransform());
}

void ezBoxManipulatorAdapter::GizmoEventHandler(const ezGizmoEvent& e)
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
      const ezBoxManipulatorAttribute* pAttr = static_cast<const ezBoxManipulatorAttribute*>(m_pManipulatorAttr);

      ChangeProperties(pAttr->GetSizeProperty(), m_Gizmo.GetSize());
    }
    break;
  }
}

void ezBoxManipulatorAdapter::UpdateGizmoTransform()
{
  m_Gizmo.SetTransformation(GetObjectTransform());
}
