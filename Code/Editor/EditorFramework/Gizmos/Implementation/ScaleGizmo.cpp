#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Graphics/Camera.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/ScaleGizmo.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Preferences/EditorPreferences.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezScaleGizmo, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezScaleGizmo::ezScaleGizmo()
{
  ezEditorPreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezEditorPreferencesUser>();
  m_bUseExperimentalGizmo = !pPreferences->m_bOldGizmos;

  if (m_bUseExperimentalGizmo)
  {
    const ezColor colr = ezColorGammaUB(206, 0, 46);
    const ezColor colg = ezColorGammaUB(101, 206, 0);
    const ezColor colb = ezColorGammaUB(0, 125, 206);
    const ezColor coly = ezColorGammaUB(128, 128, 0);

    m_AxisX.ConfigureHandle(this, ezEngineGizmoHandleType::FromFile, colr, ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable, "Editor/Meshes/ScaleArrowX.obj");
    m_AxisY.ConfigureHandle(this, ezEngineGizmoHandleType::FromFile, colg, ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable, "Editor/Meshes/ScaleArrowY.obj");
    m_AxisZ.ConfigureHandle(this, ezEngineGizmoHandleType::FromFile, colb, ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable, "Editor/Meshes/ScaleArrowZ.obj");
    m_AxisXYZ.ConfigureHandle(this, ezEngineGizmoHandleType::FromFile, coly, ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable, "Editor/Meshes/ScaleXYZ.obj");
  }
  else
  {
    m_AxisX.ConfigureHandle(this, ezEngineGizmoHandleType::Piston, ezColorLinearUB(128, 0, 0), ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable);
    m_AxisY.ConfigureHandle(this, ezEngineGizmoHandleType::Piston, ezColorLinearUB(0, 128, 0), ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable);
    m_AxisZ.ConfigureHandle(this, ezEngineGizmoHandleType::Piston, ezColorLinearUB(0, 0, 128), ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable);
    m_AxisXYZ.ConfigureHandle(this, ezEngineGizmoHandleType::Box, ezColorLinearUB(128, 128, 0), ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable);
  }

  SetVisible(false);
  SetTransformation(ezTransform::IdentityTransform());
}

void ezScaleGizmo::UpdateStatusBarText(ezQtEngineDocumentWindow* pWindow)
{
  const ezVec3 scale(1.0f);
  GetOwnerWindow()->SetPermanentStatusBarMsg(ezFmt("Scale: {}, {}, {}", ezArgF(scale.x, 2), ezArgF(scale.y, 2), ezArgF(scale.z, 2)));
}

void ezScaleGizmo::OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_AxisX);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_AxisY);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_AxisZ);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_AxisXYZ);
}

void ezScaleGizmo::OnVisibleChanged(bool bVisible)
{
  m_AxisX.SetVisible(bVisible);
  m_AxisY.SetVisible(bVisible);
  m_AxisZ.SetVisible(bVisible);
  m_AxisXYZ.SetVisible(bVisible);
}

void ezScaleGizmo::OnTransformationChanged(const ezTransform& transform)
{
  if (m_bUseExperimentalGizmo)
  {
    m_AxisX.SetTransformation(transform);
    m_AxisY.SetTransformation(transform);
    m_AxisZ.SetTransformation(transform);
    m_AxisXYZ.SetTransformation(transform);
  }
  else
  {
    ezTransform t;
    t.SetIdentity();

    t.m_vScale.Set(2.0f);
    m_AxisX.SetTransformation(transform * t);

    t.m_qRotation.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(90));
    m_AxisY.SetTransformation(transform * t);

    t.m_qRotation.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(-90));
    m_AxisZ.SetTransformation(transform * t);

    t.SetIdentity();
    t.m_vScale = ezVec3(0.2f);
    m_AxisXYZ.SetTransformation(transform * t);
  }
}

void ezScaleGizmo::DoFocusLost(bool bCancel)
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
  m_AxisXYZ.SetVisible(true);
}

ezEditorInput ezScaleGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return ezEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return ezEditorInput::MayBeHandledByOthers;

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
    return ezEditorInput::MayBeHandledByOthers;

  ezViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_LastMousePos = SetMouseMode(ezEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);

  m_vScalingResult.Set(1.0f);
  m_vScaleMouseMove.SetZero();

  ezMat4 mView = m_pCamera->GetViewMatrix();
  ezMat4 mProj;
  m_pCamera->GetProjectionMatrix((float)m_Viewport.x / (float)m_Viewport.y, mProj);
  ezMat4 mViewProj = mProj * mView;
  m_InvViewProj = mViewProj.GetInverse();

  m_LastInteraction = ezTime::Now();

  SetActiveInputContext(this);

  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = ezGizmoEvent::Type::BeginInteractions;
  m_GizmoEvents.Broadcast(ev);

  return ezEditorInput::WasExclusivelyHandled;
}

ezEditorInput ezScaleGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return ezEditorInput::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return ezEditorInput::WasExclusivelyHandled;
}

ezEditorInput ezScaleGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInput::MayBeHandledByOthers;

  const ezTime tNow = ezTime::Now();

  if (tNow - m_LastInteraction < ezTime::Seconds(1.0 / 25.0))
    return ezEditorInput::WasExclusivelyHandled;

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

  // disable snapping when ALT is pressed
  if (!e->modifiers().testFlag(Qt::AltModifier))
    ezSnapProvider::SnapScale(m_vScalingResult);

  GetOwnerWindow()->SetPermanentStatusBarMsg(
    ezFmt("Scale: {}, {}, {}", ezArgF(m_vScalingResult.x, 2), ezArgF(m_vScalingResult.y, 2), ezArgF(m_vScalingResult.z, 2)));

  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = ezGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  return ezEditorInput::WasExclusivelyHandled;
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezManipulatorScaleGizmo, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezManipulatorScaleGizmo::ezManipulatorScaleGizmo()
{
  // Overwrite axis to be boxes.
  m_AxisX.ConfigureHandle(this, ezEngineGizmoHandleType::Box, ezColorLinearUB(128, 0, 0), ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable);
  m_AxisY.ConfigureHandle(this, ezEngineGizmoHandleType::Box, ezColorLinearUB(0, 128, 0), ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable);
  m_AxisZ.ConfigureHandle(this, ezEngineGizmoHandleType::Box, ezColorLinearUB(0, 0, 128), ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable);
}

void ezManipulatorScaleGizmo::OnTransformationChanged(const ezTransform& transform)
{
  const float fOffset = 0.8f;
  ezTransform t;
  t.SetIdentity();
  t.m_vPosition = ezVec3(fOffset, 0, 0);
  t.m_vScale = ezVec3(0.2f);

  m_AxisX.SetTransformation(transform * t);

  t.m_qRotation.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(90));
  t.m_vPosition = ezVec3(0, fOffset, 0);
  m_AxisY.SetTransformation(transform * t);

  t.m_qRotation.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(-90));
  t.m_vPosition = ezVec3(0, 0, fOffset);
  m_AxisZ.SetTransformation(transform * t);

  t.SetIdentity();
  t.m_vScale = ezVec3(0.3f);
  m_AxisXYZ.SetTransformation(transform * t);
}
