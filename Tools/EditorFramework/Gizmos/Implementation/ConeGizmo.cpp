#include <PCH.h>
#include <EditorFramework/Gizmos/ConeGizmo.h>
#include <EditorFramework/DocumentWindow3D/DocumentWindow3D.moc.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <EditorFramework/DocumentWindow3D/3DViewWidget.moc.h>
#include <QMouseEvent>
#include <QDesktopWidget>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezConeGizmo, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezConeGizmo::ezConeGizmo()
{
  m_fRadius = 1.0f;
  m_Angle = ezAngle::Degree(1.0f);
  m_fAngleScale = 1.0f;
  m_fRadiusScale = 1.0f;
  m_bEnableRadiusHandle = true;

  m_ManipulateMode = ManipulateMode::None;

  m_ConeAngle.Configure(this, ezEngineGizmoHandleType::Cone, ezColorLinearUB(200, 200, 0, 128), false);
  m_ConeRadius.Configure(this, ezEngineGizmoHandleType::Cone, ezColorLinearUB(200, 200, 200, 128), false, true); // this gizmo should be rendered very last so it is always on top

  SetVisible(false);
  SetTransformation(ezMat4::IdentityMatrix());
}

void ezConeGizmo::OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView)
{
  m_ConeAngle.SetOwner(pOwnerWindow);
  m_ConeRadius.SetOwner(pOwnerWindow);
}

void ezConeGizmo::OnVisibleChanged(bool bVisible)
{
  m_ConeAngle.SetVisible(bVisible);
  m_ConeRadius.SetVisible(m_bEnableRadiusHandle && bVisible);
}

void ezConeGizmo::OnTransformationChanged(const ezMat4& transform)
{
  ezMat4 mScaleAngle, mScaleRadius;

  mScaleAngle.SetScalingMatrix(ezVec3(1.0f, m_fAngleScale, m_fAngleScale) * m_fRadius);
  mScaleRadius.SetScalingMatrix(ezVec3(1.0f, m_fRadiusScale, m_fRadiusScale) * m_fRadius);

  m_ConeAngle.SetTransformation(transform * mScaleAngle);
  m_ConeRadius.SetTransformation(transform * mScaleRadius);
  m_ConeRadius.SetVisible(m_bEnableRadiusHandle);
}

void ezConeGizmo::FocusLost(bool bCancel)
{
  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = bCancel ? ezGizmoEvent::Type::CancelInteractions : ezGizmoEvent::Type::EndInteractions;
  m_GizmoEvents.Broadcast(ev);

  ezViewHighlightMsgToEngine msg;
  msg.SendHighlightObjectMessage(GetOwnerWindow()->GetEditorEngineConnection());

  m_ConeAngle.SetVisible(true);
  m_ConeRadius.SetVisible(m_bEnableRadiusHandle);

  m_ManipulateMode = ManipulateMode::None;
}

ezEditorInut ezConeGizmo::doMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return ezEditorInut::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return ezEditorInut::MayBeHandledByOthers;

  if (m_pInteractionGizmoHandle == &m_ConeAngle)
  {
    m_ManipulateMode = ManipulateMode::Angle;
  }
  else if (m_pInteractionGizmoHandle == &m_ConeRadius)
  {
    m_ManipulateMode = ManipulateMode::Radius;
  }
  else
    return ezEditorInut::MayBeHandledByOthers;

  ezViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  msg.SendHighlightObjectMessage(GetOwnerWindow()->GetEditorEngineConnection());

  m_LastInteraction = ezTime::Now();

  m_MousePos = ezVec2(e->globalPos().x(), e->globalPos().y());

  SetActiveInputContext(this);

  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = ezGizmoEvent::Type::BeginInteractions;
  m_GizmoEvents.Broadcast(ev);

  return ezEditorInut::WasExclusivelyHandled;
}

ezEditorInut ezConeGizmo::doMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInut::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return ezEditorInut::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return ezEditorInut::WasExclusivelyHandled;
}

ezEditorInut ezConeGizmo::doMouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInut::MayBeHandledByOthers;

  const ezTime tNow = ezTime::Now();

  if (tNow - m_LastInteraction < ezTime::Seconds(1.0 / 25.0))
    return ezEditorInut::WasExclusivelyHandled;

  m_LastInteraction = tNow;

  const ezVec2 vNewMousePos = ezVec2(e->globalPos().x(), e->globalPos().y());
  ezVec2 vDiff = vNewMousePos - m_MousePos;

  QCursor::setPos(QPoint(m_MousePos.x, m_MousePos.y));

  const float fSpeed = 0.02f;
  const ezAngle aSpeed = ezAngle::Degree(1.0f);

  if (m_ManipulateMode == ManipulateMode::Radius)
  {
    m_fRadius += vDiff.x * fSpeed;
    m_fRadius -= vDiff.y * fSpeed;

    m_fRadius = ezMath::Max(0.0f, m_fRadius);
  }
  else
  {
    m_Angle += vDiff.x * aSpeed;
    m_Angle -= vDiff.y * aSpeed;

    m_Angle = ezMath::Clamp(m_Angle, ezAngle(), ezAngle::Degree(179.0f));

    m_fAngleScale = ezMath::Tan(m_Angle * 0.5f);
    m_fRadiusScale = ezMath::Tan(ezMath::Min(ezAngle::Degree(20), m_Angle / 3.0f) * 0.5f);
  }

  // update the scale
  OnTransformationChanged(GetTransformation());

  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = ezGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  return ezEditorInut::WasExclusivelyHandled;
}

void ezConeGizmo::SetRadius(float fRadius)
{
  m_fRadius = fRadius;
  
  // update the scale
  OnTransformationChanged(GetTransformation());
}

void ezConeGizmo::SetAngle(ezAngle angle)
{
  m_Angle = angle;
  m_fAngleScale = ezMath::Tan(m_Angle * 0.5f);
  m_fRadiusScale = ezMath::Tan(ezMath::Min(ezAngle::Degree(20), m_Angle / 3.0f) * 0.5f);

  // update the scale
  OnTransformationChanged(GetTransformation());
}

