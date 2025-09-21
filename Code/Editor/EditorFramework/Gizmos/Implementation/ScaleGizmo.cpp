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
  const ezColor colr = ezColorScheme::LightUI(ezColorScheme::Red);
  const ezColor colg = ezColorScheme::LightUI(ezColorScheme::Green);
  const ezColor colb = ezColorScheme::LightUI(ezColorScheme::Blue);
  const ezColor coly = ezColorScheme::LightUI(ezColorScheme::Gray);

  m_hAxisX.ConfigureHandle(this, ezEngineGizmoHandleType::FromFile, colr, ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable, "Editor/Meshes/ScaleArrowX.obj");
  m_hAxisY.ConfigureHandle(this, ezEngineGizmoHandleType::FromFile, colg, ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable, "Editor/Meshes/ScaleArrowY.obj");
  m_hAxisZ.ConfigureHandle(this, ezEngineGizmoHandleType::FromFile, colb, ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable, "Editor/Meshes/ScaleArrowZ.obj");
  m_hAxisXYZ.ConfigureHandle(this, ezEngineGizmoHandleType::FromFile, coly, ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable, "Editor/Meshes/ScaleXYZ.obj");

  SetVisible(false);
  SetTransformation(ezTransform::MakeIdentity());
}

void ezScaleGizmo::UpdateStatusBarText(ezQtEngineDocumentWindow* pWindow)
{
  const ezVec3 scale(1.0f);
  GetOwnerWindow()->SetPermanentStatusBarMsg(ezFmt("Scale: {}, {}, {}", ezArgF(scale.x, 2), ezArgF(scale.y, 2), ezArgF(scale.z, 2)));
}

void ezScaleGizmo::EnableAxis(bool x, bool y, bool z, bool bXyz)
{
  m_bEnableAxisX = x;
  m_bEnableAxisY = y;
  m_bEnableAxisZ = z;
  m_bEnableAxisXYZ = bXyz;
}

void ezScaleGizmo::OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAxisX);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAxisY);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAxisZ);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAxisXYZ);
}

void ezScaleGizmo::OnVisibleChanged(bool bVisible)
{
  m_hAxisX.SetVisible(bVisible && m_bEnableAxisX);
  m_hAxisY.SetVisible(bVisible && m_bEnableAxisY);
  m_hAxisZ.SetVisible(bVisible && m_bEnableAxisZ);
  m_hAxisXYZ.SetVisible(bVisible && m_bEnableAxisXYZ);
}

void ezScaleGizmo::OnTransformationChanged(const ezTransform& transform)
{
  m_hAxisX.SetTransformation(transform);
  m_hAxisY.SetTransformation(transform);
  m_hAxisZ.SetTransformation(transform);
  m_hAxisXYZ.SetTransformation(transform);
}

void ezScaleGizmo::DoFocusLost(bool bCancel)
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
  m_hAxisXYZ.SetVisible(m_bEnableAxisXYZ);
}

ezEditorInput ezScaleGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return ezEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return ezEditorInput::MayBeHandledByOthers;

  if (m_pInteractionGizmoHandle == &m_hAxisX)
  {
    m_vMoveAxis.Set(1, 0, 0);
  }
  else if (m_pInteractionGizmoHandle == &m_hAxisY)
  {
    m_vMoveAxis.Set(0, 1, 0);
  }
  else if (m_pInteractionGizmoHandle == &m_hAxisZ)
  {
    m_vMoveAxis.Set(0, 0, 1);
  }
  else if (m_pInteractionGizmoHandle == &m_hAxisXYZ)
  {
    m_vMoveAxis.Set(1, 1, 1);
  }
  else
    return ezEditorInput::MayBeHandledByOthers;

  ezViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_vLastMousePos = SetMouseMode(ezEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);

  m_vScalingResult.Set(1.0f);
  m_vScaleMouseMove.SetZero();

  ezMat4 mView = m_pCamera->GetViewMatrix();
  ezMat4 mProj;
  m_pCamera->GetProjectionMatrix((float)m_vViewport.x / (float)m_vViewport.y, mProj);
  ezMat4 mViewProj = mProj * mView;
  m_mInvViewProj = mViewProj.GetInverse();

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

  if (tNow - m_LastInteraction < ezTime::MakeFromSeconds(1.0 / 25.0))
    return ezEditorInput::WasExclusivelyHandled;

  m_LastInteraction = tNow;

  const QPoint mousePosition = e->globalPosition().toPoint();

  const ezVec2I32 vNewMousePos = ezVec2I32(mousePosition.x(), mousePosition.y());
  ezVec2I32 vDiff = (vNewMousePos - m_vLastMousePos);

  m_vLastMousePos = UpdateMouseMode(e);

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

  // disable snapping when SHIFT is pressed
  if (!e->modifiers().testFlag(Qt::ShiftModifier))
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
  const ezColor colr = ezColorScheme::LightUI(ezColorScheme::Red);
  const ezColor colg = ezColorScheme::LightUI(ezColorScheme::Green);
  const ezColor colb = ezColorScheme::LightUI(ezColorScheme::Blue);

  // Overwrite axis to be boxes.
  m_hAxisX.ConfigureHandle(this, ezEngineGizmoHandleType::Box, colr, ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable);
  m_hAxisY.ConfigureHandle(this, ezEngineGizmoHandleType::Box, colg, ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable);
  m_hAxisZ.ConfigureHandle(this, ezEngineGizmoHandleType::Box, colb, ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable);
}

void ezManipulatorScaleGizmo::OnTransformationChanged(const ezTransform& transform)
{
  const float fOffset = 0.8f;
  ezTransform t;
  t.SetIdentity();
  t.m_vPosition = ezVec3(fOffset, 0, 0);
  t.m_vScale = ezVec3(0.2f);

  m_hAxisX.SetTransformation(transform * t);

  t.m_qRotation = ezQuat::MakeFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::MakeFromDegree(90));
  t.m_vPosition = ezVec3(0, fOffset, 0);
  m_hAxisY.SetTransformation(transform * t);

  t.m_qRotation = ezQuat::MakeFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::MakeFromDegree(-90));
  t.m_vPosition = ezVec3(0, 0, fOffset);
  m_hAxisZ.SetTransformation(transform * t);

  t.SetIdentity();
  t.m_vScale = ezVec3(0.3f);
  m_hAxisXYZ.SetTransformation(transform * t);
}
