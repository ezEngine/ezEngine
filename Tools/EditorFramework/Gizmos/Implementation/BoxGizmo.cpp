#include <PCH.h>
#include <EditorFramework/Gizmos/BoxGizmo.h>
#include <EditorFramework/DocumentWindow3D/DocumentWindow3D.moc.h>
#include <Foundation/Logging/Log.h>
#include <CoreUtils/Graphics/Camera.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <EditorFramework/DocumentWindow3D/3DViewWidget.moc.h>
#include <QMouseEvent>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezBoxGizmo, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezBoxGizmo::ezBoxGizmo()
{
  m_vSize.Set(1.0f);

  m_ManipulateMode = ManipulateMode::None;

  m_Corners.Configure(this, ezEngineGizmoHandleType::BoxCorners, ezColorLinearUB(200, 200, 200, 128), false);

  for (int i = 0; i < 3; ++i)
  {
    m_Edges[i].Configure(this, ezEngineGizmoHandleType::BoxEdges, ezColorLinearUB(200, 200, 200, 128), false);
    m_Faces[i].Configure(this, ezEngineGizmoHandleType::BoxFaces, ezColorLinearUB(200, 200, 200, 128), false);
  }

  SetVisible(false);
  SetTransformation(ezMat4::IdentityMatrix());
}

void ezBoxGizmo::OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView)
{
  m_Corners.SetOwner(pOwnerWindow);

  for (int i = 0; i < 3; ++i)
  {
    m_Edges[i].SetOwner(pOwnerWindow);
    m_Faces[i].SetOwner(pOwnerWindow);
  }
}

void ezBoxGizmo::OnVisibleChanged(bool bVisible)
{
  m_Corners.SetVisible(bVisible);

  for (int i = 0; i < 3; ++i)
  {
    m_Edges[i].SetVisible(bVisible);
    m_Faces[i].SetVisible(bVisible);
  }
}

void ezBoxGizmo::OnTransformationChanged(const ezMat4& transform)
{
  ezMat4 scale, rot;
  scale.SetScalingMatrix(m_vSize);
  scale = transform * scale;

  m_Corners.SetTransformation(scale);

  rot.SetRotationMatrixX(ezAngle::Degree(90));
  m_Edges[0].SetTransformation(scale * rot);

  rot.SetRotationMatrixY(ezAngle::Degree(90));
  m_Faces[0].SetTransformation(scale * rot);

  rot.SetIdentity();
  m_Edges[1].SetTransformation(scale * rot);

  rot.SetRotationMatrixX(ezAngle::Degree(90));
  m_Faces[1].SetTransformation(scale * rot);

  rot.SetRotationMatrixZ(ezAngle::Degree(90));
  m_Edges[2].SetTransformation(scale * rot);

  rot.SetIdentity();
  m_Faces[2].SetTransformation(scale * rot);
}

void ezBoxGizmo::FocusLost(bool bCancel)
{
  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = bCancel ? ezGizmoEvent::Type::CancelInteractions : ezGizmoEvent::Type::EndInteractions;
  m_GizmoEvents.Broadcast(ev);

  ezViewHighlightMsgToEngine msg;
  msg.SendHighlightObjectMessage(GetOwnerWindow()->GetEditorEngineConnection());

  m_ManipulateMode = ManipulateMode::None;
}

ezEditorInut ezBoxGizmo::mousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return ezEditorInut::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return ezEditorInut::MayBeHandledByOthers;

  if (m_pInteractionGizmoHandle == &m_Corners)
  {
    m_ManipulateMode = ManipulateMode::Uniform;
  }
  else if (m_pInteractionGizmoHandle == &m_Faces[0])
  {
    m_ManipulateMode = ManipulateMode::AxisX;
  }
  else if (m_pInteractionGizmoHandle == &m_Faces[1])
  {
    m_ManipulateMode = ManipulateMode::AxisY;
  }
  else if (m_pInteractionGizmoHandle == &m_Faces[2])
  {
    m_ManipulateMode = ManipulateMode::AxisZ;
  }
  else if (m_pInteractionGizmoHandle == &m_Edges[0])
  {
    m_ManipulateMode = ManipulateMode::PlaneXY;
  }
  else if (m_pInteractionGizmoHandle == &m_Edges[1])
  {
    m_ManipulateMode = ManipulateMode::PlaneXZ;
  }
  else if (m_pInteractionGizmoHandle == &m_Edges[2])
  {
    m_ManipulateMode = ManipulateMode::PlaneYZ;
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

ezEditorInut ezBoxGizmo::mouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInut::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return ezEditorInut::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return ezEditorInut::WasExclusivelyHandled;
}

ezEditorInut ezBoxGizmo::mouseMoveEvent(QMouseEvent* e)
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
  float fChange = 0.0f;

  {
    fChange += vDiff.x * fSpeed;
    fChange -= vDiff.y * fSpeed;
  }

  ezVec3 vChange(0);

  if (m_ManipulateMode == ManipulateMode::Uniform)
    vChange.Set(fChange);
  if (m_ManipulateMode == ManipulateMode::PlaneXY)
    vChange.Set(fChange, fChange, 0);
  if (m_ManipulateMode == ManipulateMode::PlaneXZ)
    vChange.Set(fChange, 0, fChange);
  if (m_ManipulateMode == ManipulateMode::PlaneYZ)
    vChange.Set(0, fChange, fChange);
  if (m_ManipulateMode == ManipulateMode::AxisX)
    vChange.Set(fChange, 0, 0);
  if (m_ManipulateMode == ManipulateMode::AxisY)
    vChange.Set(0, fChange, 0);
  if (m_ManipulateMode == ManipulateMode::AxisZ)
    vChange.Set(0, 0, fChange);

  m_vSize += vChange;
  m_vSize.x = ezMath::Max(m_vSize.x, 0.0f);
  m_vSize.y = ezMath::Max(m_vSize.y, 0.0f);
  m_vSize.z = ezMath::Max(m_vSize.z, 0.0f);

  // update the scale
  OnTransformationChanged(GetTransformation());

  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = ezGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  return ezEditorInut::WasExclusivelyHandled;
}

void ezBoxGizmo::SetSize(const ezVec3& size)
{
  m_vSize = size;

  // update the scale
  OnTransformationChanged(GetTransformation());
}

