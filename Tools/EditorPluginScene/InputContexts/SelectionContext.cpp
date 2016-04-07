#include <PCH.h>
#include <EditorPluginScene/InputContexts/SelectionContext.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <EditorFramework/DocumentWindow3D/DocumentWindow3D.moc.h>
#include <EditorFramework/IPC/SyncObject.h>
#include <EditorFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <Foundation/Logging/Log.h>
#include <QKeyEvent>

ezSelectionContext::ezSelectionContext(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView, const ezCamera* pCamera)
{
  m_pCamera = pCamera;
  m_bSelectOnMouseUp = false;

  SetOwner(pOwnerWindow, pOwnerView);
}

ezEditorInut ezSelectionContext::mousePressEvent(QMouseEvent* e)
{
  if (e->button() == Qt::MouseButton::LeftButton)
  {
    m_bSelectOnMouseUp = false;

    const ezObjectPickingResult& res = GetOwnerWindow()->PickObject(e->pos().x(), e->pos().y());

    if (res.m_PickedOther.IsValid())
    {
      auto pSO = GetOwnerWindow()->FindSyncObject(res.m_PickedOther);

      if (pSO != nullptr)
      {
        if (pSO->GetDynamicRTTI()->IsDerivedFrom<ezGizmoHandle>())
        {
          ezGizmoHandle* pGizmoHandle = static_cast<ezGizmoHandle*>(pSO);
          ezGizmo* pGizmo = pGizmoHandle->GetOwnerGizmo();

          if (pGizmo)
          {
            pGizmo->ConfigureInteraction(pGizmoHandle, m_pCamera, res.m_vPickedPosition, m_Viewport);
            return pGizmo->mousePressEvent(e);
          }
        }
      }
    }

    m_bSelectOnMouseUp = true;
  }

  return ezEditorInut::MayBeHandledByOthers;
}

ezEditorInut ezSelectionContext::mouseReleaseEvent(QMouseEvent* e)
{
  auto* pDocument = GetOwnerWindow()->GetDocument();

  if (e->button() == Qt::MouseButton::LeftButton && m_bSelectOnMouseUp)
  {
    const ezObjectPickingResult& res = GetOwnerWindow()->PickObject(e->pos().x(), e->pos().y());

    const bool bToggle = (e->modifiers() & Qt::KeyboardModifier::ControlModifier) != 0;

    if (e->modifiers() & Qt::KeyboardModifier::AltModifier)
    {
      if (res.m_PickedComponent.IsValid())
      {
        const ezDocumentObject* pObject = pDocument->GetObjectManager()->GetObject(res.m_PickedComponent);

        if (bToggle)
          pDocument->GetSelectionManager()->ToggleObject(pObject);
        else
          pDocument->GetSelectionManager()->SetSelection(pObject);
      }
    }
    else
    {
      if (res.m_PickedObject.IsValid())
      {
        const ezDocumentObject* pObject = pDocument->GetObjectManager()->GetObject(res.m_PickedObject);

        if (bToggle)
          pDocument->GetSelectionManager()->ToggleObject(pObject);
        else
          pDocument->GetSelectionManager()->SetSelection(pObject);
      }
    }

    m_bSelectOnMouseUp = false;

    // we handled the mouse click event
    // but this is it, we don't stay active
    return ezEditorInut::WasExclusivelyHandled;
  }

  return ezEditorInut::MayBeHandledByOthers;
}

ezEditorInut ezSelectionContext::mouseMoveEvent(QMouseEvent* e)
{
  ezViewHighlightMsgToEngine msg;

  {
    const ezObjectPickingResult& res = GetOwnerWindow()->PickObject(e->pos().x(), e->pos().y());

    if (res.m_PickedComponent.IsValid())
    {
      ezSceneDocument* pScene = static_cast<ezSceneDocument*>(GetOwnerWindow()->GetDocument());

      pScene->SetPickingResult(res);
    }

    if (res.m_PickedComponent.IsValid())
      msg.m_HighlightObject = res.m_PickedComponent;
    else if (res.m_PickedOther.IsValid())
      msg.m_HighlightObject = res.m_PickedOther;
    else
      msg.m_HighlightObject = res.m_PickedObject;
  }

  msg.SendHighlightObjectMessage(GetOwnerWindow()->GetEditorEngineConnection());

  // we only updated the highlight, so others may do additional stuff, if they like
  return ezEditorInut::MayBeHandledByOthers;
}

ezEditorInut ezSelectionContext::keyPressEvent(QKeyEvent* e)
{
  if (e->key() == Qt::Key_Delete)
  {
    GetOwnerWindow()->GetDocument()->DeleteSelectedObjects();
    return ezEditorInut::WasExclusivelyHandled;
  }

  if (e->key() == Qt::Key_Escape)
  {
    GetOwnerWindow()->GetDocument()->GetSelectionManager()->Clear();
    return ezEditorInut::WasExclusivelyHandled;
  }

  return ezEditorInut::MayBeHandledByOthers;
}




