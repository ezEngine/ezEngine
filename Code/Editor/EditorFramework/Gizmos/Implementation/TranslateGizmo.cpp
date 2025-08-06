#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Graphics/Camera.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/Utilities/GraphicsUtils.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTranslateGizmo, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezTranslateGizmo::ezTranslateGizmo()
{
  m_vStartPosition.SetZero();
  m_fCameraSpeed = 0.2f;

  const ezColor colr = ezColorScheme::LightUI(ezColorScheme::Red);
  const ezColor colg = ezColorScheme::LightUI(ezColorScheme::Green);
  const ezColor colb = ezColorScheme::LightUI(ezColorScheme::Blue);

  m_hAxisX.ConfigureHandle(this, ezEngineGizmoHandleType::FromFile, colr, ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable, "Editor/Meshes/TranslateArrowX.obj");
  m_hAxisY.ConfigureHandle(this, ezEngineGizmoHandleType::FromFile, colg, ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable, "Editor/Meshes/TranslateArrowY.obj");
  m_hAxisZ.ConfigureHandle(this, ezEngineGizmoHandleType::FromFile, colb, ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable, "Editor/Meshes/TranslateArrowZ.obj");

  m_hPlaneYZ.ConfigureHandle(this, ezEngineGizmoHandleType::FromFile, colr, ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable | ezGizmoFlags::FaceCamera, "Editor/Meshes/TranslatePlaneX.obj");
  m_hPlaneXZ.ConfigureHandle(this, ezEngineGizmoHandleType::FromFile, colg, ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable | ezGizmoFlags::FaceCamera, "Editor/Meshes/TranslatePlaneY.obj");
  m_hPlaneXY.ConfigureHandle(this, ezEngineGizmoHandleType::FromFile, colb, ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable | ezGizmoFlags::FaceCamera, "Editor/Meshes/TranslatePlaneZ.obj");

  SetVisible(false);
  SetTransformation(ezTransform::MakeIdentity());

  m_Mode = TranslateMode::None;
  m_MovementMode = MovementMode::ScreenProjection;
  m_LastHandleInteraction = HandleInteraction::None;
}

void ezTranslateGizmo::OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAxisX);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAxisY);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAxisZ);

  pOwnerWindow->GetDocument()->AddSyncObject(&m_hPlaneXY);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hPlaneXZ);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hPlaneYZ);
}

void ezTranslateGizmo::OnVisibleChanged(bool bVisible)
{
  m_hAxisX.SetVisible(bVisible);
  m_hAxisY.SetVisible(bVisible);
  m_hAxisZ.SetVisible(bVisible);

  m_hPlaneXY.SetVisible(bVisible);
  m_hPlaneXZ.SetVisible(bVisible);
  m_hPlaneYZ.SetVisible(bVisible);
}

void ezTranslateGizmo::OnTransformationChanged(const ezTransform& transform)
{
  m_hAxisX.SetTransformation(transform);
  m_hAxisY.SetTransformation(transform);
  m_hAxisZ.SetTransformation(transform);
  m_hPlaneXY.SetTransformation(transform);
  m_hPlaneYZ.SetTransformation(transform);
  m_hPlaneXZ.SetTransformation(transform);

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

  m_hAxisX.SetVisible(true);
  m_hAxisY.SetVisible(true);
  m_hAxisZ.SetVisible(true);

  m_hPlaneXY.SetVisible(true);
  m_hPlaneXZ.SetVisible(true);
  m_hPlaneYZ.SetVisible(true);

  m_Mode = TranslateMode::None;
  m_LastHandleInteraction = HandleInteraction::None;
  m_MovementMode = MovementMode::ScreenProjection;
  m_vLastMoveDiff.SetZero();

  m_vStartPosition = GetTransformation().m_vPosition;
  m_vTotalMouseDiff.SetZero();

  GetOwnerWindow()->SetPermanentStatusBarMsg("");
}

ezEditorInput ezTranslateGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return ezEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return ezEditorInput::MayBeHandledByOthers;

  m_vLastMoveDiff.SetZero();

  const ezQuat gizmoRot = GetTransformation().m_qRotation;

  if (m_pInteractionGizmoHandle == &m_hAxisX)
  {
    m_vMoveAxis = gizmoRot * ezVec3(1, 0, 0);
    m_Mode = TranslateMode::Axis;
    m_LastHandleInteraction = HandleInteraction::AxisX;
  }
  else if (m_pInteractionGizmoHandle == &m_hAxisY)
  {
    m_vMoveAxis = gizmoRot * ezVec3(0, 1, 0);
    m_Mode = TranslateMode::Axis;
    m_LastHandleInteraction = HandleInteraction::AxisY;
  }
  else if (m_pInteractionGizmoHandle == &m_hAxisZ)
  {
    m_vMoveAxis = gizmoRot * ezVec3(0, 0, 1);
    m_Mode = TranslateMode::Axis;
    m_LastHandleInteraction = HandleInteraction::AxisZ;
  }
  else if (m_pInteractionGizmoHandle == &m_hPlaneXY)
  {
    m_vMoveAxis = gizmoRot * ezVec3(0, 0, 1);
    m_vPlaneAxis[0] = gizmoRot * ezVec3(1, 0, 0);
    m_vPlaneAxis[1] = gizmoRot * ezVec3(0, 1, 0);
    m_Mode = TranslateMode::Plane;
    m_LastHandleInteraction = HandleInteraction::PlaneZ;
  }
  else if (m_pInteractionGizmoHandle == &m_hPlaneXZ)
  {
    m_vMoveAxis = gizmoRot * ezVec3(0, 1, 0);
    m_vPlaneAxis[0] = gizmoRot * ezVec3(1, 0, 0);
    m_vPlaneAxis[1] = gizmoRot * ezVec3(0, 0, 1);
    m_Mode = TranslateMode::Plane;
    m_LastHandleInteraction = HandleInteraction::PlaneY;
  }
  else if (m_pInteractionGizmoHandle == &m_hPlaneYZ)
  {
    m_vMoveAxis = gizmoRot * ezVec3(1, 0, 0);
    m_vPlaneAxis[0] = gizmoRot * ezVec3(0, 1, 0);
    m_vPlaneAxis[1] = gizmoRot * ezVec3(0, 0, 1);
    m_Mode = TranslateMode::Plane;
    m_LastHandleInteraction = HandleInteraction::PlaneX;
  }
  else
    return ezEditorInput::MayBeHandledByOthers;

  ezViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_vStartPosition = GetTransformation().m_vPosition;
  m_vTotalMouseDiff.SetZero();

  GetInverseViewProjectionMatrix(m_mInvViewProj);

  m_LastInteraction = ezTime::Now();

  m_vLastMousePos = SetMouseMode(ezEditorInputContext::MouseMode::WrapAtScreenBorders);
  SetActiveInputContext(this);

  if (m_Mode == TranslateMode::Axis)
  {
    GetPointOnAxis(m_vStartPosition, m_vMoveAxis, ezVec2I32(e->pos().x(), e->pos().y()), m_mInvViewProj, m_vInteractionPivot).IgnoreResult();
  }
  else if (m_Mode == TranslateMode::Plane)
  {
    GetPointOnPlane(ezVec2I32(e->pos().x(), e->pos().y()), m_vInteractionPivot).IgnoreResult();
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

ezResult ezTranslateGizmo::GetPointOnPlane(const ezVec2I32& vScreenPos, ezVec3& out_Result) const
{
  ezPlane Plane;
  Plane = ezPlane::MakeFromNormalAndPoint(m_vMoveAxis, m_vStartPosition);

  return SUPER::GetPointOnPlane(Plane, vScreenPos, m_mInvViewProj, out_Result);
}

ezEditorInput ezTranslateGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInput::MayBeHandledByOthers;

  const ezTime tNow = ezTime::Now();

  if (tNow - m_LastInteraction < ezTime::MakeFromSeconds(1.0 / 25.0))
    return ezEditorInput::WasExclusivelyHandled;

  const QPoint mousePosition = e->globalPosition().toPoint();

  const ezVec2I32 CurMousePos(mousePosition.x(), mousePosition.y());

  m_LastInteraction = tNow;

  ezTransform mTrans = GetTransformation();
  ezVec3 vTranslate(0);

  if (m_MovementMode == MovementMode::ScreenProjection)
  {
    ezVec3 vCurrentInteractionPoint;

    if (m_Mode == TranslateMode::Axis)
    {
      if (GetPointOnAxis(m_vStartPosition, m_vMoveAxis, ezVec2I32(e->pos().x(), e->pos().y()), m_mInvViewProj, vCurrentInteractionPoint).Failed())
      {
        m_vLastMousePos = UpdateMouseMode(e);
        return ezEditorInput::WasExclusivelyHandled;
      }
    }
    else if (m_Mode == TranslateMode::Plane)
    {
      if (GetPointOnPlane(ezVec2I32(e->pos().x(), e->pos().y()), vCurrentInteractionPoint).Failed())
      {
        m_vLastMousePos = UpdateMouseMode(e);
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

    m_vTotalMouseDiff += ezVec2((float)(CurMousePos.x - m_vLastMousePos.x), (float)(CurMousePos.y - m_vLastMousePos.y));
    const ezVec3 vMouseDir = m_pCamera->GetDirRight() * m_vTotalMouseDiff.x + -m_pCamera->GetDirUp() * m_vTotalMouseDiff.y;

    if (m_Mode == TranslateMode::Axis)
    {
      vTranslate = m_vMoveAxis * (m_vMoveAxis.Dot(vMouseDir)) * fSpeed;
    }
    else if (m_Mode == TranslateMode::Plane)
    {
      vTranslate = m_vPlaneAxis[0] * (m_vPlaneAxis[0].Dot(vMouseDir)) * fSpeed + m_vPlaneAxis[1] * (m_vPlaneAxis[1].Dot(vMouseDir)) * fSpeed;
    }
  }

  m_vLastMousePos = UpdateMouseMode(e);

  // disable snapping when SHIFT is pressed
  if (!e->modifiers().testFlag(Qt::ShiftModifier))
  {
    ezSnapProvider::SnapTranslationInLocalSpace(mTrans.m_qRotation, vTranslate);
  }

  const ezVec3 vLastPos = mTrans.m_vPosition;

  mTrans.m_vPosition = m_vStartPosition + vTranslate;

  m_vLastMoveDiff = mTrans.m_vPosition - vLastPos;

  SetTransformation(mTrans);

  // set statusbar message
  {
    const ezVec3 diff = GetTransformation().m_qRotation.GetInverse() * GetTranslationResult();
    GetOwnerWindow()->SetPermanentStatusBarMsg(ezFmt("Translation: {}, {}, {}", ezArgF(diff.x, 2), ezArgF(diff.y, 2), ezArgF(diff.z, 2)));
  }

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
    m_vLastMousePos = SetMouseMode(ezEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);
  }
  else
  {
    m_vLastMousePos = SetMouseMode(ezEditorInputContext::MouseMode::WrapAtScreenBorders);
  }
}

void ezTranslateGizmo::SetCameraSpeed(float fSpeed)
{
  m_fCameraSpeed = fSpeed;
}

void ezTranslateGizmo::UpdateStatusBarText(ezQtEngineDocumentWindow* pWindow)
{
  const ezVec3 diff = ezVec3::MakeZero();
  GetOwnerWindow()->SetPermanentStatusBarMsg(ezFmt("Translation: {}, {}, {}", ezArgF(diff.x, 2), ezArgF(diff.y, 2), ezArgF(diff.z, 2)));
}
