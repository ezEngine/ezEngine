#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/SphereGizmo.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSphereGizmo, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezSphereGizmo::ezSphereGizmo()
{
  m_bInnerEnabled = false;

  m_fRadiusInner = 1.0f;
  m_fRadiusOuter = 2.0f;

  m_ManipulateMode = ManipulateMode::None;

  m_hInnerSphere.ConfigureHandle(this, ezEngineGizmoHandleType::Sphere, ezColorLinearUB(200, 200, 0, 128), ezGizmoFlags::OnTop | ezGizmoFlags::Pickable); // this gizmo should be rendered very last so it is always on top
  m_hOuterSphere.ConfigureHandle(this, ezEngineGizmoHandleType::Sphere, ezColorLinearUB(200, 200, 200, 128), ezGizmoFlags::Pickable);

  SetVisible(false);
  SetTransformation(ezTransform::IdentityTransform());
}

void ezSphereGizmo::OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hInnerSphere);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hOuterSphere);
}

void ezSphereGizmo::OnVisibleChanged(bool bVisible)
{
  m_hInnerSphere.SetVisible(bVisible && m_bInnerEnabled);
  m_hOuterSphere.SetVisible(bVisible);
}

void ezSphereGizmo::OnTransformationChanged(const ezTransform& transform)
{
  ezTransform mScaleInner, mScaleOuter;
  mScaleInner.SetIdentity();
  mScaleOuter.SetIdentity();
  mScaleInner.m_vScale = ezVec3(m_fRadiusInner);
  mScaleOuter.m_vScale = ezVec3(m_fRadiusOuter);

  m_hInnerSphere.SetTransformation(transform * mScaleInner);
  m_hOuterSphere.SetTransformation(transform * mScaleOuter);
}

void ezSphereGizmo::DoFocusLost(bool bCancel)
{
  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = bCancel ? ezGizmoEvent::Type::CancelInteractions : ezGizmoEvent::Type::EndInteractions;
  m_GizmoEvents.Broadcast(ev);

  ezViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_hInnerSphere.SetVisible(m_bInnerEnabled);
  m_hOuterSphere.SetVisible(true);

  m_ManipulateMode = ManipulateMode::None;
}

ezEditorInput ezSphereGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return ezEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return ezEditorInput::MayBeHandledByOthers;
  if (e->modifiers() != 0)
    return ezEditorInput::MayBeHandledByOthers;

  if (m_pInteractionGizmoHandle == &m_hInnerSphere)
  {
    m_ManipulateMode = ManipulateMode::InnerSphere;
  }
  else if (m_pInteractionGizmoHandle == &m_hOuterSphere)
  {
    m_ManipulateMode = ManipulateMode::OuterSphere;
  }
  else
    return ezEditorInput::MayBeHandledByOthers;

  ezViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  // m_InnerSphere.SetVisible(false);
  // m_OuterSphere.SetVisible(false);

  // m_pInteractionGizmoHandle->SetVisible(true);

  m_LastInteraction = ezTime::Now();

  m_vLastMousePos = SetMouseMode(ezEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);

  SetActiveInputContext(this);

  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = ezGizmoEvent::Type::BeginInteractions;
  m_GizmoEvents.Broadcast(ev);

  return ezEditorInput::WasExclusivelyHandled;
}

ezEditorInput ezSphereGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return ezEditorInput::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return ezEditorInput::WasExclusivelyHandled;
}

ezEditorInput ezSphereGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInput::MayBeHandledByOthers;

  const ezTime tNow = ezTime::Now();

  if (tNow - m_LastInteraction < ezTime::MakeFromSeconds(1.0 / 25.0))
    return ezEditorInput::WasExclusivelyHandled;

  m_LastInteraction = tNow;

  const QPoint mousePosition = e->globalPosition().toPoint();

  const ezVec2I32 vNewMousePos = ezVec2I32(mousePosition.x(), mousePosition.y());
  const ezVec2I32 vDiff = vNewMousePos - m_vLastMousePos;

  m_vLastMousePos = UpdateMouseMode(e);

  const float fSpeed = 0.02f;

  if (m_ManipulateMode == ManipulateMode::InnerSphere)
  {
    m_fRadiusInner += vDiff.x * fSpeed;
    m_fRadiusInner -= vDiff.y * fSpeed;

    m_fRadiusInner = ezMath::Max(0.0f, m_fRadiusInner);

    m_fRadiusOuter = ezMath::Max(m_fRadiusInner, m_fRadiusOuter);
  }
  else
  {
    m_fRadiusOuter += vDiff.x * fSpeed;
    m_fRadiusOuter -= vDiff.y * fSpeed;

    m_fRadiusOuter = ezMath::Max(0.0f, m_fRadiusOuter);

    m_fRadiusInner = ezMath::Min(m_fRadiusInner, m_fRadiusOuter);
  }

  // update the scale
  OnTransformationChanged(GetTransformation());

  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = ezGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  return ezEditorInput::WasExclusivelyHandled;
}

void ezSphereGizmo::SetInnerSphere(bool bEnabled, float fRadius)
{
  m_fRadiusInner = fRadius;
  m_bInnerEnabled = bEnabled;

  // update the scale
  OnTransformationChanged(GetTransformation());
}

void ezSphereGizmo::SetOuterSphere(float fRadius)
{
  m_fRadiusOuter = fRadius;

  // update the scale
  OnTransformationChanged(GetTransformation());
}
