#include <PCH.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <Foundation/Logging/Log.h>
#include <Core/Graphics/Camera.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <QMouseEvent>
#include <QDesktopWidget>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTranslateGizmo, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezTranslateGizmo::ezTranslateGizmo()
{
  m_vStartPosition.SetZero();
  m_fCameraSpeed = 0.2f;

  m_AxisX.Configure(this, ezEngineGizmoHandleType::Arrow, ezColorLinearUB(128, 0, 0));
  m_AxisY.Configure(this, ezEngineGizmoHandleType::Arrow, ezColorLinearUB(0, 128, 0));
  m_AxisZ.Configure(this, ezEngineGizmoHandleType::Arrow, ezColorLinearUB(0, 0, 128));

  m_PlaneXY.Configure(this, ezEngineGizmoHandleType::Rect, ezColorLinearUB(128, 128, 255));
  m_PlaneXZ.Configure(this, ezEngineGizmoHandleType::Rect, ezColorLinearUB(128, 255, 128));
  m_PlaneYZ.Configure(this, ezEngineGizmoHandleType::Rect, ezColorLinearUB(255, 128, 128));

  SetVisible(false);
  SetTransformation(ezMat4::IdentityMatrix());

  m_Mode = TranslateMode::None;
  m_MovementMode = MovementMode::ScreenProjection;
  m_LastPlaneInteraction = PlaneInteraction::PlaneZ;
}

void ezTranslateGizmo::OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView)
{
  m_AxisX.SetOwner(pOwnerWindow->GetDocument());
  m_AxisY.SetOwner(pOwnerWindow->GetDocument());
  m_AxisZ.SetOwner(pOwnerWindow->GetDocument());

  m_PlaneXY.SetOwner(pOwnerWindow->GetDocument());
  m_PlaneXZ.SetOwner(pOwnerWindow->GetDocument());
  m_PlaneYZ.SetOwner(pOwnerWindow->GetDocument());
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

  m.SetIdentity();
  m_AxisX.SetTransformation(transform * m);

  m.SetRotationMatrixZ(ezAngle::Degree(90));
  m_AxisY.SetTransformation(transform * m);

  m.SetRotationMatrixY(ezAngle::Degree(-90));
  m_AxisZ.SetTransformation(transform * m);

  m.SetIdentity();
  m_PlaneXY.SetTransformation(transform * m);

  m.SetRotationMatrixY(ezAngle::Degree(90));
  m_PlaneYZ.SetTransformation(transform * m);

  m.SetRotationMatrixX(ezAngle::Degree(90));
  m_PlaneXZ.SetTransformation(transform * m);

  if (!IsActiveInputContext())
  {
    // if the gizmo is currently not being dragged, copy the translation into the start position
    m_vStartPosition = GetTransformation().GetTranslationVector();
  }
}

void ezTranslateGizmo::DoFocusLost(bool bCancel)
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

  m_PlaneXY.SetVisible(true);
  m_PlaneXZ.SetVisible(true);
  m_PlaneYZ.SetVisible(true);

  m_Mode = TranslateMode::None;
  m_MovementMode = MovementMode::ScreenProjection;
  m_vLastMoveDiff.SetZero();

  m_vStartPosition = GetTransformation().GetTranslationVector();
}

ezEditorInut ezTranslateGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return ezEditorInut::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return ezEditorInut::MayBeHandledByOthers;

  m_vLastMoveDiff.SetZero();

  if (m_pInteractionGizmoHandle == &m_AxisX)
  {
    m_vMoveAxis = m_AxisX.GetTransformation().GetColumn(0).GetAsVec3().GetNormalized();
    m_Mode = TranslateMode::Axis;
  }
  else if (m_pInteractionGizmoHandle == &m_AxisY)
  {
    m_vMoveAxis = m_AxisY.GetTransformation().GetColumn(0).GetAsVec3().GetNormalized();
    m_Mode = TranslateMode::Axis;
  }
  else if (m_pInteractionGizmoHandle == &m_AxisZ)
  {
    m_vMoveAxis = m_AxisZ.GetTransformation().GetColumn(0).GetAsVec3().GetNormalized();
    m_Mode = TranslateMode::Axis;
  }
  else if (m_pInteractionGizmoHandle == &m_PlaneXY)
  {
    m_vMoveAxis = m_PlaneXY.GetTransformation().GetColumn(2).GetAsVec3().GetNormalized();
    m_vPlaneAxis[0] = m_PlaneXY.GetTransformation().GetColumn(0).GetAsVec3().GetNormalized();
    m_vPlaneAxis[1] = m_PlaneXY.GetTransformation().GetColumn(1).GetAsVec3().GetNormalized();
    m_Mode = TranslateMode::Plane;
    m_LastPlaneInteraction = PlaneInteraction::PlaneZ;
  }
  else if (m_pInteractionGizmoHandle == &m_PlaneXZ)
  {
    m_vMoveAxis = m_PlaneXZ.GetTransformation().GetColumn(2).GetAsVec3().GetNormalized();
    m_vPlaneAxis[0] = m_PlaneXZ.GetTransformation().GetColumn(0).GetAsVec3().GetNormalized();
    m_vPlaneAxis[1] = m_PlaneXZ.GetTransformation().GetColumn(1).GetAsVec3().GetNormalized();
    m_Mode = TranslateMode::Plane;
    m_LastPlaneInteraction = PlaneInteraction::PlaneY;
  }
  else if (m_pInteractionGizmoHandle == &m_PlaneYZ)
  {
    m_vMoveAxis = m_PlaneYZ.GetTransformation().GetColumn(2).GetAsVec3().GetNormalized();
    m_vPlaneAxis[0] = m_PlaneYZ.GetTransformation().GetColumn(0).GetAsVec3().GetNormalized();
    m_vPlaneAxis[1] = m_PlaneYZ.GetTransformation().GetColumn(1).GetAsVec3().GetNormalized();
    m_Mode = TranslateMode::Plane;
    m_LastPlaneInteraction = PlaneInteraction::PlaneX;
  }
  else
    return ezEditorInut::MayBeHandledByOthers;

  ezViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  msg.SendHighlightObjectMessage(GetOwnerWindow()->GetEditorEngineConnection());

  m_vStartPosition = GetTransformation().GetTranslationVector();

  ezMat4 mView, mProj, mViewProj;
  m_pCamera->GetViewMatrix(mView);
  m_pCamera->GetProjectionMatrix((float)m_Viewport.x / (float)m_Viewport.y, mProj);
  mViewProj = mProj * mView;
  m_InvViewProj = mViewProj.GetInverse();


  m_LastInteraction = ezTime::Now();

  m_LastMousePos = SetMouseMode(ezEditorInputContext::MouseMode::WrapAtScreenBorders);
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

  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = ezGizmoEvent::Type::BeginInteractions;
  m_GizmoEvents.Broadcast(ev);

  return ezEditorInut::WasExclusivelyHandled;
}

ezEditorInut ezTranslateGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInut::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return ezEditorInut::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return ezEditorInut::WasExclusivelyHandled;
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

ezEditorInut ezTranslateGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInut::MayBeHandledByOthers;

  const ezTime tNow = ezTime::Now();

  if (tNow - m_LastInteraction < ezTime::Seconds(1.0 / 25.0))
    return ezEditorInut::WasExclusivelyHandled;

  const ezVec2I32 CurMousePos(e->globalX(), e->globalY());

  m_LastInteraction = tNow;

  ezMat4 mTrans = GetTransformation();
  ezVec3 vTranslate(0);

  if (m_MovementMode == MovementMode::ScreenProjection)
  {
    ezVec3 vCurrentInteractionPoint;

    if (m_Mode == TranslateMode::Axis)
    {
      if (GetPointOnAxis(e->pos().x(), m_Viewport.y - e->pos().y(), vCurrentInteractionPoint).Failed())
      {
        m_LastMousePos = UpdateMouseMode(e);
        return ezEditorInut::WasExclusivelyHandled;
      }
    }
    else if (m_Mode == TranslateMode::Plane)
    {
      if (GetPointOnPlane(e->pos().x(), m_Viewport.y - e->pos().y(), vCurrentInteractionPoint).Failed())
      {
        m_LastMousePos = UpdateMouseMode(e);
        return ezEditorInut::WasExclusivelyHandled;
      }
    }


    const float fPerspectiveScale = (vCurrentInteractionPoint - m_pCamera->GetPosition()).GetLength() * 0.125;
    const ezVec3 vOffset = (m_vInteractionPivot - m_vStartPosition);

    const ezVec3 vNewPos = vCurrentInteractionPoint - vOffset * fPerspectiveScale / m_fStartScale;

    vTranslate = vNewPos - m_vStartPosition;
  }
  else
  {
    const float fSpeed = m_fCameraSpeed * 0.01f;

    const ezVec3 vMouseDir = m_pCamera->GetDirRight() * (float)(CurMousePos.x - m_LastMousePos.x) + -m_pCamera->GetDirUp() * (float)(CurMousePos.y - m_LastMousePos.y);

    if (m_Mode == TranslateMode::Axis)
    {
      vTranslate = mTrans.GetTranslationVector() - m_vStartPosition + m_vMoveAxis * (m_vMoveAxis.Dot(vMouseDir)) * fSpeed;
    }
    else if (m_Mode == TranslateMode::Plane)
    {
      vTranslate = mTrans.GetTranslationVector() - m_vStartPosition + m_vPlaneAxis[0] * (m_vPlaneAxis[0].Dot(vMouseDir)) * fSpeed + m_vPlaneAxis[1] * (m_vPlaneAxis[1].Dot(vMouseDir)) * fSpeed;
    }
  }

  m_LastMousePos = UpdateMouseMode(e);

  // disable snapping when ALT is pressed
  if (!e->modifiers().testFlag(Qt::AltModifier))
    ezSnapProvider::SnapTranslationInLocalSpace(mTrans.GetRotationalPart(), vTranslate);

  const ezVec3 vLastPos = mTrans.GetTranslationVector();

  mTrans.SetTranslationVector(m_vStartPosition + vTranslate);

  m_vLastMoveDiff = mTrans.GetTranslationVector() - vLastPos;

  SetTransformation(mTrans);

  if (!m_vLastMoveDiff.IsZero())
  {
    ezGizmoEvent ev;
    ev.m_pGizmo = this;
    ev.m_Type = ezGizmoEvent::Type::Interaction;
    m_GizmoEvents.Broadcast(ev);
  }

  return ezEditorInut::WasExclusivelyHandled;
}

void ezTranslateGizmo::SetMovementMode(MovementMode mode)
{
  if (m_MovementMode == mode)
    return;

  m_MovementMode = mode;

  if (m_MovementMode == MovementMode::MouseDiff)
  {
    m_LastMousePos = SetMouseMode(ezEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);
  }
  else
  {
    m_LastMousePos = SetMouseMode(ezEditorInputContext::MouseMode::WrapAtScreenBorders);
  }
}

void ezTranslateGizmo::SetCameraSpeed(float fSpeed)
{
  m_fCameraSpeed = fSpeed;
}

