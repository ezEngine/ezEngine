#include <EditorFrameworkPCH.h>

#include "InputContexts/OrthoGizmoContext.h"
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <EditorFramework/Manipulators/TransformManipulatorAdapter.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezTransformManipulatorAdapter::ezTransformManipulatorAdapter() {}

ezTransformManipulatorAdapter::~ezTransformManipulatorAdapter() {}

void ezTransformManipulatorAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument();

  auto* pWindow = ezQtDocumentWindow::FindWindowByDocument(pDoc);

  ezQtEngineDocumentWindow* pEngineWindow = qobject_cast<ezQtEngineDocumentWindow*>(pWindow);
  EZ_ASSERT_DEV(pEngineWindow != nullptr, "Manipulators are only supported in engine document windows");

  m_TranslateGizmo.SetTransformation(GetObjectTransform());
  m_RotateGizmo.SetTransformation(GetObjectTransform());
  m_ScaleGizmo.SetTransformation(GetObjectTransform());

  const ezTransformManipulatorAttribute* pAttr = static_cast<const ezTransformManipulatorAttribute*>(m_pManipulatorAttr);

  m_TranslateGizmo.SetOwner(pEngineWindow, nullptr);
  m_TranslateGizmo.SetVisible(m_bManipulatorIsVisible);
  m_RotateGizmo.SetOwner(pEngineWindow, nullptr);
  m_RotateGizmo.SetVisible(m_bManipulatorIsVisible && !pAttr->GetRotateProperty().IsEmpty());
  m_ScaleGizmo.SetOwner(pEngineWindow, nullptr);
  m_ScaleGizmo.SetVisible(m_bManipulatorIsVisible && !pAttr->GetScaleProperty().IsEmpty());

  m_TranslateGizmo.m_GizmoEvents.AddEventHandler(ezMakeDelegate(&ezTransformManipulatorAdapter::GizmoEventHandler, this));
  m_RotateGizmo.m_GizmoEvents.AddEventHandler(ezMakeDelegate(&ezTransformManipulatorAdapter::GizmoEventHandler, this));
  m_ScaleGizmo.m_GizmoEvents.AddEventHandler(ezMakeDelegate(&ezTransformManipulatorAdapter::GizmoEventHandler, this));
}

void ezTransformManipulatorAdapter::Update()
{
  UpdateGizmoTransform();
}

void ezTransformManipulatorAdapter::GizmoEventHandler(const ezGizmoEvent& e)
{
  const ezTransformManipulatorAttribute* pAttr = static_cast<const ezTransformManipulatorAttribute*>(m_pManipulatorAttr);
  ezObjectAccessorBase* pAccessor = GetObjectAccessor();
  switch (e.m_Type)
  {
    case ezGizmoEvent::Type::BeginInteractions:
      m_vOldScale = GetScale();
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
      if (e.m_pGizmo == &m_TranslateGizmo || e.m_pGizmo == &m_RotateGizmo || e.m_pGizmo == &m_ScaleGizmo)
      {
        const ezTransform tParent = GetObjectTransform();
        const ezTransform tGlobal = static_cast<const ezGizmo*>(e.m_pGizmo)->GetTransformation();
        ezTransform tLocal;
        tLocal.SetLocalTransform(tParent, tGlobal);
        if (e.m_pGizmo == &m_TranslateGizmo)
        {
          ChangeProperties(pAttr->GetTranslateProperty(), tLocal.m_vPosition);
        }
        else if (e.m_pGizmo == &m_RotateGizmo)
        {
          ChangeProperties(pAttr->GetRotateProperty(), tLocal.m_qRotation);
        }
        else if (e.m_pGizmo == &m_ScaleGizmo)
        {
          ezVec3 vNewScale = m_vOldScale.CompMul(m_ScaleGizmo.GetScalingResult());
          ChangeProperties(pAttr->GetScaleProperty(), vNewScale);
        }
      }
    }
    break;
  }
}


void ezTransformManipulatorAdapter::UpdateGizmoTransform()
{
  const ezTransformManipulatorAttribute* pAttr = static_cast<const ezTransformManipulatorAttribute*>(m_pManipulatorAttr);

  m_TranslateGizmo.SetVisible(m_bManipulatorIsVisible);
  m_RotateGizmo.SetVisible(m_bManipulatorIsVisible && !pAttr->GetRotateProperty().IsEmpty());
  m_ScaleGizmo.SetVisible(m_bManipulatorIsVisible && !pAttr->GetScaleProperty().IsEmpty());

  const ezVec3 vPos = GetTranslation();
  const ezQuat vRot = GetRotation();
  const ezVec3 vScale = GetScale();

  const ezTransform tParent = GetObjectTransform();
  ezTransform tLocal;
  tLocal.m_vPosition = vPos;
  tLocal.m_qRotation = vRot;
  tLocal.m_vScale = vScale;
  ezTransform tGlobal;
  tGlobal.SetGlobalTransform(tParent, tLocal);
  // Let's not apply scaling to the gizmos.
  tGlobal.m_vScale = ezVec3(1, 1, 1);

  m_TranslateGizmo.SetTransformation(tGlobal);
  m_RotateGizmo.SetTransformation(tGlobal);
  m_ScaleGizmo.SetTransformation(tGlobal);
}

ezVec3 ezTransformManipulatorAdapter::GetTranslation()
{
  const ezTransformManipulatorAttribute* pAttr = static_cast<const ezTransformManipulatorAttribute*>(m_pManipulatorAttr);

  if (!pAttr->GetTranslateProperty().IsEmpty())
  {
    ezObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
    return pObjectAccessor->Get<ezVec3>(m_pObject, GetProperty(pAttr->GetTranslateProperty()));
  }

  return ezVec3(0);
}

ezQuat ezTransformManipulatorAdapter::GetRotation()
{
  const ezTransformManipulatorAttribute* pAttr = static_cast<const ezTransformManipulatorAttribute*>(m_pManipulatorAttr);

  if (!pAttr->GetRotateProperty().IsEmpty())
  {
    ezObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
    return pObjectAccessor->Get<ezQuat>(m_pObject, GetProperty(pAttr->GetRotateProperty()));
  }

  return ezQuat::IdentityQuaternion();
}

ezVec3 ezTransformManipulatorAdapter::GetScale()
{
  const ezTransformManipulatorAttribute* pAttr = static_cast<const ezTransformManipulatorAttribute*>(m_pManipulatorAttr);

  if (!pAttr->GetScaleProperty().IsEmpty())
  {
    ezObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
    return pObjectAccessor->Get<ezVec3>(m_pObject, GetProperty(pAttr->GetScaleProperty()));
  }

  return ezVec3(1);
}
