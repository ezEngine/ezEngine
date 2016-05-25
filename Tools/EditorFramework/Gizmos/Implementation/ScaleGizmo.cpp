#include <PCH.h>
#include <EditorFramework/Gizmos/ScaleGizmo.h>
#include <EditorFramework/DocumentWindow3D/DocumentWindow3D.moc.h>
#include <Foundation/Logging/Log.h>
#include <QMouseEvent>
#include <CoreUtils/Graphics/Camera.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <EditorFramework/Gizmos/SnapProvider.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezScaleGizmo, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezScaleGizmo::ezScaleGizmo()
{
  m_AxisX.Configure(this, ezEngineGizmoHandleType::Piston, ezColorLinearUB(128, 0, 0));
  m_AxisY.Configure(this, ezEngineGizmoHandleType::Piston, ezColorLinearUB(0, 128, 0));
  m_AxisZ.Configure(this, ezEngineGizmoHandleType::Piston, ezColorLinearUB(0, 0, 128));
  m_AxisXYZ.Configure(this, ezEngineGizmoHandleType::Box, ezColorLinearUB(128, 128, 0));

  SetVisible(false);
  SetTransformation(ezMat4::IdentityMatrix());
}

void ezScaleGizmo::OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView)
{
  m_AxisX.SetOwner(pOwnerWindow);
  m_AxisY.SetOwner(pOwnerWindow);
  m_AxisZ.SetOwner(pOwnerWindow);
  m_AxisXYZ.SetOwner(pOwnerWindow);
}

void ezScaleGizmo::OnVisibleChanged(bool bVisible)
{
  m_AxisX.SetVisible(bVisible);
  m_AxisY.SetVisible(bVisible);
  m_AxisZ.SetVisible(bVisible);
  m_AxisXYZ.SetVisible(bVisible);
}

void ezScaleGizmo::OnTransformationChanged(const ezMat4& transform)
{
  ezMat4 m, s;

  s.SetScalingMatrix(ezVec3(0.2f));

  m.SetIdentity();
  m_AxisX.SetTransformation(transform * m);

  m.SetRotationMatrixZ(ezAngle::Degree(90));
  m_AxisY.SetTransformation(transform * m);

  m.SetRotationMatrixY(ezAngle::Degree(-90));
  m_AxisZ.SetTransformation(transform * m);

  m.SetIdentity();
  m_AxisXYZ.SetTransformation(transform * s * m);
}

void ezScaleGizmo::DoFocusLost(bool bCancel)
{
  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = bCancel ? ezGizmoEvent::Type::CancelInteractions : ezGizmoEvent::Type::EndInteractions;
  m_GizmoEvents.Broadcast(ev);

  ezViewHighlightMsgToEngine msg;
  msg.SendHighlightObjectMessage(GetOwnerWindow()->GetEditorEngineConnection());

  m_AxisX.SetVisible(true);
  m_AxisY.SetVisible(true);
  m_AxisZ.SetVisible(true);
  m_AxisXYZ.SetVisible(true);

  QApplication::restoreOverrideCursor();
}

ezEditorInut ezScaleGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return ezEditorInut::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return ezEditorInut::MayBeHandledByOthers;

  if (m_pInteractionGizmoHandle == &m_AxisX)
  {
    m_vMoveAxis.Set(1, 0, 0);
  }
  else if (m_pInteractionGizmoHandle == &m_AxisY)
  {
    m_vMoveAxis.Set(0, 1, 0);
  }
  else if (m_pInteractionGizmoHandle == &m_AxisZ)
  {
    m_vMoveAxis.Set(0, 0, 1);
  }
  else if (m_pInteractionGizmoHandle == &m_AxisXYZ)
  {
    m_vMoveAxis.Set(1, 1, 1);
  }
  else
    return ezEditorInut::MayBeHandledByOthers;

  ezViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  msg.SendHighlightObjectMessage(GetOwnerWindow()->GetEditorEngineConnection());

  m_LastMousePos = SetMouseMode(ezEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);

  //m_AxisX.SetVisible(false);
  //m_AxisY.SetVisible(false);
  //m_AxisZ.SetVisible(false);
  //m_AxisXYZ.SetVisible(false);

  //m_pInteractionGizmoHandle->SetVisible(true);

  m_vScalingResult.Set(1.0f);
  m_vScaleMouseMove.SetZero();

  ezMat4 mView, mProj, mViewProj;
  m_pCamera->GetViewMatrix(mView);
  m_pCamera->GetProjectionMatrix((float)m_Viewport.x / (float)m_Viewport.y, mProj);
  mViewProj = mProj * mView;
  m_InvViewProj = mViewProj.GetInverse();

  m_LastInteraction = ezTime::Now();

  SetActiveInputContext(this);

  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = ezGizmoEvent::Type::BeginInteractions;
  m_GizmoEvents.Broadcast(ev);

  return ezEditorInut::WasExclusivelyHandled;
}

ezEditorInut ezScaleGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInut::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return ezEditorInut::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return ezEditorInut::WasExclusivelyHandled;
}

ezEditorInut ezScaleGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInut::MayBeHandledByOthers;

  const ezTime tNow = ezTime::Now();

  if (tNow - m_LastInteraction < ezTime::Seconds(1.0 / 25.0))
    return ezEditorInut::WasExclusivelyHandled;

  m_LastInteraction = tNow;

  const ezVec2I32 vNewMousePos = ezVec2I32(e->globalPos().x(), e->globalPos().y());
  ezVec2I32 vDiff = (vNewMousePos - m_LastMousePos);

  m_LastMousePos = UpdateMouseMode(e);

  m_vScaleMouseMove += m_vMoveAxis * (float)vDiff.x;
  m_vScaleMouseMove -= m_vMoveAxis * (float)vDiff.y;

  m_vScalingResult.Set(1.0f);

  const float fScaleSpeed = 0.01f;

  if (m_vScaleMouseMove.x > 0.0f)
    m_vScalingResult.x = 1.0f + m_vScaleMouseMove.x * fScaleSpeed;
  if (m_vScaleMouseMove.x < 0.0f)
    m_vScalingResult.x = 1.0f / (1.0f - m_vScaleMouseMove.x * fScaleSpeed);

  if (m_vScaleMouseMove.y > 0.0f)
    m_vScalingResult.y = 1.0f + m_vScaleMouseMove.y * fScaleSpeed;
  if (m_vScaleMouseMove.y < 0.0f)
    m_vScalingResult.y = 1.0f / (1.0f - m_vScaleMouseMove.y * fScaleSpeed);

  if (m_vScaleMouseMove.z > 0.0f)
    m_vScalingResult.z = 1.0f + m_vScaleMouseMove.z * fScaleSpeed;
  if (m_vScaleMouseMove.z < 0.0f)
    m_vScalingResult.z = 1.0f / (1.0f - m_vScaleMouseMove.z * fScaleSpeed);

  ezSnapProvider::SnapScale(m_vScalingResult);

  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = ezGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  return ezEditorInut::WasExclusivelyHandled;
}

