#include <PCH.h>
#include <EditorFramework/Gizmos/DragToPositionGizmo.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <Foundation/Logging/Log.h>
#include <QMouseEvent>
#include <Core/Graphics/Camera.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/Assets/AssetDocument.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDragToPositionGizmo, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezDragToPositionGizmo::ezDragToPositionGizmo()
{
  m_bModifiesRotation = false;

  m_Bobble.Configure(this, ezEngineGizmoHandleType::Box, ezColor::DodgerBlue);
  m_AlignPX.Configure(this, ezEngineGizmoHandleType::HalfPiston, ezColor::SteelBlue);
  m_AlignNX.Configure(this, ezEngineGizmoHandleType::HalfPiston, ezColor::SteelBlue);
  m_AlignPY.Configure(this, ezEngineGizmoHandleType::HalfPiston, ezColor::SteelBlue);
  m_AlignNY.Configure(this, ezEngineGizmoHandleType::HalfPiston, ezColor::SteelBlue);
  m_AlignPZ.Configure(this, ezEngineGizmoHandleType::HalfPiston, ezColor::SteelBlue);
  m_AlignNZ.Configure(this, ezEngineGizmoHandleType::HalfPiston, ezColor::SteelBlue);

  SetVisible(false);
  SetTransformation(ezTransform::Identity());
}

void ezDragToPositionGizmo::OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_Bobble);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_AlignPX);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_AlignNX);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_AlignPY);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_AlignNY);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_AlignPZ);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_AlignNZ);
}

void ezDragToPositionGizmo::OnVisibleChanged(bool bVisible)
{
  m_Bobble.SetVisible(bVisible);
  m_AlignPX.SetVisible(bVisible);
  m_AlignNX.SetVisible(bVisible);
  m_AlignPY.SetVisible(bVisible);
  m_AlignNY.SetVisible(bVisible);
  m_AlignPZ.SetVisible(bVisible);
  m_AlignNZ.SetVisible(bVisible);
}

void ezDragToPositionGizmo::OnTransformationChanged(const ezTransform& transform)
{
  ezTransform m;
  m.SetIdentity();

  m.m_vScale = ezVec3(0.2f);
  m_Bobble.SetTransformation(transform * m);

  m.SetIdentity();
  m_AlignPX.SetTransformation(transform * m);
  m.m_qRotation.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(180));
  m_AlignNX.SetTransformation(transform * m);

  m.m_qRotation.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(+90));
  m_AlignPY.SetTransformation(transform * m);
  m.m_qRotation.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(-90));
  m_AlignNY.SetTransformation(transform * m);

  m.m_qRotation.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(-90));
  m_AlignPZ.SetTransformation(transform * m);
  m.m_qRotation.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(+90));
  m_AlignNZ.SetTransformation(transform * m);

}

void ezDragToPositionGizmo::DoFocusLost(bool bCancel)
{
  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = bCancel ? ezGizmoEvent::Type::CancelInteractions : ezGizmoEvent::Type::EndInteractions;
  m_GizmoEvents.Broadcast(ev);

  ezViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_Bobble.SetVisible(true);
  m_AlignPX.SetVisible(true);
  m_AlignNX.SetVisible(true);
  m_AlignPY.SetVisible(true);
  m_AlignNY.SetVisible(true);
  m_AlignPZ.SetVisible(true);
  m_AlignNZ.SetVisible(true);
}

ezEditorInput ezDragToPositionGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return ezEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return ezEditorInput::MayBeHandledByOthers;

  ezViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  // The gizmo is actually "hidden" somewhere else during dragging,
  // because it musn't be rendered into the picking buffer, to avoid picking against the gizmo
  //m_Bobble.SetVisible(false);
  //m_AlignPX.SetVisible(false);
  //m_AlignNX.SetVisible(false);
  //m_AlignPY.SetVisible(false);
  //m_AlignNY.SetVisible(false);
  //m_AlignPZ.SetVisible(false);
  //m_AlignNZ.SetVisible(false);
  //m_pInteractionGizmoHandle->SetVisible(true);

  m_vStartPosition = GetTransformation().m_vPosition;

  m_LastInteraction = ezTime::Now();

  SetActiveInputContext(this);

  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = ezGizmoEvent::Type::BeginInteractions;
  m_GizmoEvents.Broadcast(ev);

  return ezEditorInput::WasExclusivelyHandled;
}

ezEditorInput ezDragToPositionGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return ezEditorInput::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return ezEditorInput::WasExclusivelyHandled;
}

static const ezVec3 GetOrthogonalVector(const ezVec3& vDir)
{
  if (ezMath::Abs(vDir.Dot(ezVec3(0, 0, 1))) < 0.999f)
    return -vDir.Cross(ezVec3(0, 0, 1));

  return -vDir.Cross(ezVec3(1, 0, 0));
}

ezEditorInput ezDragToPositionGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInput::MayBeHandledByOthers;

  const ezTime tNow = ezTime::Now();

  if (tNow - m_LastInteraction < ezTime::Seconds(1.0 / 25.0))
    return ezEditorInput::WasExclusivelyHandled;

  m_LastInteraction = tNow;

  const ezObjectPickingResult& res = GetOwnerView()->PickObject(e->pos().x(), e->pos().y());

  if (!res.m_PickedObject.IsValid())
    return ezEditorInput::WasExclusivelyHandled;

  if (res.m_vPickedPosition.IsNaN() || res.m_vPickedNormal.IsNaN() || res.m_vPickedNormal.IsZero())
    return ezEditorInput::WasExclusivelyHandled;

  const ezVec3 vTangent = GetOrthogonalVector(res.m_vPickedNormal).GetNormalized();
  const ezVec3 vBiTangent = res.m_vPickedNormal.Cross(vTangent).GetNormalized();

  ezVec3 vSnappedPosition = res.m_vPickedPosition;

  // disable snapping when ALT is pressed
  if (!e->modifiers().testFlag(Qt::AltModifier))
    ezSnapProvider::SnapTranslation(vSnappedPosition);

  ezMat3 mRot;
  ezTransform mTrans = GetTransformation();
  mTrans.m_vPosition = vSnappedPosition;

  m_bModifiesRotation = true;

  if (m_pInteractionGizmoHandle == &m_AlignPX)
  {
    mRot.SetColumn(0, res.m_vPickedNormal);
    mRot.SetColumn(1, vTangent);
    mRot.SetColumn(2, vBiTangent);
  }
  else if (m_pInteractionGizmoHandle == &m_AlignNX)
  {
    mRot.SetColumn(0, -res.m_vPickedNormal);
    mRot.SetColumn(2, vBiTangent);
    mRot.SetColumn(1, -vTangent);
  }
  else if (m_pInteractionGizmoHandle == &m_AlignPY)
  {
    mRot.SetColumn(0, -vTangent);
    mRot.SetColumn(1, res.m_vPickedNormal);
    mRot.SetColumn(2, vBiTangent);
  }
  else if (m_pInteractionGizmoHandle == &m_AlignNY)
  {
    mRot.SetColumn(0, vTangent);
    mRot.SetColumn(1, -res.m_vPickedNormal);
    mRot.SetColumn(2, vBiTangent);
  }
  else if (m_pInteractionGizmoHandle == &m_AlignPZ)
  {
    mRot.SetColumn(0, vTangent);
    mRot.SetColumn(1, vBiTangent);
    mRot.SetColumn(2, res.m_vPickedNormal);
  }
  else if (m_pInteractionGizmoHandle == &m_AlignNZ)
  {
    mRot.SetColumn(0, -vTangent);
    mRot.SetColumn(1, vBiTangent);
    mRot.SetColumn(2, -res.m_vPickedNormal);
  }
  else
  {
    m_bModifiesRotation = false;
    mRot.SetIdentity();
  }

  mTrans.m_qRotation.SetFromMat3(mRot);
  SetTransformation(mTrans);

  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = ezGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  return ezEditorInput::WasExclusivelyHandled;
}

