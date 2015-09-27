#include <PCH.h>
#include <EditorFramework/Gizmos/ScaleGizmo.h>
#include <EditorFramework/DocumentWindow3D/DocumentWindow3D.moc.h>
#include <Foundation/Logging/Log.h>
#include <QMouseEvent>
#include <CoreUtils/Graphics/Camera.h>
#include <Foundation/Utilities/GraphicsUtils.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezScaleGizmo, ezGizmoBase, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezScaleGizmo::ezScaleGizmo()
{
  m_AxisX.Configure(this, ezGizmoHandleType::Piston, ezColorLinearUB(128, 0, 0));
  m_AxisY.Configure(this, ezGizmoHandleType::Piston, ezColorLinearUB(0, 128, 0));
  m_AxisZ.Configure(this, ezGizmoHandleType::Piston, ezColorLinearUB(0, 0, 128));
  m_AxisXYZ.Configure(this, ezGizmoHandleType::Box, ezColorLinearUB(128, 128, 0));

  SetVisible(false);
  SetTransformation(ezMat4::IdentityMatrix());

  m_fSnappingValue = 0.0f;
}

void ezScaleGizmo::OnSetOwner(ezDocumentWindow3D* pOwnerWindow, ezEngineViewWidget* pOwnerView)
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
  ezMat4 m;

  m.SetIdentity();
  m_AxisX.SetTransformation(transform * m);

  m.SetRotationMatrixZ(ezAngle::Degree(90));
  m_AxisY.SetTransformation(transform * m);

  m.SetRotationMatrixY(ezAngle::Degree(-90));
  m_AxisZ.SetTransformation(transform * m);

  m.SetIdentity();
  m_AxisXYZ.SetTransformation(transform * m);
}

void ezScaleGizmo::FocusLost()
{
  BaseEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = BaseEvent::Type::EndInteractions;
  m_BaseEvents.Broadcast(ev);

  ezViewHighlightMsgToEngine msg;
  msg.SendHighlightObjectMessage(GetOwnerWindow()->GetEditorEngineConnection());

  m_AxisX.SetVisible(true);
  m_AxisY.SetVisible(true);
  m_AxisZ.SetVisible(true);
  m_AxisXYZ.SetVisible(true);

  QApplication::restoreOverrideCursor();
}

bool ezScaleGizmo::mousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return true;

  if (e->button() != Qt::MouseButton::LeftButton)
    return false;

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
    return false;

  ezViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  msg.SendHighlightObjectMessage(GetOwnerWindow()->GetEditorEngineConnection());

  QApplication::setOverrideCursor(QCursor(Qt::BlankCursor));

  m_MousePos = ezVec2(e->globalPos().x(), e->globalPos().y());

  m_AxisX.SetVisible(false);
  m_AxisY.SetVisible(false);
  m_AxisZ.SetVisible(false);
  m_AxisXYZ.SetVisible(false);

  m_pInteractionGizmoHandle->SetVisible(true);

  m_vScalingResult.Set(1.0f);
  m_vScaleMouseMove.SetZero();

  ezMat4 mView, mProj, mViewProj;
  m_pCamera->GetViewMatrix(mView);
  m_pCamera->GetProjectionMatrix((float)m_Viewport.x / (float)m_Viewport.y, mProj);
  mViewProj = mProj * mView;
  m_InvViewProj = mViewProj.GetInverse();

  m_LastInteraction = ezTime::Now();

  SetActiveInputContext(this);

  BaseEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = BaseEvent::Type::BeginInteractions;
  m_BaseEvents.Broadcast(ev);

  return true;
}

bool ezScaleGizmo::mouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return false;

  if (e->button() != Qt::MouseButton::LeftButton)
    return true;

  FocusLost();

  SetActiveInputContext(nullptr);
  return true;
}

bool ezScaleGizmo::mouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return false;

  const ezTime tNow = ezTime::Now();

  if (tNow - m_LastInteraction < ezTime::Seconds(1.0 / 25.0))
    return true;

  m_LastInteraction = tNow;

  const ezVec2 vNewMousePos = ezVec2(e->globalPos().x(), e->globalPos().y());
  ezVec2 vDiff = (vNewMousePos - m_MousePos);

  QCursor::setPos(QPoint(m_MousePos.x, m_MousePos.y));

  float fFactor = m_fSnappingValue != 0.0f ? m_fSnappingValue : 1.0f;

  m_vScaleMouseMove += m_vMoveAxis * vDiff.x * fFactor;
  m_vScaleMouseMove -= m_vMoveAxis * vDiff.y * fFactor;

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

  BaseEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = BaseEvent::Type::Interaction;
  m_BaseEvents.Broadcast(ev);

  return true;
}

