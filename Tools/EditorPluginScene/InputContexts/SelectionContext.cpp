#include <PCH.h>
#include <EditorPluginScene/InputContexts/SelectionContext.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <EditorFramework/DocumentWindow3D/DocumentWindow3D.moc.h>
#include <EditorFramework/IPC/SyncObject.h>
#include <EditorFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <Foundation/Logging/Log.h>
#include <QKeyEvent>

ezSelectionContext::ezSelectionContext(ezDocumentBase* pDocument, ezDocumentWindow3D* pDocumentWindow, const ezCamera* pCamera)
{
  m_pDocument = pDocument;
  m_pDocumentWindow = pDocumentWindow;
  m_pCamera = pCamera;
}

bool ezSelectionContext::mousePressEvent(QMouseEvent* e)
{
  if (e->button() == Qt::MouseButton::LeftButton)
  {
    const ezObjectPickingResult& res = m_pDocumentWindow->PickObject(e->pos().x(), e->pos().y());

    if (res.m_PickedOther.IsValid())
    {
      auto pSO = ezEditorEngineSyncObject::FindSyncObject(res.m_PickedOther);

      if (pSO != nullptr)
      {
        if (pSO->GetDynamicRTTI()->IsDerivedFrom<ezGizmoHandleBase>())
        {
          ezGizmoHandleBase* pGizmoHandle = static_cast<ezGizmoHandleBase*>(pSO);
          ezGizmoBase* pGizmo = pGizmoHandle->GetParentGizmo();
          pGizmo->ConfigureInteraction(pGizmoHandle, m_pCamera, res.m_vPickedPosition, m_Viewport);
          return pGizmo->mousePressEvent(e);
        }
      }
    }
  }

  return false;
}

bool ezSelectionContext::mouseReleaseEvent(QMouseEvent* e)
{
  if (e->button() == Qt::MouseButton::LeftButton)
  {
    const ezObjectPickingResult& res = m_pDocumentWindow->PickObject(e->pos().x(), e->pos().y());

    if (e->modifiers() == Qt::KeyboardModifier::AltModifier)
    {
      if (res.m_PickedComponent.IsValid())
      {
        const ezDocumentObjectBase* pObject = m_pDocument->GetObjectManager()->GetObject(res.m_PickedComponent);
        m_pDocument->GetSelectionManager()->SetSelection(pObject);
      }
    }
    else
    {
      if (res.m_PickedObject.IsValid())
      {
        const ezDocumentObjectBase* pObject = m_pDocument->GetObjectManager()->GetObject(res.m_PickedObject);
        m_pDocument->GetSelectionManager()->SetSelection(pObject);
      }
    }

    // we handled the mouse click event
    // but this is it, we don't stay active
    return true;
  }

  return false;
}

bool ezSelectionContext::mouseMoveEvent(QMouseEvent* e)
{
  ezViewHighlightMsgToEngine msg;

  {
    const ezObjectPickingResult& res = m_pDocumentWindow->PickObject(e->pos().x(), e->pos().y());

    if (res.m_PickedComponent.IsValid())
      msg.m_HighlightObject = res.m_PickedComponent;
    else if (res.m_PickedOther.IsValid())
      msg.m_HighlightObject = res.m_PickedOther;
    else
      msg.m_HighlightObject = res.m_PickedObject;
  }

  msg.SendHighlightObjectMessage(m_pDocumentWindow->GetEditorEngineConnection());

  // we only updated the highlight, so others may do additional stuff, if they like
  return false;
}
