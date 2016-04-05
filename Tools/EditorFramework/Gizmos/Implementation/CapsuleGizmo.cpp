#include <PCH.h>
#include <EditorFramework/Gizmos/CapsuleGizmo.h>
#include <EditorFramework/DocumentWindow3D/DocumentWindow3D.moc.h>
#include <Foundation/Logging/Log.h>
#include <CoreUtils/Graphics/Camera.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <EditorFramework/DocumentWindow3D/3DViewWidget.moc.h>
#include <QMouseEvent>
#include <QDesktopWidget>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCapsuleGizmo, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezCapsuleGizmo::ezCapsuleGizmo()
{
  m_fLength = 1.0f;
  m_fRadius = 0.25f;

  m_ManipulateMode = ManipulateMode::None;

  m_Radius.Configure(this, ezEngineGizmoHandleType::CylinderZ, ezColorLinearUB(200, 200, 200, 128), false);
  m_LengthTop.Configure(this, ezEngineGizmoHandleType::HalfSphereZ, ezColorLinearUB(200, 200, 200, 128), false);
  m_LengthBottom.Configure(this, ezEngineGizmoHandleType::HalfSphereZ, ezColorLinearUB(200, 200, 200, 128), false);

  SetVisible(false);
  SetTransformation(ezMat4::IdentityMatrix());
}

void ezCapsuleGizmo::OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView)
{
  m_LengthTop.SetOwner(pOwnerWindow);
  m_LengthBottom.SetOwner(pOwnerWindow);
  m_Radius.SetOwner(pOwnerWindow);
}

void ezCapsuleGizmo::OnVisibleChanged(bool bVisible)
{
  m_LengthTop.SetVisible(bVisible);
  m_LengthBottom.SetVisible(bVisible);
  m_Radius.SetVisible(bVisible);
}

void ezCapsuleGizmo::OnTransformationChanged(const ezMat4& transform)
{
  {
    ezMat4 mScaleCylinder;
    mScaleCylinder.SetScalingMatrix(ezVec3(m_fRadius, m_fRadius, m_fLength));

    m_Radius.SetTransformation(transform * mScaleCylinder);
  }

  {
    ezMat4 mScaleSpheres;
    mScaleSpheres.SetScalingMatrix(ezVec3(m_fRadius));
    mScaleSpheres.SetTranslationVector(ezVec3(0, 0, m_fLength * 0.5f));
    m_LengthTop.SetTransformation(transform * mScaleSpheres);
  }

  {
    ezMat4 mScaleSpheres;
    mScaleSpheres.SetScalingMatrix(ezVec3(m_fRadius, -m_fRadius, -m_fRadius));
    mScaleSpheres.SetTranslationVector(ezVec3(0, 0, -m_fLength * 0.5f));

    /// \todo When m_fRadius goes below 1/3 the GPU gets an incorrect scaling matrix
    /// Position and Scaling values are correct, but the signs for scaling get lost (mirroring?)

    /// \todo Use this for easier debugging
    //mScaleSpheres.SetTranslationVector(ezVec3(1.0f, 0, -m_fLength * 0.5f));

    m_LengthBottom.SetTransformation(transform * mScaleSpheres);
  }
}

void ezCapsuleGizmo::FocusLost(bool bCancel)
{
  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = bCancel ? ezGizmoEvent::Type::CancelInteractions : ezGizmoEvent::Type::EndInteractions;
  m_GizmoEvents.Broadcast(ev);

  ezViewHighlightMsgToEngine msg;
  msg.SendHighlightObjectMessage(GetOwnerWindow()->GetEditorEngineConnection());

  m_LengthTop.SetVisible(true);
  m_LengthBottom.SetVisible(true);
  m_Radius.SetVisible(true);

  m_ManipulateMode = ManipulateMode::None;
}

ezEditorInut ezCapsuleGizmo::mousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return ezEditorInut::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return ezEditorInut::MayBeHandledByOthers;

  if (m_pInteractionGizmoHandle == &m_Radius)
  {
    m_ManipulateMode = ManipulateMode::Radius;
  }
  else if (m_pInteractionGizmoHandle == &m_LengthTop || m_pInteractionGizmoHandle == &m_LengthBottom)
  {
    m_ManipulateMode = ManipulateMode::Length;
  }
  else
    return ezEditorInut::MayBeHandledByOthers;

  ezViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  msg.SendHighlightObjectMessage(GetOwnerWindow()->GetEditorEngineConnection());

  m_LastInteraction = ezTime::Now();

  m_MousePos = ezVec2(e->globalPos().x(), e->globalPos().y());

  SetActiveInputContext(this);

  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = ezGizmoEvent::Type::BeginInteractions;
  m_GizmoEvents.Broadcast(ev);

  return ezEditorInut::WasExclusivelyHandled;
}

ezEditorInut ezCapsuleGizmo::mouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInut::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return ezEditorInut::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return ezEditorInut::WasExclusivelyHandled;
}

ezEditorInut ezCapsuleGizmo::mouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInut::MayBeHandledByOthers;

  const ezTime tNow = ezTime::Now();

  if (tNow - m_LastInteraction < ezTime::Seconds(1.0 / 25.0))
    return ezEditorInut::WasExclusivelyHandled;

  m_LastInteraction = tNow;

  const ezVec2 vNewMousePos = ezVec2(e->globalPos().x(), e->globalPos().y());
  ezVec2 vDiff = vNewMousePos - m_MousePos;

  QCursor::setPos(QPoint(m_MousePos.x, m_MousePos.y));

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

  return ezEditorInut::WasExclusivelyHandled;
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

