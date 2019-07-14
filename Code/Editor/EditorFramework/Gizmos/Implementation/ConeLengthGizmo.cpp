#include <EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/Gizmos/ConeLengthGizmo.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <QDesktopWidget>
#include <QMouseEvent>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezConeLengthGizmo, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezConeLengthGizmo::ezConeLengthGizmo()
{
  m_fRadius = 1.0f;
  m_fRadiusScale = 0.1f;

  m_ManipulateMode = ManipulateMode::None;

  m_ConeRadius.Configure(this, ezEngineGizmoHandleType::Cone, ezColorLinearUB(200, 200, 200, 128), false,
                         true); // this gizmo should be rendered very last so it is always on top

  SetVisible(false);
  SetTransformation(ezTransform::IdentityTransform());
}

void ezConeLengthGizmo::OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_ConeRadius);
}

void ezConeLengthGizmo::OnVisibleChanged(bool bVisible)
{
  m_ConeRadius.SetVisible(bVisible);
}

void ezConeLengthGizmo::OnTransformationChanged(const ezTransform& transform)
{
  ezTransform t = transform;
  t.m_vScale *= ezVec3(1.0f, m_fRadiusScale, m_fRadiusScale) * m_fRadius;

  m_ConeRadius.SetTransformation(t);
}

void ezConeLengthGizmo::DoFocusLost(bool bCancel)
{
  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = bCancel ? ezGizmoEvent::Type::CancelInteractions : ezGizmoEvent::Type::EndInteractions;
  m_GizmoEvents.Broadcast(ev);

  ezViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_ConeRadius.SetVisible(true);

  m_ManipulateMode = ManipulateMode::None;
}

ezEditorInput ezConeLengthGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return ezEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return ezEditorInput::MayBeHandledByOthers;
  if (e->modifiers() != 0)
    return ezEditorInput::MayBeHandledByOthers;

  if (m_pInteractionGizmoHandle == &m_ConeRadius)
  {
    m_ManipulateMode = ManipulateMode::Radius;
  }
  else
    return ezEditorInput::MayBeHandledByOthers;

  ezViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_LastInteraction = ezTime::Now();

  m_LastMousePos = SetMouseMode(ezEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);

  SetActiveInputContext(this);

  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = ezGizmoEvent::Type::BeginInteractions;
  m_GizmoEvents.Broadcast(ev);

  return ezEditorInput::WasExclusivelyHandled;
}

ezEditorInput ezConeLengthGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return ezEditorInput::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return ezEditorInput::WasExclusivelyHandled;
}

ezEditorInput ezConeLengthGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInput::MayBeHandledByOthers;

  const ezTime tNow = ezTime::Now();

  if (tNow - m_LastInteraction < ezTime::Seconds(1.0 / 25.0))
    return ezEditorInput::WasExclusivelyHandled;

  m_LastInteraction = tNow;

  const ezVec2I32 vNewMousePos = ezVec2I32(e->globalPos().x(), e->globalPos().y());
  const ezVec2I32 vDiff = vNewMousePos - m_LastMousePos;

  m_LastMousePos = UpdateMouseMode(e);

  const float fSpeed = 0.02f;
  const ezAngle aSpeed = ezAngle::Degree(1.0f);

  if (m_ManipulateMode == ManipulateMode::Radius)
  {
    m_fRadius += vDiff.x * fSpeed;
    m_fRadius -= vDiff.y * fSpeed;

    m_fRadius = ezMath::Max(0.0f, m_fRadius);
  }

  // update the scale
  OnTransformationChanged(GetTransformation());

  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = ezGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  return ezEditorInput::WasExclusivelyHandled;
}

void ezConeLengthGizmo::SetRadius(float fRadius)
{
  m_fRadius = fRadius;

  // update the scale
  OnTransformationChanged(GetTransformation());
}
