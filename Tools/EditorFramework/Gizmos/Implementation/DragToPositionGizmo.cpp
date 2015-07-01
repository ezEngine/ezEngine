#include <PCH.h>
#include <EditorFramework/Gizmos/DragToPositionGizmo.h>
#include <EditorFramework/DocumentWindow3D/DocumentWindow3D.moc.h>
#include <Foundation/Logging/Log.h>
#include <QMouseEvent>
#include <CoreUtils/Graphics/Camera.h>
#include <Foundation/Utilities/GraphicsUtils.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDragToPositionGizmo, ezGizmoBase, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezDragToPositionGizmo::ezDragToPositionGizmo()
{
  m_Bobble.Configure(this, ezGizmoHandleType::Box, ezColorLinearUB(64, 64, 255));

  SetVisible(false);
  SetTransformation(ezMat4::IdentityMatrix());
}

void ezDragToPositionGizmo::SetDocumentGuid(const ezUuid& guid)
{
  m_Bobble.SetDocumentGuid(guid);
}

void ezDragToPositionGizmo::OnVisibleChanged(bool bVisible)
{
  m_Bobble.SetVisible(bVisible);
}

void ezDragToPositionGizmo::OnTransformationChanged(const ezMat4& transform)
{
  ezMat4 m;
  m.SetTranslationMatrix(transform.GetTranslationVector());
  m_Bobble.SetTransformation(m);
}

void ezDragToPositionGizmo::FocusLost()
{
  BaseEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = BaseEvent::Type::EndInteractions;
  m_BaseEvents.Broadcast(ev);

  ezViewHighlightMsgToEngine msg;
  msg.SendHighlightObjectMessage(GetDocumentWindow3D()->GetEditorEngineConnection());

  m_Bobble.SetVisible(true);
}

bool ezDragToPositionGizmo::mousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return true;

  if (e->button() != Qt::MouseButton::LeftButton)
    return false;

  ezViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  msg.SendHighlightObjectMessage(GetDocumentWindow3D()->GetEditorEngineConnection());

  //m_Bobble.SetVisible(false);
  //m_pInteractionGizmoHandle->SetVisible(true);

  m_vStartPosition = GetTransformation().GetTranslationVector();

  m_LastInteraction = ezTime::Now();

  SetActiveInputContext(this);

  BaseEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = BaseEvent::Type::BeginInteractions;
  m_BaseEvents.Broadcast(ev);

  return true;
}

bool ezDragToPositionGizmo::mouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return false;

  if (e->button() != Qt::MouseButton::LeftButton)
    return true;

  FocusLost();

  SetActiveInputContext(nullptr);
  return true;
}

bool ezDragToPositionGizmo::mouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return false;

  const ezTime tNow = ezTime::Now();

  if (tNow - m_LastInteraction < ezTime::Seconds(1.0 / 25.0))
    return true;

  m_LastInteraction = tNow;

  const ezObjectPickingResult& res = GetDocumentWindow3D()->PickObject(e->pos().x(), e->pos().y());

  if (res.m_vPickedPosition.IsNaN())
    return true;

  ezMat4 mTrans;
  mTrans.SetTranslationMatrix(res.m_vPickedPosition);

  SetTransformation(mTrans);

  BaseEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = BaseEvent::Type::Interaction;
  m_BaseEvents.Broadcast(ev);

  return true;
}

