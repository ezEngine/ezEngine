#include <PCH.h>
#include <EditorPluginScene/InputContexts/OrthoGizmoContext.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <EditorFramework/DocumentWindow3D/DocumentWindow3D.moc.h>
#include <EditorFramework/IPC/SyncObject.h>
#include <EditorFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <Foundation/Logging/Log.h>
#include <QKeyEvent>
#include <EditorFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow3D/3DViewWidget.moc.h>

ezOrthoGizmoContext::ezOrthoGizmoContext(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView, const ezCamera* pCamera)
{
  m_pCamera = pCamera;
  m_bCanInteract = false;

  SetOwner(pOwnerWindow, pOwnerView);
}


void ezOrthoGizmoContext::FocusLost(bool bCancel)
{
  m_bCanInteract = false;
  SetActiveInputContext(nullptr);
}

ezEditorInut ezOrthoGizmoContext::mousePressEvent(QMouseEvent* e)
{
  if (!IsViewInOthoMode())
    return ezEditorInut::MayBeHandledByOthers;
  if (GetOwnerWindow()->GetDocument()->GetSelectionManager()->IsSelectionEmpty())
    return ezEditorInut::MayBeHandledByOthers;

  if (e->button() == Qt::MouseButton::LeftButton)
  {
    m_bCanInteract = true;

  }

  return ezEditorInut::MayBeHandledByOthers;
}

ezEditorInut ezOrthoGizmoContext::mouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInut::MayBeHandledByOthers;

  if (e->button() == Qt::MouseButton::LeftButton)
  {
    FocusLost(false);
    return ezEditorInut::WasExclusivelyHandled;
  }

  return ezEditorInut::MayBeHandledByOthers;
}

ezEditorInut ezOrthoGizmoContext::mouseMoveEvent(QMouseEvent* e)
{
  if (!m_bCanInteract && !IsActiveInputContext())
    return ezEditorInut::MayBeHandledByOthers;

  m_bCanInteract = false;
  SetActiveInputContext(this);



  return ezEditorInut::WasExclusivelyHandled;
}

bool ezOrthoGizmoContext::IsViewInOthoMode() const
{
  return (GetOwnerView()->m_pViewConfig->m_Perspective != ezSceneViewPerspective::Perspective);
}



