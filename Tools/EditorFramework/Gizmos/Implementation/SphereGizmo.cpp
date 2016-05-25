#include <PCH.h>
#include <EditorFramework/Gizmos/SphereGizmo.h>
#include <EditorFramework/DocumentWindow3D/DocumentWindow3D.moc.h>
#include <Foundation/Logging/Log.h>
#include <CoreUtils/Graphics/Camera.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <EditorFramework/DocumentWindow3D/3DViewWidget.moc.h>
#include <QMouseEvent>
#include <QDesktopWidget>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSphereGizmo, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezSphereGizmo::ezSphereGizmo()
{
  m_bInnerEnabled = false;

  m_fRadiusInner = 1.0f;
  m_fRadiusOuter = 2.0f;

  m_ManipulateMode = ManipulateMode::None;

  m_InnerSphere.Configure(this, ezEngineGizmoHandleType::Sphere, ezColorLinearUB(200, 200, 0, 128), false, true); // this gizmo should be rendered very last so it is always on top
  m_OuterSphere.Configure(this, ezEngineGizmoHandleType::Sphere, ezColorLinearUB(200, 200, 200, 128), false);

  SetVisible(false);
  SetTransformation(ezMat4::IdentityMatrix());
}

void ezSphereGizmo::OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView)
{
  m_InnerSphere.SetOwner(pOwnerWindow);
  m_OuterSphere.SetOwner(pOwnerWindow);
}

void ezSphereGizmo::OnVisibleChanged(bool bVisible)
{
  m_InnerSphere.SetVisible(bVisible && m_bInnerEnabled);
  m_OuterSphere.SetVisible(bVisible);
}

void ezSphereGizmo::OnTransformationChanged(const ezMat4& transform)
{
  ezMat4 mScaleInner, mScaleOuter;
  mScaleInner.SetScalingMatrix(ezVec3(m_fRadiusInner));
  mScaleOuter.SetScalingMatrix(ezVec3(m_fRadiusOuter));

  m_InnerSphere.SetTransformation(transform * mScaleInner);
  m_OuterSphere.SetTransformation(transform * mScaleOuter);
}

void ezSphereGizmo::DoFocusLost(bool bCancel)
{
  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = bCancel ? ezGizmoEvent::Type::CancelInteractions : ezGizmoEvent::Type::EndInteractions;
  m_GizmoEvents.Broadcast(ev);

  ezViewHighlightMsgToEngine msg;
  msg.SendHighlightObjectMessage(GetOwnerWindow()->GetEditorEngineConnection());

  m_InnerSphere.SetVisible(m_bInnerEnabled);
  m_OuterSphere.SetVisible(true);

  m_ManipulateMode = ManipulateMode::None;
}

ezEditorInut ezSphereGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return ezEditorInut::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return ezEditorInut::MayBeHandledByOthers;

  if (m_pInteractionGizmoHandle == &m_InnerSphere)
  {
    m_ManipulateMode = ManipulateMode::InnerSphere;
  }
  else if (m_pInteractionGizmoHandle == &m_OuterSphere)
  {
    m_ManipulateMode = ManipulateMode::OuterSphere;
  }
  else
    return ezEditorInut::MayBeHandledByOthers;

  ezViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  msg.SendHighlightObjectMessage(GetOwnerWindow()->GetEditorEngineConnection());

  //m_InnerSphere.SetVisible(false);
  //m_OuterSphere.SetVisible(false);

  //m_pInteractionGizmoHandle->SetVisible(true);

  m_LastInteraction = ezTime::Now();

  m_LastMousePos = SetMouseMode(ezEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);

  SetActiveInputContext(this);

  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = ezGizmoEvent::Type::BeginInteractions;
  m_GizmoEvents.Broadcast(ev);

  return ezEditorInut::WasExclusivelyHandled;
}

ezEditorInut ezSphereGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInut::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return ezEditorInut::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return ezEditorInut::WasExclusivelyHandled;
}

ezEditorInut ezSphereGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInut::MayBeHandledByOthers;

  const ezTime tNow = ezTime::Now();

  if (tNow - m_LastInteraction < ezTime::Seconds(1.0 / 25.0))
    return ezEditorInut::WasExclusivelyHandled;

  m_LastInteraction = tNow;

  const ezVec2I32 vNewMousePos = ezVec2I32(e->globalPos().x(), e->globalPos().y());
  const ezVec2I32 vDiff = vNewMousePos - m_LastMousePos;

  m_LastMousePos = UpdateMouseMode(e);

  const float fSpeed = 0.02f;

  if (m_ManipulateMode == ManipulateMode::InnerSphere)
  {
    m_fRadiusInner += vDiff.x * fSpeed;
    m_fRadiusInner -= vDiff.y * fSpeed;

    m_fRadiusInner = ezMath::Max(0.0f, m_fRadiusInner);

    m_fRadiusOuter = ezMath::Max(m_fRadiusInner, m_fRadiusOuter);
  }
  else
  {
    m_fRadiusOuter += vDiff.x * fSpeed;
    m_fRadiusOuter -= vDiff.y * fSpeed;

    m_fRadiusOuter = ezMath::Max(0.0f, m_fRadiusOuter);

    m_fRadiusInner = ezMath::Min(m_fRadiusInner, m_fRadiusOuter);
  }

  // update the scale
  OnTransformationChanged(GetTransformation());

  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = ezGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  return ezEditorInut::WasExclusivelyHandled;
}

void ezSphereGizmo::SetInnerSphere(bool bEnabled, float fRadius)
{
  m_fRadiusInner = fRadius;
  m_bInnerEnabled = bEnabled;
  
  // update the scale
  OnTransformationChanged(GetTransformation());
}

void ezSphereGizmo::SetOuterSphere(float fRadius)
{
  m_fRadiusOuter = fRadius;

  // update the scale
  OnTransformationChanged(GetTransformation());
}

