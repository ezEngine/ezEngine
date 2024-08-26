#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/Gizmos/DragToPositionGizmo.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Preferences/EditorPreferences.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDragToPositionGizmo, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezDragToPositionGizmo::ezDragToPositionGizmo()
{
  m_bModifiesRotation = false;

  // TODO: adjust colors for +/- axis
  const ezColor colr1 = ezColorGammaUB(206, 0, 46);
  const ezColor colr2 = ezColorGammaUB(206, 0, 46);
  const ezColor colg1 = ezColorGammaUB(101, 206, 0);
  const ezColor colg2 = ezColorGammaUB(101, 206, 0);
  const ezColor colb1 = ezColorGammaUB(0, 125, 206);
  const ezColor colb2 = ezColorGammaUB(0, 125, 206);
  const ezColor coly = ezColorGammaUB(128, 128, 0);

  m_hBobble.ConfigureHandle(this, ezEngineGizmoHandleType::FromFile, coly, ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable, "Editor/Meshes/DragCenter.obj");
  m_hAlignPX.ConfigureHandle(this, ezEngineGizmoHandleType::FromFile, colr1, ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable, "Editor/Meshes/DragArrowPX.obj");
  m_hAlignNX.ConfigureHandle(this, ezEngineGizmoHandleType::FromFile, colr2, ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable, "Editor/Meshes/DragArrowNX.obj");
  m_hAlignPY.ConfigureHandle(this, ezEngineGizmoHandleType::FromFile, colg1, ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable, "Editor/Meshes/DragArrowPY.obj");
  m_hAlignNY.ConfigureHandle(this, ezEngineGizmoHandleType::FromFile, colg2, ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable, "Editor/Meshes/DragArrowNY.obj");
  m_hAlignPZ.ConfigureHandle(this, ezEngineGizmoHandleType::FromFile, colb1, ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable, "Editor/Meshes/DragArrowPZ.obj");
  m_hAlignNZ.ConfigureHandle(this, ezEngineGizmoHandleType::FromFile, colb2, ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable, "Editor/Meshes/DragArrowNZ.obj");

  SetVisible(false);
  SetTransformation(ezTransform::MakeIdentity());
}

void ezDragToPositionGizmo::UpdateStatusBarText(ezQtEngineDocumentWindow* pWindow)
{
  if (m_pInteractionGizmoHandle != nullptr)
  {
    if (m_pInteractionGizmoHandle == &m_hBobble)
      GetOwnerWindow()->SetPermanentStatusBarMsg(ezFmt("Drag to Position: Center"));
    else if (m_pInteractionGizmoHandle == &m_hAlignPX)
      GetOwnerWindow()->SetPermanentStatusBarMsg(ezFmt("Drag to Position: +X"));
    else if (m_pInteractionGizmoHandle == &m_hAlignNX)
      GetOwnerWindow()->SetPermanentStatusBarMsg(ezFmt("Drag to Position: -X"));
    else if (m_pInteractionGizmoHandle == &m_hAlignPY)
      GetOwnerWindow()->SetPermanentStatusBarMsg(ezFmt("Drag to Position: +Y"));
    else if (m_pInteractionGizmoHandle == &m_hAlignNY)
      GetOwnerWindow()->SetPermanentStatusBarMsg(ezFmt("Drag to Position: -Y"));
    else if (m_pInteractionGizmoHandle == &m_hAlignPZ)
      GetOwnerWindow()->SetPermanentStatusBarMsg(ezFmt("Drag to Position: +Z"));
    else if (m_pInteractionGizmoHandle == &m_hAlignNZ)
      GetOwnerWindow()->SetPermanentStatusBarMsg(ezFmt("Drag to Position: -Z"));
  }
  else
  {
    GetOwnerWindow()->SetPermanentStatusBarMsg(ezFmt("Drag to Position"));
  }
}

void ezDragToPositionGizmo::OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hBobble);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAlignPX);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAlignNX);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAlignPY);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAlignNY);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAlignPZ);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAlignNZ);
}

void ezDragToPositionGizmo::OnVisibleChanged(bool bVisible)
{
  m_hBobble.SetVisible(bVisible);
  m_hAlignPX.SetVisible(bVisible);
  m_hAlignNX.SetVisible(bVisible);
  m_hAlignPY.SetVisible(bVisible);
  m_hAlignNY.SetVisible(bVisible);
  m_hAlignPZ.SetVisible(bVisible);
  m_hAlignNZ.SetVisible(bVisible);
}

void ezDragToPositionGizmo::OnTransformationChanged(const ezTransform& transform)
{
  m_hBobble.SetTransformation(transform);
  m_hAlignPX.SetTransformation(transform);
  m_hAlignNX.SetTransformation(transform);
  m_hAlignPY.SetTransformation(transform);
  m_hAlignNY.SetTransformation(transform);
  m_hAlignPZ.SetTransformation(transform);
  m_hAlignNZ.SetTransformation(transform);
}

void ezDragToPositionGizmo::DoFocusLost(bool bCancel)
{
  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = bCancel ? ezGizmoEvent::Type::CancelInteractions : ezGizmoEvent::Type::EndInteractions;
  m_GizmoEvents.Broadcast(ev);

  ezViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_hBobble.SetVisible(true);
  m_hAlignPX.SetVisible(true);
  m_hAlignNX.SetVisible(true);
  m_hAlignPY.SetVisible(true);
  m_hAlignNY.SetVisible(true);
  m_hAlignPZ.SetVisible(true);
  m_hAlignNZ.SetVisible(true);

  m_pInteractionGizmoHandle = nullptr;
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
  // m_Bobble.SetVisible(false);
  // m_AlignPX.SetVisible(false);
  // m_AlignNX.SetVisible(false);
  // m_AlignPY.SetVisible(false);
  // m_AlignNY.SetVisible(false);
  // m_AlignPZ.SetVisible(false);
  // m_AlignNZ.SetVisible(false);
  // m_pInteractionGizmoHandle->SetVisible(true);

  m_vStartPosition = GetTransformation().m_vPosition;
  m_qStartOrientation = GetTransformation().m_qRotation;

  m_LastInteraction = ezTime::Now();

  SetActiveInputContext(this);

  UpdateStatusBarText(nullptr);

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

ezEditorInput ezDragToPositionGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInput::MayBeHandledByOthers;

  const ezTime tNow = ezTime::Now();

  if (tNow - m_LastInteraction < ezTime::MakeFromSeconds(1.0 / 25.0))
    return ezEditorInput::WasExclusivelyHandled;

  m_LastInteraction = tNow;

  const ezObjectPickingResult& res = GetOwnerView()->PickObject(e->pos().x(), e->pos().y());

  if (!res.m_PickedObject.IsValid())
    return ezEditorInput::WasExclusivelyHandled;

  if (res.m_vPickedPosition.IsNaN() || res.m_vPickedNormal.IsNaN() || res.m_vPickedNormal.IsZero())
    return ezEditorInput::WasExclusivelyHandled;

  ezVec3 vSnappedPosition = res.m_vPickedPosition;

  // disable snapping when SHIFT is pressed
  if (!e->modifiers().testFlag(Qt::ShiftModifier))
    ezSnapProvider::SnapTranslation(vSnappedPosition);

  ezTransform mTrans = GetTransformation();
  mTrans.m_vPosition = vSnappedPosition;

  ezQuat rot;
  ezVec3 alignAxis, orthoAxis;

  if (m_pInteractionGizmoHandle == &m_hAlignPX)
  {
    alignAxis.Set(1, 0, 0);
    orthoAxis.Set(0, 0, 1);
  }
  else if (m_pInteractionGizmoHandle == &m_hAlignNX)
  {
    alignAxis.Set(-1, 0, 0);
    orthoAxis.Set(0, 0, 1);
  }
  else if (m_pInteractionGizmoHandle == &m_hAlignPY)
  {
    alignAxis.Set(0, 1, 0);
    orthoAxis.Set(0, 0, 1);
  }
  else if (m_pInteractionGizmoHandle == &m_hAlignNY)
  {
    alignAxis.Set(0, -1, 0);
    orthoAxis.Set(0, 0, 1);
  }
  else if (m_pInteractionGizmoHandle == &m_hAlignPZ)
  {
    alignAxis.Set(0, 0, 1);
    orthoAxis.Set(1, 0, 0);
  }
  else if (m_pInteractionGizmoHandle == &m_hAlignNZ)
  {
    alignAxis.Set(0, 0, -1);
    orthoAxis.Set(1, 0, 0);
  }
  else
  {
    m_bModifiesRotation = false;
    rot.SetIdentity();
  }

  if (m_pInteractionGizmoHandle != &m_hBobble)
  {
    m_bModifiesRotation = true;

    alignAxis = m_qStartOrientation * alignAxis;
    alignAxis.Normalize();

    if (alignAxis.GetAngleBetween(res.m_vPickedNormal) > ezAngle::MakeFromDegree(179))
    {
      rot = ezQuat::MakeFromAxisAndAngle(m_qStartOrientation * orthoAxis, ezAngle::MakeFromDegree(180));
    }
    else
    {
      rot = ezQuat::MakeShortestRotation(alignAxis, res.m_vPickedNormal);
    }
  }

  mTrans.m_qRotation = rot * m_qStartOrientation;
  SetTransformation(mTrans);

  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = ezGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  return ezEditorInput::WasExclusivelyHandled;
}
