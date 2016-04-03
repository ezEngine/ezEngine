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

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezOrthoGizmoContext, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezOrthoGizmoContext::ezOrthoGizmoContext(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView, const ezCamera* pCamera)
{
  m_pCamera = pCamera;
  m_bCanInteract = false;

  SetOwner(pOwnerWindow, pOwnerView);
}


void ezOrthoGizmoContext::FocusLost(bool bCancel)
{
  ezGizmoEvent e;
  e.m_pGizmo = this;
  e.m_Type = bCancel ? ezGizmoEvent::Type::CancelInteractions : ezGizmoEvent::Type::EndInteractions;

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

    if (m_pCamera->GetCameraMode() == ezCameraMode::OrthoFixedHeight)
      fDistPerPixel = m_pCamera->GetFovOrDim() / (float)GetOwnerView()->size().height();

    if (m_pCamera->GetCameraMode() == ezCameraMode::OrthoFixedWidth)
      fDistPerPixel = m_pCamera->GetFovOrDim() / (float)GetOwnerView()->size().width();

    const QPointF diff = e->globalPos() - m_LastMousePos;
    ezQuat qRot;
    qRot.SetIdentity();

    switch (GetOwnerView()->m_pViewConfig->m_Perspective)
    {
    case ezSceneViewPerspective::Orthogonal_Front:
      m_vTranslationResult.y -= diff.x() * fDistPerPixel;
      m_vTranslationResult.z -= diff.y() * fDistPerPixel;
      qRot.SetFromAxisAndAngle(ezVec3(1, 0, 0), ezAngle::Degree(diff.x()));
      break;
    case ezSceneViewPerspective::Orthogonal_Top:
      m_vTranslationResult.y += diff.x() * fDistPerPixel;
      m_vTranslationResult.x -= diff.y() * fDistPerPixel;
      qRot.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(diff.x()));
      break;
    case ezSceneViewPerspective::Orthogonal_Right:
      m_vTranslationResult.x += diff.x() * fDistPerPixel;
      m_vTranslationResult.z -= diff.y() * fDistPerPixel;
      qRot.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(diff.x()));
      break;
    }

    m_qRotationResult = qRot * m_qRotationResult;

    {
      float fFactor = 1.0f; // m_fSnappingValue != 0.0f ? m_fSnappingValue : 1.0f;
      m_fScaleMouseMove += diff.x() * fFactor;
      m_fScalingResult = 1.0f;

      const float fScaleSpeed = 0.01f;

      if (m_fScaleMouseMove > 0.0f)
        m_fScalingResult = 1.0f + m_fScaleMouseMove * fScaleSpeed;
      if (m_fScaleMouseMove < 0.0f)
        m_fScalingResult = 1.0f / (1.0f - m_fScaleMouseMove * fScaleSpeed);
    }

    m_LastMousePos = e->globalPos();

    ezGizmoEvent ev;
    ev.m_pGizmo = this;
    ev.m_Type = ezGizmoEvent::Type::Interaction;

    m_GizmoEvents.Broadcast(ev);

    return ezEditorInut::WasExclusivelyHandled;
  }

  if (m_bCanInteract)
  {
    m_LastMousePos = e->globalPos();
    m_vTranslationResult.SetZero();
    m_qRotationResult.SetIdentity();
    m_fScalingResult = 1.0f;
    m_fScaleMouseMove = 0.0f;

    m_bCanInteract = false;
    SetActiveInputContext(this);

    ezGizmoEvent ev;
    ev.m_pGizmo = this;
    ev.m_Type = ezGizmoEvent::Type::BeginInteractions;

    m_GizmoEvents.Broadcast(ev);
    return ezEditorInut::WasExclusivelyHandled;
  }

  return ezEditorInut::MayBeHandledByOthers;
}

bool ezOrthoGizmoContext::IsViewInOthoMode() const
{
  return (GetOwnerView()->m_pViewConfig->m_Perspective != ezSceneViewPerspective::Perspective);
}



