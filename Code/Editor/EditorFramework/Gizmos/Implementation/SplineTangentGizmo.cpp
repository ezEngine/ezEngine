#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Graphics/Camera.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Gizmos/SplineTangentGizmo.h>
#include <EditorFramework/Preferences/EditorPreferences.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSplineTangentGizmo, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezSplineTangentGizmo::ezSplineTangentGizmo()
{
  const ezColor colLength = ezColorScheme::LightUI(ezColorScheme::Grape);
  const ezColor colYaw = ezColorScheme::LightUI(ezColorScheme::Blue);
  const ezColor colPitch = ezColorScheme::LightUI(ezColorScheme::Green);

  m_hScale.ConfigureHandle(this, ezEngineGizmoHandleType::FromFile, colLength, ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable, "Editor/Meshes/ScaleArrowX.obj");
  m_hYaw.ConfigureHandle(this, ezEngineGizmoHandleType::FromFile, colYaw, ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable, "Editor/Meshes/RotatePlaneZ.obj");
  m_hPitch.ConfigureHandle(this, ezEngineGizmoHandleType::FromFile, colPitch, ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable, "Editor/Meshes/RotatePlaneY.obj");

  SetVisible(false);
  SetTransformation(ezTransform::MakeIdentity());
}

ezQuat ezSplineTangentGizmo::GetRotationResult() const
{
  ezQuat qYaw = ezQuat::MakeFromAxisAndAngle(GetTransformation().m_qRotation * ezVec3::MakeAxisZ(), m_fYawResult);
  ezQuat qPitch = ezQuat::MakeFromAxisAndAngle(GetTransformation().m_qRotation * ezVec3::MakeAxisY(), m_fPitchResult);
  return qYaw * qPitch;
}

void ezSplineTangentGizmo::UpdateStatusBarText(ezQtEngineDocumentWindow* pWindow)
{
  float fScale = 1.0f;
  GetOwnerWindow()->SetPermanentStatusBarMsg(ezFmt("Scale: {}", ezArgF(fScale, 2)));
}

void ezSplineTangentGizmo::OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hScale);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hYaw);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hPitch);
}

void ezSplineTangentGizmo::OnVisibleChanged(bool bVisible)
{
  m_hScale.SetVisible(bVisible);
  m_hYaw.SetVisible(bVisible);
  m_hPitch.SetVisible(bVisible);
}

void ezSplineTangentGizmo::OnTransformationChanged(const ezTransform& transform)
{
  m_hScale.SetTransformation(transform);
  m_hYaw.SetTransformation(transform);
  m_hPitch.SetTransformation(transform);
}

ezAngle ezSplineTangentGizmo::GetAngleFromMousePos(const ezVec2I32& vMousePos) const
{
  const ezVec3 axis = (m_uiActiveHandleIndex == 1) ? ezVec3::MakeAxisZ() : ezVec3::MakeAxisY();
  const ezPlane plane = ezPlane::MakeFromNormalAndPoint(GetTransformation().m_qRotation * axis, GetTransformation().m_vPosition);

  ezVec3 vCurrentInteractionPoint;
  GetPointOnPlane(plane, vMousePos, m_mInvViewProj, vCurrentInteractionPoint).IgnoreResult();

  const ezVec3 vDir = (vCurrentInteractionPoint - GetTransformation().m_vPosition).GetNormalized();
  const ezAngle a = vDir.GetAngleBetween(GetTransformation().m_qRotation * ezVec3::MakeAxisX());

  if (m_uiActiveHandleIndex == 1)
  {
    const ezVec3 vRightDir = GetTransformation().m_qRotation * ezVec3::MakeAxisY();
    return a * ezMath::Sign(vRightDir.Dot(vDir));
  }
  else
  {
    const ezVec3 vUpDir = GetTransformation().m_qRotation * ezVec3::MakeAxisZ();
    return a * ezMath::Sign(vUpDir.Dot(vDir));
  }
}

void ezSplineTangentGizmo::DoFocusLost(bool bCancel)
{
  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = bCancel ? ezGizmoEvent::Type::CancelInteractions : ezGizmoEvent::Type::EndInteractions;
  m_GizmoEvents.Broadcast(ev);

  ezViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  OnVisibleChanged(true);

  m_uiActiveHandleIndex = ezInvalidIndex;
}

ezEditorInput ezSplineTangentGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return ezEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return ezEditorInput::MayBeHandledByOthers;

  if (m_pInteractionGizmoHandle == &m_hScale)
  {
    m_uiActiveHandleIndex = 0;
  }
  else if (m_pInteractionGizmoHandle == &m_hYaw)
  {
    m_uiActiveHandleIndex = 1;
  }
  else if (m_pInteractionGizmoHandle == &m_hPitch)
  {
    m_uiActiveHandleIndex = 2;
  }
  else
  {
    return ezEditorInput::MayBeHandledByOthers;
  }

  ezViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  SetMouseMode(ezEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);
  GetInverseViewProjectionMatrix(m_mInvViewProj);

  ezVec2I32 vMousePos = ezVec2I32(e->pos().x(), e->pos().y());

  if (m_uiActiveHandleIndex == 0)
  {
    const ezVec3 startPos = GetTransformation().m_vPosition;
    const ezVec3 axis = GetTransformation().m_qRotation * ezVec3::MakeAxisX();

    GetPointOnAxis(startPos, axis, vMousePos, m_mInvViewProj, m_vInteractionPivot).IgnoreResult();
  }
  else
  {
    m_fStartAngle = GetAngleFromMousePos(vMousePos);
  }

  m_fScalingResult = 1.0f;
  m_fYawResult = ezAngle::MakeZero();
  m_fPitchResult = ezAngle::MakeZero();

  m_LastInteraction = ezTime::Now();

  SetActiveInputContext(this);

  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = ezGizmoEvent::Type::BeginInteractions;
  m_GizmoEvents.Broadcast(ev);

  return ezEditorInput::WasExclusivelyHandled;
}

ezEditorInput ezSplineTangentGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return ezEditorInput::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return ezEditorInput::WasExclusivelyHandled;
}

ezEditorInput ezSplineTangentGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInput::MayBeHandledByOthers;

  const ezTime tNow = ezTime::Now();

  if (tNow - m_LastInteraction < ezTime::MakeFromSeconds(1.0 / 25.0))
    return ezEditorInput::WasExclusivelyHandled;

  m_LastInteraction = tNow;

  if (m_uiActiveHandleIndex == 0)
  {
    const ezVec3 axis = GetTransformation().m_qRotation * ezVec3::MakeAxisX();

    ezVec3 vCurrentInteractionPoint;
    float fProjectedLength = 0.0f;
    if (GetPointOnAxis(m_vInteractionPivot, axis, ezVec2I32(e->pos().x(), e->pos().y()), m_mInvViewProj, vCurrentInteractionPoint, &fProjectedLength).Failed())
    {
      UpdateMouseMode(e);
      return ezEditorInput::WasExclusivelyHandled;
    }

    const float fScaleSpeed = 1.0f;

    if (fProjectedLength > 0.0f)
      m_fScalingResult = 1.0f + fProjectedLength * fScaleSpeed;
    if (fProjectedLength < 0.0f)
      m_fScalingResult = 1.0f / (1.0f - fProjectedLength * fScaleSpeed);

    GetOwnerWindow()->SetPermanentStatusBarMsg(ezFmt("Scale: {}", ezArgF(m_fScalingResult, 2)));
  }
  else
  {
    const ezAngle a = GetAngleFromMousePos(ezVec2I32(e->pos().x(), e->pos().y()));

    auto NormalizeAngle = [](ezAngle angle) {
      if (angle > ezAngle::MakeFromDegree(180.0f))
        angle -= ezAngle::MakeFromDegree(360.0f);
      else if (angle < ezAngle::MakeFromDegree(-180.0f))
        angle += ezAngle::MakeFromDegree(360.0f);
      return angle;
    };

    if (m_uiActiveHandleIndex == 1)
    {
      m_fYawResult = NormalizeAngle(a - m_fStartAngle);
      GetOwnerWindow()->SetPermanentStatusBarMsg(ezFmt("Rotate: {}", m_fYawResult));
    }
    else
    {
      m_fPitchResult = NormalizeAngle(a - m_fStartAngle);
      GetOwnerWindow()->SetPermanentStatusBarMsg(ezFmt("Rotate: {}", m_fPitchResult));
    }
  }

  UpdateMouseMode(e);

  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = ezGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  return ezEditorInput::WasExclusivelyHandled;
}
