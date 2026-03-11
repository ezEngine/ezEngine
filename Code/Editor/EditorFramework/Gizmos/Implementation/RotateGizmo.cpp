#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Graphics/Camera.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/RotateGizmo.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/Utilities/GraphicsUtils.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRotateGizmo, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezRotateGizmo::ezRotateGizmo()
{
  const ezColor colr = ezColorScheme::LightUI(ezColorScheme::Red);
  const ezColor colg = ezColorScheme::LightUI(ezColorScheme::Green);
  const ezColor colb = ezColorScheme::LightUI(ezColorScheme::Blue);

  m_hAxisX.ConfigureHandle(this, ezEngineGizmoHandleType::FromFile, colr, ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable, "Editor/Meshes/RotatePlaneX.obj");
  m_hAxisY.ConfigureHandle(this, ezEngineGizmoHandleType::FromFile, colg, ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable, "Editor/Meshes/RotatePlaneY.obj");
  m_hAxisZ.ConfigureHandle(this, ezEngineGizmoHandleType::FromFile, colb, ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable, "Editor/Meshes/RotatePlaneZ.obj");

  SetVisible(false);
  SetTransformation(ezTransform::MakeIdentity());
}

void ezRotateGizmo::UpdateStatusBarText(ezQtEngineDocumentWindow* pWindow)
{
  GetOwnerWindow()->SetPermanentStatusBarMsg(ezFmt("Rotation: {}", ezAngle()));
}

void ezRotateGizmo::EnableAxis(bool x, bool y, bool z)
{
  m_bEnableAxisX = x;
  m_bEnableAxisY = y;
  m_bEnableAxisZ = z;
}

void ezRotateGizmo::OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAxisX);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAxisY);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAxisZ);
}

void ezRotateGizmo::OnVisibleChanged(bool bVisible)
{
  m_hAxisX.SetVisible(bVisible && m_bEnableAxisX);
  m_hAxisY.SetVisible(bVisible && m_bEnableAxisY);
  m_hAxisZ.SetVisible(bVisible && m_bEnableAxisZ);
}

void ezRotateGizmo::OnTransformationChanged(const ezTransform& transform)
{
  m_hAxisX.SetTransformation(transform);
  m_hAxisY.SetTransformation(transform);
  m_hAxisZ.SetTransformation(transform);
}

void ezRotateGizmo::DoFocusLost(bool bCancel)
{
  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = bCancel ? ezGizmoEvent::Type::CancelInteractions : ezGizmoEvent::Type::EndInteractions;
  m_GizmoEvents.Broadcast(ev);

  ezViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_hAxisX.SetVisible(m_bEnableAxisX);
  m_hAxisY.SetVisible(m_bEnableAxisY);
  m_hAxisZ.SetVisible(m_bEnableAxisZ);
}

ezEditorInput ezRotateGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return ezEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return ezEditorInput::MayBeHandledByOthers;

  const ezQuat gizmoRot = GetTransformation().m_qRotation;

  if (m_pInteractionGizmoHandle == &m_hAxisX)
  {
    m_vRotationAxis = gizmoRot * ezVec3(1, 0, 0);
  }
  else if (m_pInteractionGizmoHandle == &m_hAxisY)
  {
    m_vRotationAxis = gizmoRot * ezVec3(0, 1, 0);
  }
  else if (m_pInteractionGizmoHandle == &m_hAxisZ)
  {
    m_vRotationAxis = gizmoRot * ezVec3(0, 0, 1);
  }
  else
    return ezEditorInput::MayBeHandledByOthers;

  ezViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_Rotation = ezAngle();

  m_vLastMousePos = SetMouseMode(ezEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);

  m_qStartRotation = GetTransformation().m_qRotation;

  ezMat4 mView = m_pCamera->GetViewMatrix();
  ezMat4 mProj;
  m_pCamera->GetProjectionMatrix((float)m_vViewport.x / (float)m_vViewport.y, mProj);
  ezMat4 mViewProj = mProj * mView;
  m_mInvViewProj = mViewProj.GetInverse();

  // compute screen space tangent for rotation
  {
    const ezVec3 vAxisWS = m_vRotationAxis.GetNormalized();
    const ezVec3 vMousePos(e->pos().x(), e->pos().y(), 0);
    const ezVec3 vGizmoPosWS = GetTransformation().m_vPosition;

    ezVec3 vPosOnNearPlane, vRayDir;
    ezGraphicsUtils::ConvertScreenPosToWorldPos(m_mInvViewProj, 0, 0, m_vViewport.x, m_vViewport.y, vMousePos, vPosOnNearPlane, &vRayDir).IgnoreResult();

    ezPlane plane;
    plane = ezPlane::MakeFromNormalAndPoint(vAxisWS, vGizmoPosWS);

    ezVec3 vPointOnGizmoWS;
    if (!plane.GetRayIntersection(vPosOnNearPlane, vRayDir, nullptr, &vPointOnGizmoWS))
    {
      // fallback at grazing angles, will result in fallback vDirWS during normalization
      vPointOnGizmoWS = vGizmoPosWS;
    }

    ezVec3 vDirWS = vPointOnGizmoWS - vGizmoPosWS;
    vDirWS.NormalizeIfNotZero(ezVec3(1, 0, 0)).IgnoreResult();

    ezVec3 vTangentWS = vAxisWS.CrossRH(vDirWS);
    vTangentWS.Normalize();

    const ezVec3 vTangentEndWS = vPointOnGizmoWS + vTangentWS;

    // compute the screen space position of the end point of the tangent vector, so that we can then compute the tangent in screen space
    ezVec3 vTangentEndSS;
    ezGraphicsUtils::ConvertWorldPosToScreenPos(mViewProj, 0, 0, m_vViewport.x, m_vViewport.y, vTangentEndWS, vTangentEndSS).IgnoreResult();
    vTangentEndSS.z = 0;

    const ezVec3 vTangentSS = vTangentEndSS - vMousePos;
    m_vScreenTangent.Set(vTangentSS.x, vTangentSS.y);
    m_vScreenTangent.NormalizeIfNotZero(ezVec2(1, 0)).IgnoreResult();

    // because window coordinates are flipped along Y
    m_vScreenTangent.y = -m_vScreenTangent.y;
  }

  m_LastInteraction = ezTime::Now();

  SetActiveInputContext(this);

  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = ezGizmoEvent::Type::BeginInteractions;
  m_GizmoEvents.Broadcast(ev);

  return ezEditorInput::WasExclusivelyHandled;
}

ezEditorInput ezRotateGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return ezEditorInput::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return ezEditorInput::WasExclusivelyHandled;
}

ezEditorInput ezRotateGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInput::MayBeHandledByOthers;

  const ezTime tNow = ezTime::Now();

  if (tNow - m_LastInteraction < ezTime::MakeFromSeconds(1.0 / 25.0))
    return ezEditorInput::WasExclusivelyHandled;

  m_LastInteraction = tNow;

  const QPoint mousePosition = e->globalPosition().toPoint();

  const ezVec2 vNewMousePos = ezVec2(mousePosition.x(), mousePosition.y());
  ezVec2 vDiff = vNewMousePos - ezVec2(m_vLastMousePos.x, m_vLastMousePos.y);

  m_vLastMousePos = UpdateMouseMode(e);

  const float dv = m_vScreenTangent.Dot(vDiff);
  m_Rotation += ezAngle::MakeFromDegree(dv);

  ezAngle rot = m_Rotation;

  // disable snapping when SHIFT is pressed
  if (!e->modifiers().testFlag(Qt::ShiftModifier))
    ezSnapProvider::SnapRotation(rot);

  m_qCurrentRotation = ezQuat::MakeFromAxisAndAngle(m_vRotationAxis, rot);

  ezTransform mTrans = GetTransformation();
  mTrans.m_qRotation = m_qCurrentRotation * m_qStartRotation;

  SetTransformation(mTrans);

  GetOwnerWindow()->SetPermanentStatusBarMsg(ezFmt("Rotation: {}", rot));

  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = ezGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  return ezEditorInput::WasExclusivelyHandled;
}
