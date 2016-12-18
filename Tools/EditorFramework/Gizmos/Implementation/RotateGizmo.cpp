#include <PCH.h>
#include <EditorFramework/Gizmos/RotateGizmo.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <Foundation/Logging/Log.h>
#include <QMouseEvent>
#include <CoreUtils/Graphics/Camera.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <EditorFramework/Gizmos/SnapProvider.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRotateGizmo, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezRotateGizmo::ezRotateGizmo()
{
  m_AxisX.Configure(this, ezEngineGizmoHandleType::Ring, ezColorLinearUB(128, 0, 0));
  m_AxisY.Configure(this, ezEngineGizmoHandleType::Ring, ezColorLinearUB(0, 128, 0));
  m_AxisZ.Configure(this, ezEngineGizmoHandleType::Ring, ezColorLinearUB(0, 0, 128));

  SetVisible(false);
  SetTransformation(ezMat4::IdentityMatrix());
}

void ezRotateGizmo::OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView)
{
  m_AxisX.SetOwner(pOwnerWindow->GetDocument());
  m_AxisY.SetOwner(pOwnerWindow->GetDocument());
  m_AxisZ.SetOwner(pOwnerWindow->GetDocument());
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

  m.SetRotationMatrixY(ezAngle::Degree(-90));
  m_AxisX.SetTransformation(transform * m);

  m.SetRotationMatrixX(ezAngle::Degree(90));
  m_AxisY.SetTransformation(transform * m);

  m.SetIdentity();
  m_AxisZ.SetTransformation(transform * m);
}

void ezRotateGizmo::DoFocusLost(bool bCancel)
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

  QApplication::restoreOverrideCursor();
}

ezEditorInut ezRotateGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return ezEditorInut::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return ezEditorInut::MayBeHandledByOthers;

  if (m_pInteractionGizmoHandle == &m_AxisX)
  {
    m_vRotationAxis = m_AxisX.GetTransformation().GetColumn(2).GetAsVec3().GetNormalized();
  }
  else if (m_pInteractionGizmoHandle == &m_AxisY)
  {
    m_vRotationAxis = m_AxisY.GetTransformation().GetColumn(2).GetAsVec3().GetNormalized();
  }
  else if (m_pInteractionGizmoHandle == &m_AxisZ)
  {
    m_vRotationAxis = m_AxisZ.GetTransformation().GetColumn(2).GetAsVec3().GetNormalized();
  }
  else
    return ezEditorInut::MayBeHandledByOthers;

  ezViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  msg.SendHighlightObjectMessage(GetOwnerWindow()->GetEditorEngineConnection());

  m_Rotation = ezAngle();

  m_LastMousePos = SetMouseMode(ezEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);

  m_StartRotation.SetFromMat3(GetTransformation().GetRotationalPart());

  ezMat4 mView, mProj, mViewProj;
  m_pCamera->GetViewMatrix(mView);
  m_pCamera->GetProjectionMatrix((float)m_Viewport.x / (float)m_Viewport.y, mProj);
  mViewProj = mProj * mView;
  m_InvViewProj = mViewProj.GetInverse();

  // compute screen space tangent for rotation
  {
    const ezVec3 vAxisWS = m_vRotationAxis.GetNormalized();
    const ezVec3 vMousePos(e->pos().x(), m_Viewport.y - e->pos().y(), 0);
    const ezVec3 vGizmoPosWS = GetTransformation().GetTranslationVector();

    ezVec3 vPosOnNearPlane, vRayDir;
    ezGraphicsUtils::ConvertScreenPosToWorldPos(m_InvViewProj, 0, 0, m_Viewport.x, m_Viewport.y, vMousePos, vPosOnNearPlane, &vRayDir);

    ezPlane plane;
    plane.SetFromNormalAndPoint(vAxisWS, vGizmoPosWS);

    ezVec3 vPointOnGizmoWS;
    if (!plane.GetRayIntersection(vPosOnNearPlane, vRayDir, nullptr, &vPointOnGizmoWS))
    {
      // fallback at grazing angles, will result in fallback vDirWS during normalization
      vPointOnGizmoWS = vGizmoPosWS;
    }

    ezVec3 vDirWS = vPointOnGizmoWS - vGizmoPosWS;
    vDirWS.NormalizeIfNotZero(ezVec3(1, 0, 0));

    ezVec3 vTangentWS = vAxisWS.Cross(vDirWS);
    vTangentWS.Normalize();

    const ezVec3 vTangentEndWS = vPointOnGizmoWS + vTangentWS;

    // compute the screen space position of the end point of the tangent vector, so that we can then compute the tangent in screen space
    ezVec3 vTangentEndSS;
    ezGraphicsUtils::ConvertWorldPosToScreenPos(mViewProj, 0, 0, m_Viewport.x, m_Viewport.y, vTangentEndWS, vTangentEndSS);
    vTangentEndSS.z = 0;

    const ezVec3 vTangentSS = vTangentEndSS - vMousePos;
    m_vScreenTangent.Set(vTangentSS.x, vTangentSS.y);
    m_vScreenTangent.NormalizeIfNotZero(ezVec2(1, 0));

    // because window coordinates are flipped along Y
    m_vScreenTangent.y = -m_vScreenTangent.y;
  }

  m_LastInteraction = ezTime::Now();

  SetActiveInputContext(this);

  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = ezGizmoEvent::Type::BeginInteractions;
  m_GizmoEvents.Broadcast(ev);

  return ezEditorInut::WasExclusivelyHandled;
}

ezEditorInut ezRotateGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInut::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return ezEditorInut::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return ezEditorInut::WasExclusivelyHandled;
}

ezEditorInut ezRotateGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInut::MayBeHandledByOthers;

  const ezTime tNow = ezTime::Now();

  if (tNow - m_LastInteraction < ezTime::Seconds(1.0 / 25.0))
    return ezEditorInut::WasExclusivelyHandled;

  m_LastInteraction = tNow;

  const ezVec2 vNewMousePos = ezVec2(e->globalPos().x(), e->globalPos().y());
  ezVec2 vDiff = vNewMousePos - ezVec2(m_LastMousePos.x, m_LastMousePos.y);

  m_LastMousePos = UpdateMouseMode(e);

  const float dv = m_vScreenTangent.Dot(vDiff);
  m_Rotation += ezAngle::Degree(dv);

  ezAngle rot = m_Rotation;

  // disable snapping when ALT is pressed
  if (!e->modifiers().testFlag(Qt::AltModifier))
    ezSnapProvider::SnapRotation(rot);

  m_CurrentRotation.SetFromAxisAndAngle(m_vRotationAxis, rot);

  ezMat4 mTrans = GetTransformation();
  mTrans.SetRotationalPart((m_CurrentRotation * m_StartRotation).GetAsMat3());

  SetTransformation(mTrans);

  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = ezGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  return ezEditorInut::WasExclusivelyHandled;
}

