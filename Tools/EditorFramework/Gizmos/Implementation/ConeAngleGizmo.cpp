#include <PCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/Gizmos/ConeAngleGizmo.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <QDesktopWidget>
#include <QMouseEvent>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezConeAngleGizmo, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezConeAngleGizmo::ezConeAngleGizmo()
{
  m_Angle = ezAngle::Degree(1.0f);
  m_fAngleScale = 1.0f;
  m_fRadius = 1.0f;

  m_ManipulateMode = ManipulateMode::None;

  m_ConeAngle.Configure(this, ezEngineGizmoHandleType::Cone, ezColorLinearUB(200, 200, 0, 128), false);

  SetVisible(false);
  SetTransformation(ezTransform::Identity());
}

void ezConeAngleGizmo::OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_ConeAngle);
}

void ezConeAngleGizmo::OnVisibleChanged(bool bVisible)
{
  m_ConeAngle.SetVisible(bVisible);
}

void ezConeAngleGizmo::OnTransformationChanged(const ezTransform& transform)
{
  ezTransform t = transform;

  t.m_vScale *= ezVec3(1.0f, m_fAngleScale, m_fAngleScale) * m_fRadius;
  m_ConeAngle.SetTransformation(t);
}

void ezConeAngleGizmo::DoFocusLost(bool bCancel)
{
  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = bCancel ? ezGizmoEvent::Type::CancelInteractions : ezGizmoEvent::Type::EndInteractions;
  m_GizmoEvents.Broadcast(ev);

  ezViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_ConeAngle.SetVisible(true);

  m_ManipulateMode = ManipulateMode::None;
}

ezEditorInput ezConeAngleGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return ezEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return ezEditorInput::MayBeHandledByOthers;
  if (e->modifiers() != 0)
    return ezEditorInput::MayBeHandledByOthers;

  if (m_pInteractionGizmoHandle == &m_ConeAngle)
  {
    m_ManipulateMode = ManipulateMode::Angle;
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

ezEditorInput ezConeAngleGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return ezEditorInput::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return ezEditorInput::WasExclusivelyHandled;
}

ezEditorInput ezConeAngleGizmo::DoMouseMoveEvent(QMouseEvent* e)
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

  {
    m_Angle += vDiff.x * aSpeed;
    m_Angle -= vDiff.y * aSpeed;

    m_Angle = ezMath::Clamp(m_Angle, ezAngle(), ezAngle::Degree(179.0f));

    m_fAngleScale = ezMath::Tan(m_Angle * 0.5f);
  }

  // update the scale
  OnTransformationChanged(GetTransformation());

  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = ezGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  return ezEditorInput::WasExclusivelyHandled;
}

void ezConeAngleGizmo::SetAngle(ezAngle angle)
{
  m_Angle = angle;
  m_fAngleScale = ezMath::Tan(m_Angle * 0.5f);

  // update the scale
  OnTransformationChanged(GetTransformation());
}
