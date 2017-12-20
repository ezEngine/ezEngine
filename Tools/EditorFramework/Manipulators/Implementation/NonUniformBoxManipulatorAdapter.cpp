#include <PCH.h>
#include <EditorFramework/Manipulators/NonUniformBoxManipulatorAdapter.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezNonUniformBoxManipulatorAdapter::ezNonUniformBoxManipulatorAdapter()
{
}

ezNonUniformBoxManipulatorAdapter::~ezNonUniformBoxManipulatorAdapter()
{
}

void ezNonUniformBoxManipulatorAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument();

  auto* pWindow = ezQtDocumentWindow::FindWindowByDocument(pDoc);

  ezQtEngineDocumentWindow* pEngineWindow = qobject_cast<ezQtEngineDocumentWindow*>(pWindow);
  EZ_ASSERT_DEV(pEngineWindow != nullptr, "Manipulators are only supported in engine document windows");

  m_Gizmo.SetTransformation(GetObjectTransform());

  m_Gizmo.SetOwner(pEngineWindow, nullptr);
  m_Gizmo.SetVisible(m_bManipulatorIsVisible);

  m_Gizmo.m_GizmoEvents.AddEventHandler(ezMakeDelegate(&ezNonUniformBoxManipulatorAdapter::GizmoEventHandler, this));
}

void ezNonUniformBoxManipulatorAdapter::Update()
{
  m_Gizmo.SetVisible(m_bManipulatorIsVisible);
  ezObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  const ezNonUniformBoxManipulatorAttribute* pAttr = static_cast<const ezNonUniformBoxManipulatorAttribute*>(m_pManipulatorAttr);

  if (pAttr->HasSixAxis())
  {
    const float fNegX = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetNegXProperty()));
    const float fPosX = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetPosXProperty()));
    const float fNegY = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetNegYProperty()));
    const float fPosY = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetPosYProperty()));
    const float fNegZ = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetNegZProperty()));
    const float fPosZ = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetPosZProperty()));

    m_Gizmo.SetSize(ezVec3(fNegX, fNegY, fNegZ), ezVec3(fPosX, fPosY, fPosZ));
  }
  else
  {
    const float fSizeX = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetSizeXProperty()));
    const float fSizeY = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetSizeYProperty()));
    const float fSizeZ = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetSizeZProperty()));

    m_Gizmo.SetSize(ezVec3(fSizeX, fSizeY, fSizeZ) * 0.5f, ezVec3(fSizeX, fSizeY, fSizeZ) * 0.5f, true);

  }

  m_Gizmo.SetTransformation(GetObjectTransform());
}

void ezNonUniformBoxManipulatorAdapter::GizmoEventHandler(const ezGizmoEvent& e)
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
      const ezNonUniformBoxManipulatorAttribute* pAttr = static_cast<const ezNonUniformBoxManipulatorAttribute*>(m_pManipulatorAttr);

      const ezVec3 neg = m_Gizmo.GetNegSize();
      const ezVec3 pos = m_Gizmo.GetPosSize();

      if (pAttr->HasSixAxis())
      {
        ChangeProperties(pAttr->GetNegXProperty(), neg.x,
          pAttr->GetPosXProperty(), pos.x,
          pAttr->GetNegYProperty(), neg.y,
          pAttr->GetPosYProperty(), pos.y,
          pAttr->GetNegZProperty(), neg.z,
          pAttr->GetPosZProperty(), pos.z);
      }
      else
      {
        ChangeProperties(pAttr->GetSizeXProperty(), pos.x * 2,
          pAttr->GetSizeYProperty(), pos.y * 2,
          pAttr->GetSizeZProperty(), pos.z * 2);
      }
    }
    break;
  }
}

void ezNonUniformBoxManipulatorAdapter::UpdateGizmoTransform()
{
  m_Gizmo.SetTransformation(GetObjectTransform());
}


