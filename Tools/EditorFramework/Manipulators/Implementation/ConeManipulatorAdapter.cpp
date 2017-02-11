#include <PCH.h>
#include <EditorFramework/Manipulators/ConeManipulatorAdapter.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezConeManipulatorAdapter::ezConeManipulatorAdapter()
{
}

ezConeManipulatorAdapter::~ezConeManipulatorAdapter()
{
}

void ezConeManipulatorAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument();

  auto* pWindow = ezQtDocumentWindow::FindWindowByDocument(pDoc);

  ezQtEngineDocumentWindow* pEngineWindow = qobject_cast<ezQtEngineDocumentWindow*>(pWindow);
  EZ_ASSERT_DEV(pEngineWindow != nullptr, "Manipulators are only supported in engine document windows");

  m_Gizmo.SetTransformation(GetObjectTransform().GetAsMat4());
  m_Gizmo.SetVisible(m_bManipulatorIsVisible);

  m_Gizmo.SetOwner(pEngineWindow, nullptr);

  m_Gizmo.m_GizmoEvents.AddEventHandler(ezMakeDelegate(&ezConeManipulatorAdapter::GizmoEventHandler, this));
}

void ezConeManipulatorAdapter::Update()
{
  m_Gizmo.SetVisible(m_bManipulatorIsVisible);
  ezObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  const ezConeManipulatorAttribute* pAttr = static_cast<const ezConeManipulatorAttribute*>(m_pManipulatorAttr);

  if (!pAttr->GetRadiusProperty().IsEmpty())
  {
    float fValue = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetRadiusProperty()));
    m_Gizmo.SetRadius(fValue);
  }
  else
  {
    m_Gizmo.SetRadius(pAttr->m_fScale);
    m_Gizmo.SetEnableRadiusHandle(false);
  }

  if (!pAttr->GetAngleProperty().IsEmpty())
  {
    ezAngle value = pObjectAccessor->Get<ezAngle>(m_pObject, GetProperty(pAttr->GetAngleProperty()));
    m_Gizmo.SetAngle(value);
  }

  m_Gizmo.SetTransformation(GetObjectTransform().GetAsMat4());
}

void ezConeManipulatorAdapter::GizmoEventHandler(const ezGizmoEvent& e)
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
      const ezConeManipulatorAttribute* pAttr = static_cast<const ezConeManipulatorAttribute*>(m_pManipulatorAttr);

      ChangeProperties(pAttr->GetAngleProperty(), m_Gizmo.GetAngle(), pAttr->GetRadiusProperty(), m_Gizmo.GetRadius());
    }
    break;
  }
}

void ezConeManipulatorAdapter::UpdateGizmoTransform()
{
  m_Gizmo.SetTransformation(GetObjectTransform().GetAsMat4());
}


