#include <PCH.h>
#include <EditorFramework/Gizmos/RotateGizmo.h>
#include <EditorFramework/DocumentWindow3D/DocumentWindow3D.moc.h>
#include <Foundation/Logging/Log.h>
#include <QMouseEvent>
#include <CoreUtils/Graphics/Camera.h>
#include <Foundation/Utilities/GraphicsUtils.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRotateGizmo, ezGizmoBase, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezRotateGizmo::ezRotateGizmo()
{
  m_AxisX.Configure(this, ezGizmoHandleType::Ring, ezColorLinearUB(128, 0, 0));
  m_AxisY.Configure(this, ezGizmoHandleType::Ring, ezColorLinearUB(0, 128, 0));
  m_AxisZ.Configure(this, ezGizmoHandleType::Ring, ezColorLinearUB(0, 0, 128));

  SetVisible(false);
  SetTransformation(ezMat4::IdentityMatrix());

  m_SnappingAngle = ezAngle();
}

void ezRotateGizmo::OnSetOwner(ezDocumentWindow3D* pOwnerWindow, ezEngineViewWidget* pOwnerView)
{
  m_AxisX.SetOwner(pOwnerWindow);
  m_AxisY.SetOwner(pOwnerWindow);
  m_AxisZ.SetOwner(pOwnerWindow);
}

void ezRotateGizmo::OnVisibleChanged(bool bVisible)
{
  m_AxisX.SetVisible(bVisible);
  m_AxisY.SetVisible(bVisible);
  m_AxisZ.SetVisible(bVisible);
}

void ezRotateGizmo::OnTransformationChanged(const ezMat4& transform)
{
  ezMat4 m;

  m.SetRotationMatrixZ(ezAngle::Degree(-90));
  m_AxisX.SetTransformation(transform * m);

  m.SetIdentity();
  m_AxisY.SetTransformation(transform * m);

  m.SetRotationMatrixX(ezAngle::Degree(90));
  m_AxisZ.SetTransformation(transform * m);
}

void ezRotateGizmo::FocusLost()
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

  QApplication::restoreOverrideCursor();
}

bool ezRotateGizmo::mousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return true;

  if (e->button() != Qt::MouseButton::LeftButton)
    return false;

  if (m_pInteractionGizmoHandle == &m_AxisX)
  {
    m_vMoveAxis = m_AxisX.GetTransformation().GetColumn(1).GetAsVec3().GetNormalized();
  }
  else if (m_pInteractionGizmoHandle == &m_AxisY)
  {
    m_vMoveAxis = m_AxisY.GetTransformation().GetColumn(1).GetAsVec3().GetNormalized();
  }
  else if (m_pInteractionGizmoHandle == &m_AxisZ)
  {
    m_vMoveAxis = m_AxisZ.GetTransformation().GetColumn(1).GetAsVec3().GetNormalized();
  }
  else
    return false;

  // Determine on which side of the gizmo the camera is located
  // if it is on the wrong side, flip the rotation axis, so that the mouse move direction matches the rotation direction better
  {
    ezPlane p;
    p.SetFromNormalAndPoint(m_vMoveAxis, GetTransformation().GetTranslationVector());

    if (p.GetPointPosition(m_pCamera->GetPosition()) == ezPositionOnPlane::Front)
      m_vMoveAxis = -m_vMoveAxis;
  }

  ezViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  msg.SendHighlightObjectMessage(GetOwnerWindow()->GetEditorEngineConnection());

  QApplication::setOverrideCursor(QCursor(Qt::BlankCursor));

  m_MousePos = ezVec2(e->globalPos().x(), e->globalPos().y());
  m_Rotation = ezAngle();

  m_AxisX.SetVisible(false);
  m_AxisY.SetVisible(false);
  m_AxisZ.SetVisible(false);

  m_pInteractionGizmoHandle->SetVisible(true);

  m_StartRotation.SetFromMat3(GetTransformation().GetRotationalPart());

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

bool ezRotateGizmo::mouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return false;

  if (e->button() != Qt::MouseButton::LeftButton)
    return true;

  FocusLost();

  SetActiveInputContext(nullptr);
  return true;
}

bool ezRotateGizmo::mouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return false;

  const ezTime tNow = ezTime::Now();

  if (tNow - m_LastInteraction < ezTime::Seconds(1.0 / 25.0))
    return true;

  m_LastInteraction = tNow;

  const ezVec2 vNewMousePos = ezVec2(e->globalPos().x(), e->globalPos().y());
  ezVec2 vDiff = vNewMousePos - m_MousePos;

  QCursor::setPos(QPoint(m_MousePos.x, m_MousePos.y));

  m_Rotation += ezAngle::Degree(vDiff.x);
  m_Rotation -= ezAngle::Degree(vDiff.y);

  ezAngle rot = m_Rotation;

  if (m_SnappingAngle.GetRadian() != 0.0f)
    rot = ezAngle::Radian(ezMath::Round(m_Rotation.GetRadian(), m_SnappingAngle.GetRadian()));

  m_CurrentRotation.SetFromAxisAndAngle(m_vMoveAxis, rot);

  ezMat4 mTrans = GetTransformation();
  mTrans.SetRotationalPart((m_CurrentRotation * m_StartRotation).GetAsMat3());

  SetTransformation(mTrans);

  BaseEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = BaseEvent::Type::Interaction;
  m_BaseEvents.Broadcast(ev);

  return true;
}

