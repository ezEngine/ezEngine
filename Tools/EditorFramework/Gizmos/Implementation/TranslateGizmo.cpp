#include <PCH.h>

#include <Core/Graphics/Camera.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <QDesktopWidget>
#include <QMouseEvent>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTranslateGizmo, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;

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
  SetTransformation(ezTransform::IdentityTransform());

  m_Mode = TranslateMode::None;
  m_MovementMode = MovementMode::ScreenProjection;
  m_LastPlaneInteraction = PlaneInteraction::PlaneZ;
}

void ezTranslateGizmo::OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_AxisX);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_AxisY);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_AxisZ);

  pOwnerWindow->GetDocument()->AddSyncObject(&m_PlaneXY);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_PlaneXZ);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_PlaneYZ);
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

void ezTranslateGizmo::OnTransformationChanged(const ezTransform& transform)
{
  ezTransform m;
  m.SetIdentity();

  m_AxisX.SetTransformation(transform * m);

  m.m_qRotation.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(90));
  m_AxisY.SetTransformation(transform * m);

  m.m_qRotation.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(-90));
  m_AxisZ.SetTransformation(transform * m);

  m.SetIdentity();
  m_PlaneXY.SetTransformation(transform * m);

  m.m_qRotation.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(90));
  m_PlaneYZ.SetTransformation(transform * m);

  m.m_qRotation.SetFromAxisAndAngle(ezVec3(1, 0, 0), ezAngle::Degree(90));
  m_PlaneXZ.SetTransformation(transform * m);

  if (!IsActiveInputContext())
  {
    // if the gizmo is currently not being dragged, copy the translation into the start position
    m_vStartPosition = GetTransformation().m_vPosition;
  }
}

void ezTranslateGizmo::DoFocusLost(bool bCancel)
{
  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = bCancel ? ezGizmoEvent::Type::CancelInteractions : ezGizmoEvent::Type::EndInteractions;
  m_GizmoEvents.Broadcast(ev);

  ezViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_AxisX.SetVisible(true);
  m_AxisY.SetVisible(true);
  m_AxisZ.SetVisible(true);

  m_PlaneXY.SetVisible(true);
  m_PlaneXZ.SetVisible(true);
  m_PlaneYZ.SetVisible(true);

  m_Mode = TranslateMode::None;
  m_MovementMode = MovementMode::ScreenProjection;
  m_vLastMoveDiff.SetZero();

  m_vStartPosition = GetTransformation().m_vPosition;
  m_TotalMouseDiff.SetZero();
}

ezEditorInput ezTranslateGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return ezEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return ezEditorInput::MayBeHandledByOthers;

  m_vLastMoveDiff.SetZero();

  if (m_pInteractionGizmoHandle == &m_AxisX)
  {
    m_vMoveAxis = m_AxisX.GetTransformation().m_qRotation * ezVec3(1, 0, 0);
    m_Mode = TranslateMode::Axis;
  }
  else if (m_pInteractionGizmoHandle == &m_AxisY)
  {
    m_vMoveAxis = m_AxisY.GetTransformation().m_qRotation * ezVec3(1, 0, 0);
    m_Mode = TranslateMode::Axis;
  }
  else if (m_pInteractionGizmoHandle == &m_AxisZ)
  {
    m_vMoveAxis = m_AxisZ.GetTransformation().m_qRotation * ezVec3(1, 0, 0);
    m_Mode = TranslateMode::Axis;
  }
  else if (m_pInteractionGizmoHandle == &m_PlaneXY)
  {
    m_vMoveAxis = m_PlaneXY.GetTransformation().m_qRotation * ezVec3(0, 0, 1);
    m_vPlaneAxis[0] = m_PlaneXY.GetTransformation().m_qRotation * ezVec3(1, 0, 0);
    m_vPlaneAxis[1] = m_PlaneXY.GetTransformation().m_qRotation * ezVec3(0, 1, 0);
    m_Mode = TranslateMode::Plane;
    m_LastPlaneInteraction = PlaneInteraction::PlaneZ;
  }
  else if (m_pInteractionGizmoHandle == &m_PlaneXZ)
  {
    m_vMoveAxis = m_PlaneXZ.GetTransformation().m_qRotation * ezVec3(0, 0, 1);
    m_vPlaneAxis[0] = m_PlaneXZ.GetTransformation().m_qRotation * ezVec3(1, 0, 0);
    m_vPlaneAxis[1] = m_PlaneXZ.GetTransformation().m_qRotation * ezVec3(0, 1, 0);
    m_Mode = TranslateMode::Plane;
    m_LastPlaneInteraction = PlaneInteraction::PlaneY;
  }
  else if (m_pInteractionGizmoHandle == &m_PlaneYZ)
  {
    m_vMoveAxis = m_PlaneYZ.GetTransformation().m_qRotation * ezVec3(0, 0, 1);
    m_vPlaneAxis[0] = m_PlaneYZ.GetTransformation().m_qRotation * ezVec3(1, 0, 0);
    m_vPlaneAxis[1] = m_PlaneYZ.GetTransformation().m_qRotation * ezVec3(0, 1, 0);
    m_Mode = TranslateMode::Plane;
    m_LastPlaneInteraction = PlaneInteraction::PlaneX;
  }
  else
    return ezEditorInput::MayBeHandledByOthers;

  ezViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_vStartPosition = GetTransformation().m_vPosition;
  m_TotalMouseDiff.SetZero();

  ezMat4 mView = m_pCamera->GetViewMatrix();
  ezMat4 mProj;
  m_pCamera->GetProjectionMatrix((float)m_Viewport.x / (float)m_Viewport.y, mProj);
  ezMat4 mViewProj = mProj * mView;
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

  return ezEditorInput::WasExclusivelyHandled;
}

ezEditorInput ezTranslateGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return ezEditorInput::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return ezEditorInput::WasExclusivelyHandled;
}

ezResult ezTranslateGizmo::GetPointOnPlane(ezInt32 iScreenPosX, ezInt32 iScreenPosY, ezVec3& out_Result) const
{
  out_Result = m_vStartPosition;

  ezVec3 vPos, vRayDir;
  if (ezGraphicsUtils::ConvertScreenPosToWorldPos(m_InvViewProj, 0, 0, m_Viewport.x, m_Viewport.y, ezVec3(iScreenPosX, iScreenPosY, 0),
                                                  vPos, &vRayDir)
          .Failed())
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
  if (ezGraphicsUtils::ConvertScreenPosToWorldPos(m_InvViewProj, 0, 0, m_Viewport.x, m_Viewport.y, ezVec3(iScreenPosX, iScreenPosY, 0),
                                                  vPos, &vRayDir)
          .Failed())
    return EZ_FAILURE;

  const ezVec3 vPlaneTangent = m_vMoveAxis.CrossRH(m_pCamera->GetDirForwards()).GetNormalized();
  const ezVec3 vPlaneNormal = m_vMoveAxis.CrossRH(vPlaneTangent);

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

ezEditorInput ezTranslateGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInput::MayBeHandledByOthers;

  const ezTime tNow = ezTime::Now();

  if (tNow - m_LastInteraction < ezTime::Seconds(1.0 / 25.0))
    return ezEditorInput::WasExclusivelyHandled;

  const ezVec2I32 CurMousePos(e->globalX(), e->globalY());

  m_LastInteraction = tNow;

  ezTransform mTrans = GetTransformation();
  ezVec3 vTranslate(0);

  if (m_MovementMode == MovementMode::ScreenProjection)
  {
    ezVec3 vCurrentInteractionPoint;

    if (m_Mode == TranslateMode::Axis)
    {
      if (GetPointOnAxis(e->pos().x(), m_Viewport.y - e->pos().y(), vCurrentInteractionPoint).Failed())
      {
        m_LastMousePos = UpdateMouseMode(e);
        return ezEditorInput::WasExclusivelyHandled;
      }
    }
    else if (m_Mode == TranslateMode::Plane)
    {
      if (GetPointOnPlane(e->pos().x(), m_Viewport.y - e->pos().y(), vCurrentInteractionPoint).Failed())
      {
        m_LastMousePos = UpdateMouseMode(e);
        return ezEditorInput::WasExclusivelyHandled;
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

    m_TotalMouseDiff += ezVec2((float)(CurMousePos.x - m_LastMousePos.x), (float)(CurMousePos.y - m_LastMousePos.y));
    const ezVec3 vMouseDir = m_pCamera->GetDirRight() * m_TotalMouseDiff.x + -m_pCamera->GetDirUp() * m_TotalMouseDiff.y;

    if (m_Mode == TranslateMode::Axis)
    {
      vTranslate = m_vMoveAxis * (m_vMoveAxis.Dot(vMouseDir)) * fSpeed;
    }
    else if (m_Mode == TranslateMode::Plane)
    {
      vTranslate =
          m_vPlaneAxis[0] * (m_vPlaneAxis[0].Dot(vMouseDir)) * fSpeed + m_vPlaneAxis[1] * (m_vPlaneAxis[1].Dot(vMouseDir)) * fSpeed;
    }
  }

  m_LastMousePos = UpdateMouseMode(e);

  // disable snapping when ALT is pressed
  if (!e->modifiers().testFlag(Qt::AltModifier))
  {
    ezSnapProvider::SnapTranslationInLocalSpace(mTrans.m_qRotation, vTranslate);
  }

  const ezVec3 vLastPos = mTrans.m_vPosition;

  mTrans.m_vPosition = m_vStartPosition + vTranslate;

  m_vLastMoveDiff = mTrans.m_vPosition - vLastPos;

  SetTransformation(mTrans);

  if (!m_vLastMoveDiff.IsZero())
  {
    ezGizmoEvent ev;
    ev.m_pGizmo = this;
    ev.m_Type = ezGizmoEvent::Type::Interaction;
    m_GizmoEvents.Broadcast(ev);
  }

  return ezEditorInput::WasExclusivelyHandled;
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
