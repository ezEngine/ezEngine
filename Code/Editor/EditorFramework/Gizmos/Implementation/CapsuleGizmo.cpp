#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/CapsuleGizmo.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCapsuleGizmo, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezCapsuleGizmo::ezCapsuleGizmo()
{
  m_fLength = 1.0f;
  m_fRadius = 0.25f;

  m_ManipulateMode = ManipulateMode::None;

  m_hRadius.ConfigureHandle(this, ezEngineGizmoHandleType::CylinderZ, ezColorLinearUB(200, 200, 200, 128), ezGizmoFlags::Pickable);
  m_hLengthTop.ConfigureHandle(this, ezEngineGizmoHandleType::HalfSphereZ, ezColorLinearUB(200, 200, 200, 128), ezGizmoFlags::Pickable);
  m_hLengthBottom.ConfigureHandle(this, ezEngineGizmoHandleType::HalfSphereZ, ezColorLinearUB(200, 200, 200, 128), ezGizmoFlags::Pickable);

  SetVisible(false);
  SetTransformation(ezTransform::MakeIdentity());
}

void ezCapsuleGizmo::OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hLengthTop);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hLengthBottom);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hRadius);
}

void ezCapsuleGizmo::OnVisibleChanged(bool bVisible)
{
  m_hLengthTop.SetVisible(bVisible);
  m_hLengthBottom.SetVisible(bVisible);
  m_hRadius.SetVisible(bVisible);
}

void ezCapsuleGizmo::OnTransformationChanged(const ezTransform& transform)
{
  {
    ezTransform mScaleCylinder;
    mScaleCylinder.SetIdentity();
    mScaleCylinder.m_vScale = ezVec3(m_fRadius, m_fRadius, m_fLength);

    m_hRadius.SetTransformation(transform * mScaleCylinder);
  }

  {
    ezTransform mScaleSpheres;
    mScaleSpheres.SetIdentity();
    mScaleSpheres.m_vScale.Set(m_fRadius);
    mScaleSpheres.m_vPosition.Set(0, 0, m_fLength * 0.5f);
    m_hLengthTop.SetTransformation(transform * mScaleSpheres);
  }

  {
    ezTransform mScaleSpheres;
    mScaleSpheres.SetIdentity();
    mScaleSpheres.m_vScale.Set(m_fRadius, -m_fRadius, -m_fRadius);
    mScaleSpheres.m_vPosition.Set(0, 0, -m_fLength * 0.5f);
    m_hLengthBottom.SetTransformation(transform * mScaleSpheres);
  }
}

void ezCapsuleGizmo::DoFocusLost(bool bCancel)
{
  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = bCancel ? ezGizmoEvent::Type::CancelInteractions : ezGizmoEvent::Type::EndInteractions;
  m_GizmoEvents.Broadcast(ev);

  ezViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_hLengthTop.SetVisible(true);
  m_hLengthBottom.SetVisible(true);
  m_hRadius.SetVisible(true);

  m_ManipulateMode = ManipulateMode::None;
}

ezEditorInput ezCapsuleGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return ezEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return ezEditorInput::MayBeHandledByOthers;
  if (e->modifiers() != 0)
    return ezEditorInput::MayBeHandledByOthers;

  if (m_pInteractionGizmoHandle == &m_hRadius)
  {
    m_ManipulateMode = ManipulateMode::Radius;
  }
  else if (m_pInteractionGizmoHandle == &m_hLengthTop || m_pInteractionGizmoHandle == &m_hLengthBottom)
  {
    m_ManipulateMode = ManipulateMode::Length;
  }
  else
    return ezEditorInput::MayBeHandledByOthers;

  ezViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_LastInteraction = ezTime::Now();

  m_vLastMousePos = SetMouseMode(ezEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);

  SetActiveInputContext(this);

  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = ezGizmoEvent::Type::BeginInteractions;
  m_GizmoEvents.Broadcast(ev);

  return ezEditorInput::WasExclusivelyHandled;
}

ezEditorInput ezCapsuleGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return ezEditorInput::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return ezEditorInput::WasExclusivelyHandled;
}

ezEditorInput ezCapsuleGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInput::MayBeHandledByOthers;

  const ezTime tNow = ezTime::Now();

  if (tNow - m_LastInteraction < ezTime::MakeFromSeconds(1.0 / 25.0))
    return ezEditorInput::WasExclusivelyHandled;

  m_LastInteraction = tNow;

  QPoint mousePosition = e->globalPosition().toPoint();

  const ezVec2I32 vNewMousePos = ezVec2I32(mousePosition.x(), mousePosition.y());
  const ezVec2I32 vDiff = vNewMousePos - m_vLastMousePos;

  m_vLastMousePos = UpdateMouseMode(e);

  const float fSpeed = 0.02f;

  if (m_ManipulateMode == ManipulateMode::Radius)
  {
    m_fRadius += vDiff.x * fSpeed;
    m_fRadius -= vDiff.y * fSpeed;

    m_fRadius = ezMath::Max(0.0f, m_fRadius);
  }
  else
  {
    m_fLength += vDiff.x * fSpeed;
    m_fLength -= vDiff.y * fSpeed;

    m_fLength = ezMath::Max(0.0f, m_fLength);
  }

  // update the scale
  OnTransformationChanged(GetTransformation());

  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = ezGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  return ezEditorInput::WasExclusivelyHandled;
}

void ezCapsuleGizmo::SetLength(float fRadius)
{
  m_fLength = fRadius;

  // update the scale
  OnTransformationChanged(GetTransformation());
}

void ezCapsuleGizmo::SetRadius(float fRadius)
{
  m_fRadius = fRadius;

  // update the scale
  OnTransformationChanged(GetTransformation());
}
