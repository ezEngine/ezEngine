#include <PCH.h>
#include <EditorFramework/Manipulators/BoxManipulatorAdapter.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow3D/DocumentWindow3D.moc.h>
#include <EditorFramework/Gizmos/GizmoBase.h>

ezBoxManipulatorAdapter::ezBoxManipulatorAdapter()
{
}

ezBoxManipulatorAdapter::~ezBoxManipulatorAdapter()
{
}

void ezBoxManipulatorAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument();

  auto* pWindow = ezQtDocumentWindow::FindWindowByDocument(pDoc);

  ezQtEngineDocumentWindow* pEngineWindow = qobject_cast<ezQtEngineDocumentWindow*>(pWindow);
  EZ_ASSERT_DEV(pEngineWindow != nullptr, "Manipulators are only supported in engine document windows");

  m_Gizmo.SetTransformation(GetObjectTransform().GetAsMat4());

  m_Gizmo.SetOwner(pEngineWindow, nullptr);
  m_Gizmo.SetVisible(m_bManipulatorIsVisible);

  m_Gizmo.m_GizmoEvents.AddEventHandler(ezMakeDelegate(&ezBoxManipulatorAdapter::GizmoEventHandler, this));
}

void ezBoxManipulatorAdapter::Update()
{
  m_Gizmo.SetVisible(m_bManipulatorIsVisible);

  const ezBoxManipulatorAttribute* pAttr = static_cast<const ezBoxManipulatorAttribute*>(m_pManipulatorAttr);

  if (!pAttr->GetSizeProperty().IsEmpty())
  {
    ezVariant value = m_pObject->GetTypeAccessor().GetValue(ezPropertyPath(pAttr->GetSizeProperty()));
    m_Gizmo.SetSize(value.ConvertTo<ezVec3>());
  }

  m_Gizmo.SetTransformation(GetObjectTransform().GetAsMat4());
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
  m_Gizmo.SetTransformation(GetObjectTransform().GetAsMat4());
}


