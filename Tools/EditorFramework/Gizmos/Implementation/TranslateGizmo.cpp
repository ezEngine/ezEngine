#include <PCH.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>
#include <EditorFramework/DocumentWindow3D/DocumentWindow3D.moc.h>
#include <Foundation/Logging/Log.h>
#include <QMouseEvent>
#include <CoreUtils/Graphics/Camera.h>
#include <Foundation/Utilities/GraphicsUtils.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTranslateGizmo, ezGizmoBase, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezTranslateGizmo::ezTranslateGizmo()
{
  m_AxisX.Configure(this, ezGizmoHandleType::Arrow, ezColorLinearUB(128, 0, 0));
  m_AxisY.Configure(this, ezGizmoHandleType::Arrow, ezColorLinearUB(0, 128, 0));
  m_AxisZ.Configure(this, ezGizmoHandleType::Arrow, ezColorLinearUB(0, 0, 128));

  m_PlaneXY.Configure(this, ezGizmoHandleType::Rect, ezColorLinearUB(128, 128, 255));
  m_PlaneXZ.Configure(this, ezGizmoHandleType::Rect, ezColorLinearUB(128, 255, 128));
  m_PlaneYZ.Configure(this, ezGizmoHandleType::Rect, ezColorLinearUB(255, 128, 128));

  SetVisible(false);
  SetTransformation(ezMat4::IdentityMatrix());

  m_Mode = TranslateMode::None;
}

void ezTranslateGizmo::SetDocumentGuid(const ezUuid& guid)
{
  m_AxisX.SetDocumentGuid(guid);
  m_AxisY.SetDocumentGuid(guid);
  m_AxisZ.SetDocumentGuid(guid);

  m_PlaneXY.SetDocumentGuid(guid);
  m_PlaneXZ.SetDocumentGuid(guid);
  m_PlaneYZ.SetDocumentGuid(guid);
}

void ezTranslateGizmo::OnVisibleChanged(bool bVisible)
{
  m_AxisX.SetVisible(bVisible);
  m_AxisY.SetVisible(bVisible);
  m_AxisZ.SetVisible(bVisible);

  m_PlaneXY.SetVisible(bVisible);
  m_PlaneXZ.SetVisible(bVisible);
  m_PlaneYZ.SetVisible(bVisible);
}

void ezTranslateGizmo::OnTransformationChanged(const ezMat4& transform)
{
  ezMat4 m;

  m.SetRotationMatrixZ(ezAngle::Degree(-90));
  m_AxisX.SetTransformation(transform * m);

  m.SetIdentity();
  m_AxisY.SetTransformation(transform * m);

  m.SetRotationMatrixX(ezAngle::Degree(90));
  m_AxisZ.SetTransformation(transform * m);

  m.SetIdentity();
  m_PlaneXY.SetTransformation(transform * m);

  m.SetRotationMatrixY(ezAngle::Degree(90));
  m_PlaneYZ.SetTransformation(transform * m);

  m.SetRotationMatrixX(ezAngle::Degree(90));
  m_PlaneXZ.SetTransformation(transform * m);
}

void ezTranslateGizmo::FocusLost()
{
  BaseEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = BaseEvent::Type::EndInteractions;
  m_BaseEvents.Broadcast(ev);

  ezViewHighlightMsgToEngine msg;
  msg.SendHighlightObjectMessage(GetDocumentWindow3D()->GetEditorEngineConnection());

  m_AxisX.SetVisible(true);
  m_AxisY.SetVisible(true);
  m_AxisZ.SetVisible(true);

  m_PlaneXY.SetVisible(true);
  m_PlaneXZ.SetVisible(true);
  m_PlaneYZ.SetVisible(true);

  m_Mode = TranslateMode::None;
}

bool ezTranslateGizmo::mousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return true;

  if (e->button() != Qt::MouseButton::LeftButton)
    return false;

  if (m_pInteractionGizmoHandle == &m_AxisX)
  {
    m_vMoveAxis = m_AxisX.GetTransformation().GetColumn(1).GetAsVec3().GetNormalized();
    m_Mode = TranslateMode::Axis;
  }
  else if (m_pInteractionGizmoHandle == &m_AxisY)
  {
    m_vMoveAxis = m_AxisY.GetTransformation().GetColumn(1).GetAsVec3().GetNormalized();
    m_Mode = TranslateMode::Axis;
  }
  else if (m_pInteractionGizmoHandle == &m_AxisZ)
  {
    m_vMoveAxis = m_AxisZ.GetTransformation().GetColumn(1).GetAsVec3().GetNormalized();
    m_Mode = TranslateMode::Axis;
  }
  else if (m_pInteractionGizmoHandle == &m_PlaneXY)
  {
    m_vMoveAxis = m_PlaneXY.GetTransformation().GetColumn(2).GetAsVec3().GetNormalized();
    m_Mode = TranslateMode::Plane;
  }
  else if (m_pInteractionGizmoHandle == &m_PlaneXZ)
  {
    m_vMoveAxis = m_PlaneXZ.GetTransformation().GetColumn(2).GetAsVec3().GetNormalized();
    m_Mode = TranslateMode::Plane;
  }
  else if (m_pInteractionGizmoHandle == &m_PlaneYZ)
  {
    m_vMoveAxis = m_PlaneYZ.GetTransformation().GetColumn(2).GetAsVec3().GetNormalized();
    m_Mode = TranslateMode::Plane;
  }
  else
    return false;

  ezViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  msg.SendHighlightObjectMessage(GetDocumentWindow3D()->GetEditorEngineConnection());

  m_AxisX.SetVisible(false);
  m_AxisY.SetVisible(false);
  m_AxisZ.SetVisible(false);
  m_PlaneXY.SetVisible(false);
  m_PlaneXZ.SetVisible(false);
  m_PlaneYZ.SetVisible(false);

  m_pInteractionGizmoHandle->SetVisible(true);

  m_vStartPosition = GetTransformation().GetTranslationVector();

  ezMat4 mView, mProj, mViewProj;
  m_pCamera->GetViewMatrix(mView);
  m_pCamera->GetProjectionMatrix((float)m_Viewport.x / (float)m_Viewport.y, mProj);
  mViewProj = mProj * mView;
  m_InvViewProj = mViewProj.GetInverse();


  m_LastInteraction = ezTime::Now();

  SetActiveInputContext(this);

  if (m_Mode == TranslateMode::Axis)
  {
    GetPointOnAxis(e->pos().x(), m_Viewport.y - e->pos().y(), m_vInteractionPivot);
  }
  else if (m_Mode == TranslateMode::Plane)
  {
    GetPointOnPlane(e->pos().x(), m_Viewport.y - e->pos().y(), m_vInteractionPivot);
  }

  m_fStartScale = (m_vInteractionPivot - m_pCamera->GetPosition()).GetLength() * 0.125;

  BaseEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = BaseEvent::Type::BeginInteractions;
  m_BaseEvents.Broadcast(ev);

  return true;
}

bool ezTranslateGizmo::mouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return false;

  if (e->button() != Qt::MouseButton::LeftButton)
    return true;

  FocusLost();

  SetActiveInputContext(nullptr);
  return true;
}

ezResult ezTranslateGizmo::GetPointOnPlane(ezInt32 iScreenPosX, ezInt32 iScreenPosY, ezVec3& out_Result) const
{
  out_Result = m_vStartPosition;

  ezVec3 vPos, vRayDir;
  if (ezGraphicsUtils::ConvertScreenPosToWorldPos(m_InvViewProj, 0, 0, m_Viewport.x, m_Viewport.y, ezVec3(iScreenPosX, iScreenPosY, 0), vPos, &vRayDir).Failed())
    return EZ_FAILURE;

  ezPlane Plane;
  Plane.SetFromNormalAndPoint(m_vMoveAxis, m_vStartPosition);

  ezVec3 vIntersection;
  if (!Plane.GetRayIntersection(m_pCamera->GetPosition(), vRayDir, nullptr, &vIntersection))
    return EZ_FAILURE;

  out_Result = vIntersection;
  return EZ_SUCCESS;
}

ezResult ezTranslateGizmo::GetPointOnAxis(ezInt32 iScreenPosX, ezInt32 iScreenPosY, ezVec3& out_Result) const
{
  out_Result = m_vStartPosition;

  ezVec3 vPos, vRayDir;
  if (ezGraphicsUtils::ConvertScreenPosToWorldPos(m_InvViewProj, 0, 0, m_Viewport.x, m_Viewport.y, ezVec3(iScreenPosX, iScreenPosY, 0), vPos, &vRayDir).Failed())
    return EZ_FAILURE;

  const ezVec3 vPlaneTangent = m_vMoveAxis.Cross(m_pCamera->GetDirForwards()).GetNormalized();
  const ezVec3 vPlaneNormal = m_vMoveAxis.Cross(vPlaneTangent);

  ezPlane Plane;
  Plane.SetFromNormalAndPoint(vPlaneNormal, m_vStartPosition);

  ezVec3 vIntersection;
  if (!Plane.GetRayIntersection(m_pCamera->GetPosition(), vRayDir, nullptr, &vIntersection))
    return EZ_FAILURE;

  const ezVec3 vDirAlongRay = vIntersection - m_vStartPosition;
  const float fProjectedLength = vDirAlongRay.Dot(m_vMoveAxis);

  out_Result = m_vStartPosition + fProjectedLength * m_vMoveAxis;
  return EZ_SUCCESS;
}

bool ezTranslateGizmo::mouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return false;

  const ezTime tNow = ezTime::Now();

  if (tNow - m_LastInteraction < ezTime::Seconds(1.0 / 30.0))
    return true;

  m_LastInteraction = tNow;

  ezVec3 vCurrentInteractionPoint;
  
  if (m_Mode == TranslateMode::Axis)
  {
    if (GetPointOnAxis(e->pos().x(), m_Viewport.y - e->pos().y(), vCurrentInteractionPoint).Failed())
      return true;
  }
  else if (m_Mode == TranslateMode::Plane)
  {
     if (GetPointOnPlane(e->pos().x(), m_Viewport.y - e->pos().y(), vCurrentInteractionPoint).Failed())
      return true;
  }


  const float fPerspectiveScale = (vCurrentInteractionPoint - m_pCamera->GetPosition()).GetLength() * 0.125;
  const ezVec3 vOffset = (m_vInteractionPivot - m_vStartPosition);

  ezMat4 mTrans = GetTransformation();
  mTrans.SetTranslationVector(vCurrentInteractionPoint - vOffset * fPerspectiveScale / m_fStartScale);

  SetTransformation(mTrans);

  BaseEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = BaseEvent::Type::Interaction;
  m_BaseEvents.Broadcast(ev);

  return true;
}

