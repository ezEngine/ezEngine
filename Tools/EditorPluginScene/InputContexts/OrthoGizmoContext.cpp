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

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezOrthoGizmoContext, ezEditorInputContext, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezOrthoGizmoContext::ezOrthoGizmoContext(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView, const ezCamera* pCamera)
{
  m_pCamera = pCamera;
  m_bCanInteract = false;

  SetOwner(pOwnerWindow, pOwnerView);
}


void ezOrthoGizmoContext::FocusLost(bool bCancel)
{
  ezGizmo::GizmoEvent e;
  e.m_pGizmo = this;
  e.m_Type = bCancel ? ezGizmo::GizmoEvent::Type::CancelInteractions : ezGizmo::GizmoEvent::Type::EndInteractions;

  m_GizmoEvents.Broadcast(e);


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
  {
    m_bCanInteract = false;
    return ezEditorInut::MayBeHandledByOthers;
  }

  if (e->button() == Qt::MouseButton::LeftButton)
  {
    FocusLost(false);
    return ezEditorInut::WasExclusivelyHandled;
  }

  return ezEditorInut::MayBeHandledByOthers;
}

ezEditorInut ezOrthoGizmoContext::mouseMoveEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
  {
    float fDistPerPixel = 0;

    if (m_pCamera->GetCameraMode() == ezCamera::OrthoFixedHeight)
      fDistPerPixel = m_pCamera->GetFovOrDim() / (float)GetOwnerView()->size().height();

    if (m_pCamera->GetCameraMode() == ezCamera::OrthoFixedWidth)
      fDistPerPixel = m_pCamera->GetFovOrDim() / (float)GetOwnerView()->size().width();

    const QPointF diff = e->globalPos() - m_LastMousePos;

    switch (GetOwnerView()->m_pViewConfig->m_Perspective)
    {
    case ezSceneViewPerspective::Orthogonal_Front:
      m_vTranslationResult.y -= diff.x() * fDistPerPixel;
      m_vTranslationResult.z -= diff.y() * fDistPerPixel;
      break;
    case ezSceneViewPerspective::Orthogonal_Top:
      m_vTranslationResult.y += diff.x() * fDistPerPixel;
      m_vTranslationResult.x -= diff.y() * fDistPerPixel;
      break;
    case ezSceneViewPerspective::Orthogonal_Right:
      m_vTranslationResult.x += diff.x() * fDistPerPixel;
      m_vTranslationResult.z -= diff.y() * fDistPerPixel;
      break;
    }

    m_LastMousePos = e->globalPos();

    ezGizmo::GizmoEvent ev;
    ev.m_pGizmo = this;
    ev.m_Type = ezGizmo::GizmoEvent::Type::Interaction;

    m_GizmoEvents.Broadcast(ev);

    return ezEditorInut::WasExclusivelyHandled;
  }

  if (m_bCanInteract)
  {
    m_LastMousePos = e->globalPos();
    m_vTranslationResult.SetZero();

    m_bCanInteract = false;
    SetActiveInputContext(this);

    ezGizmo::GizmoEvent ev;
    ev.m_pGizmo = this;
    ev.m_Type = ezGizmo::GizmoEvent::Type::BeginInteractions;

    m_GizmoEvents.Broadcast(ev);
    return ezEditorInut::WasExclusivelyHandled;
  }

  return ezEditorInut::MayBeHandledByOthers;
}

bool ezOrthoGizmoContext::IsViewInOthoMode() const
{
  return (GetOwnerView()->m_pViewConfig->m_Perspective != ezSceneViewPerspective::Perspective);
}



