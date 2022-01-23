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

  ezEditorPreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezEditorPreferencesUser>();
  m_bUseExperimentalGizmo = !pPreferences->m_bOldGizmos;

  if (m_bUseExperimentalGizmo)
  {
    // TODO: adjust colors for +/- axis
    const ezColor colr1 = ezColorGammaUB(206, 0, 46);
    const ezColor colr2 = ezColorGammaUB(206, 0, 46);
    const ezColor colg1 = ezColorGammaUB(101, 206, 0);
    const ezColor colg2 = ezColorGammaUB(101, 206, 0);
    const ezColor colb1 = ezColorGammaUB(0, 125, 206);
    const ezColor colb2 = ezColorGammaUB(0, 125, 206);
    const ezColor coly = ezColorGammaUB(128, 128, 0);

    m_Bobble.ConfigureHandle(this, ezEngineGizmoHandleType::FromFile, coly, ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable, "Editor/Meshes/DragCenter.obj");
    m_AlignPX.ConfigureHandle(this, ezEngineGizmoHandleType::FromFile, colr1, ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable, "Editor/Meshes/DragArrowPX.obj");
    m_AlignNX.ConfigureHandle(this, ezEngineGizmoHandleType::FromFile, colr2, ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable, "Editor/Meshes/DragArrowNX.obj");
    m_AlignPY.ConfigureHandle(this, ezEngineGizmoHandleType::FromFile, colg1, ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable, "Editor/Meshes/DragArrowPY.obj");
    m_AlignNY.ConfigureHandle(this, ezEngineGizmoHandleType::FromFile, colg2, ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable, "Editor/Meshes/DragArrowNY.obj");
    m_AlignPZ.ConfigureHandle(this, ezEngineGizmoHandleType::FromFile, colb1, ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable, "Editor/Meshes/DragArrowPZ.obj");
    m_AlignNZ.ConfigureHandle(this, ezEngineGizmoHandleType::FromFile, colb2, ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable, "Editor/Meshes/DragArrowNZ.obj");
  }
  else
  {
    const float b = 0.1f;
    const float l = 0.5f;
    const float h = 0.9f;

    m_Bobble.ConfigureHandle(this, ezEngineGizmoHandleType::Box, ezColor::DodgerBlue, ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable);
    m_AlignPX.ConfigureHandle(this, ezEngineGizmoHandleType::HalfPiston, ezColor(h, b, b), ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable);
    m_AlignNX.ConfigureHandle(this, ezEngineGizmoHandleType::HalfPiston, ezColor(l, b, b), ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable);
    m_AlignPY.ConfigureHandle(this, ezEngineGizmoHandleType::HalfPiston, ezColor(b, h, b), ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable);
    m_AlignNY.ConfigureHandle(this, ezEngineGizmoHandleType::HalfPiston, ezColor(b, l, b), ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable);
    m_AlignPZ.ConfigureHandle(this, ezEngineGizmoHandleType::HalfPiston, ezColor(b, b, h), ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable);
    m_AlignNZ.ConfigureHandle(this, ezEngineGizmoHandleType::HalfPiston, ezColor(b, b, l / 3), ezGizmoFlags::ConstantSize | ezGizmoFlags::Pickable);
  }

  SetVisible(false);
  SetTransformation(ezTransform::IdentityTransform());
}

void ezDragToPositionGizmo::UpdateStatusBarText(ezQtEngineDocumentWindow* pWindow)
{
  if (m_pInteractionGizmoHandle != nullptr)
  {
    if (m_pInteractionGizmoHandle == &m_Bobble)
      GetOwnerWindow()->SetPermanentStatusBarMsg(ezFmt("Drag to Position: Center"));
    else if (m_pInteractionGizmoHandle == &m_AlignPX)
      GetOwnerWindow()->SetPermanentStatusBarMsg(ezFmt("Drag to Position: +X"));
    else if (m_pInteractionGizmoHandle == &m_AlignNX)
      GetOwnerWindow()->SetPermanentStatusBarMsg(ezFmt("Drag to Position: -X"));
    else if (m_pInteractionGizmoHandle == &m_AlignPY)
      GetOwnerWindow()->SetPermanentStatusBarMsg(ezFmt("Drag to Position: +Y"));
    else if (m_pInteractionGizmoHandle == &m_AlignNY)
      GetOwnerWindow()->SetPermanentStatusBarMsg(ezFmt("Drag to Position: -Y"));
    else if (m_pInteractionGizmoHandle == &m_AlignPZ)
      GetOwnerWindow()->SetPermanentStatusBarMsg(ezFmt("Drag to Position: +Z"));
    else if (m_pInteractionGizmoHandle == &m_AlignNZ)
      GetOwnerWindow()->SetPermanentStatusBarMsg(ezFmt("Drag to Position: -Z"));
  }
  else
  {
    GetOwnerWindow()->SetPermanentStatusBarMsg(ezFmt("Drag to Position"));
  }
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
  if (m_bUseExperimentalGizmo)
  {
    m_Bobble.SetTransformation(transform);
    m_AlignPX.SetTransformation(transform);
    m_AlignNX.SetTransformation(transform);
    m_AlignPY.SetTransformation(transform);
    m_AlignNY.SetTransformation(transform);
    m_AlignPZ.SetTransformation(transform);
    m_AlignNZ.SetTransformation(transform);
  }
  else
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

static const ezVec3 GetOrthogonalVector(const ezVec3& vDir)
{
  if (ezMath::Abs(vDir.Dot(ezVec3(0, 0, 1))) < 0.999f)
    return -vDir.CrossRH(ezVec3(0, 0, 1));

  return -vDir.CrossRH(ezVec3(1, 0, 0));
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

  ezVec3 vSnappedPosition = res.m_vPickedPosition;

  // disable snapping when ALT is pressed
  if (!e->modifiers().testFlag(Qt::AltModifier))
    ezSnapProvider::SnapTranslation(vSnappedPosition);

  ezTransform mTrans = GetTransformation();
  mTrans.m_vPosition = vSnappedPosition;

  ezQuat rot;
  ezVec3 alignAxis, orthoAxis;

  if (m_pInteractionGizmoHandle == &m_AlignPX)
  {
    alignAxis.Set(1, 0, 0);
    orthoAxis.Set(0, 0, 1);
  }
  else if (m_pInteractionGizmoHandle == &m_AlignNX)
  {
    alignAxis.Set(-1, 0, 0);
    orthoAxis.Set(0, 0, 1);
  }
  else if (m_pInteractionGizmoHandle == &m_AlignPY)
  {
    alignAxis.Set(0, 1, 0);
    orthoAxis.Set(0, 0, 1);
  }
  else if (m_pInteractionGizmoHandle == &m_AlignNY)
  {
    alignAxis.Set(0, -1, 0);
    orthoAxis.Set(0, 0, 1);
  }
  else if (m_pInteractionGizmoHandle == &m_AlignPZ)
  {
    alignAxis.Set(0, 0, 1);
    orthoAxis.Set(1, 0, 0);
  }
  else if (m_pInteractionGizmoHandle == &m_AlignNZ)
  {
    alignAxis.Set(0, 0, -1);
    orthoAxis.Set(1, 0, 0);
  }
  else
  {
    m_bModifiesRotation = false;
    rot.SetIdentity();
  }

  if (m_pInteractionGizmoHandle != &m_Bobble)
  {
    m_bModifiesRotation = true;

    alignAxis = m_qStartOrientation * alignAxis;
    alignAxis.Normalize();

    if (alignAxis.GetAngleBetween(res.m_vPickedNormal) > ezAngle::Degree(179))
    {
      rot.SetFromAxisAndAngle(m_qStartOrientation * orthoAxis, ezAngle::Degree(180));
    }
    else
    {
      rot.SetShortestRotation(alignAxis, res.m_vPickedNormal);
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
