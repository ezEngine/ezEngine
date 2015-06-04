#include <PCH.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>
#include <Foundation/Logging/Log.h>
#include <QMouseEvent>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTranslateGizmo, ezGizmoBase, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezTranslateGizmo::ezTranslateGizmo()
{
  m_AxisX.Configure(this, ezGizmoHandleType::Arrow, ezColorLinearUB(128, 0, 0));
  m_AxisY.Configure(this, ezGizmoHandleType::Arrow, ezColorLinearUB(0, 128, 0));
  m_AxisZ.Configure(this, ezGizmoHandleType::Arrow, ezColorLinearUB(0, 0, 128));

  SetVisible(false);
  SetTransformation(ezMat4::IdentityMatrix());
}

void ezTranslateGizmo::SetDocumentGuid(const ezUuid& guid)
{
  m_AxisX.SetDocumentGuid(guid);
  m_AxisY.SetDocumentGuid(guid);
  m_AxisZ.SetDocumentGuid(guid);
}

void ezTranslateGizmo::OnVisibleChanged(bool bVisible)
{
  m_AxisX.SetVisible(bVisible);
  m_AxisY.SetVisible(bVisible);
  m_AxisZ.SetVisible(bVisible);
}

void ezTranslateGizmo::OnTransformationChanged(const ezMat4& transform)
{
  ezMat4 m;

  m.SetRotationMatrixZ(ezAngle::Degree(-90));
  m_AxisX.SetTransformation(transform * m);

  m.SetIdentity();
  m_AxisY.SetTransformation(transform * m);

  m.SetRotationMatrixX(ezAngle::Degree(90));
  m_AxisZ.SetTransformation(transform * m);
}

void ezTranslateGizmo::FocusLost()
{
  BaseEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = BaseEvent::Type::EndInteractions;
  m_BaseEvents.Broadcast(ev);
}

bool ezTranslateGizmo::mousePressEvent(QMouseEvent* e)
{
  if (m_pInteractionGizmoHandle == &m_AxisX)
    m_vMoveAxis = m_AxisX.GetTransformation().GetColumn(1).GetAsVec3();
  else if (m_pInteractionGizmoHandle == &m_AxisY)
    m_vMoveAxis = m_AxisY.GetTransformation().GetColumn(1).GetAsVec3();
  else if (m_pInteractionGizmoHandle == &m_AxisZ)
    m_vMoveAxis = m_AxisZ.GetTransformation().GetColumn(1).GetAsVec3();
  else
    return false;

  SetActiveInputContext(this);

  m_iMousePosX = e->globalPos().x();
  m_iMousePosY = e->globalPos().y();

  BaseEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = BaseEvent::Type::BeginInteractions;
  m_BaseEvents.Broadcast(ev);

  return true;
}

bool ezTranslateGizmo::mouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return false;

  BaseEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = BaseEvent::Type::EndInteractions;
  m_BaseEvents.Broadcast(ev);

  SetActiveInputContext(nullptr);
  return true;
}

bool ezTranslateGizmo::mouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return false;

  const ezInt32 iNewPosX = e->globalPos().x();
  const ezInt32 iNewPosY = e->globalPos().y();

  const ezInt32 iDiffX = iNewPosX - m_iMousePosX;
  const ezInt32 iDiffY = iNewPosY - m_iMousePosY;

  m_iMousePosX = iNewPosX;
  m_iMousePosY = iNewPosY;

  const ezVec3 vMove = m_vMoveAxis * (float) iDiffX * 0.01f;

  ezMat4 mTrans;
  mTrans.SetTranslationMatrix(vMove);

  SetTransformation(mTrans * GetTransformation());

  BaseEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = BaseEvent::Type::Interaction;
  m_BaseEvents.Broadcast(ev);

  return true;
}

