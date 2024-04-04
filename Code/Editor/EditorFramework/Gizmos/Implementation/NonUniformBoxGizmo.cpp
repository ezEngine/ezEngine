#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Graphics/Camera.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/NonUniformBoxGizmo.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <Foundation/Utilities/GraphicsUtils.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezNonUniformBoxGizmo, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezNonUniformBoxGizmo::ezNonUniformBoxGizmo()
{
  m_vNegSize.Set(1.0f);
  m_vPosSize.Set(1.0f);

  m_ManipulateMode = ManipulateMode::None;

  m_hOutline.ConfigureHandle(this, ezEngineGizmoHandleType::LineBox, ezColorScheme::LightUI(ezColorScheme::Gray), ezGizmoFlags::ShowInOrtho);

  ezColor cols[6] = {
    ezColorScheme::LightUI(ezColorScheme::Red),
    ezColorScheme::LightUI(ezColorScheme::Red),
    ezColorScheme::LightUI(ezColorScheme::Green),
    ezColorScheme::LightUI(ezColorScheme::Green),
    ezColorScheme::LightUI(ezColorScheme::Blue),
    ezColorScheme::LightUI(ezColorScheme::Blue),
  };

  for (ezUInt32 i = 0; i < 6; ++i)
  {
    m_Nobs[i].ConfigureHandle(this, ezEngineGizmoHandleType::Box, cols[i], ezGizmoFlags::ConstantSize | ezGizmoFlags::OnTop | ezGizmoFlags::ShowInOrtho | ezGizmoFlags::Pickable);
  }

  SetVisible(false);
  SetTransformation(ezTransform::MakeIdentity());
}

void ezNonUniformBoxGizmo::OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hOutline);

  for (ezUInt32 i = 0; i < 6; ++i)
  {
    pOwnerWindow->GetDocument()->AddSyncObject(&m_Nobs[i]);
  }
}

void ezNonUniformBoxGizmo::OnVisibleChanged(bool bVisible)
{
  m_hOutline.SetVisible(bVisible);

  for (ezUInt32 i = 0; i < 6; ++i)
  {
    m_Nobs[i].SetVisible(bVisible);
  }
}

void ezNonUniformBoxGizmo::OnTransformationChanged(const ezTransform& transform)
{
  ezMat4 scale, rot;
  scale = ezMat4::MakeScaling(m_vNegSize + m_vPosSize);

  const ezVec3 center = ezMath::Lerp(-m_vNegSize, m_vPosSize, 0.5f);

  scale.SetTranslationVector(center);
  scale = transform.GetAsMat4() * scale;

  m_hOutline.SetTransformation(scale);

  for (ezUInt32 i = 0; i < 6; ++i)
  {
    ezVec3 pos = center;
    ezVec3 dir;

    switch (i)
    {
      case ManipulateMode::DragNegX:
        pos.x = -m_vNegSize.x;
        dir.Set(-1, 0, 0);
        break;
      case ManipulateMode::DragPosX:
        pos.x = m_vPosSize.x;
        dir.Set(+1, 0, 0);
        break;
      case ManipulateMode::DragNegY:
        pos.y = -m_vNegSize.y;
        dir.Set(0, -1, 0);
        break;
      case ManipulateMode::DragPosY:
        pos.y = m_vPosSize.y;
        dir.Set(0, +1, 0);
        break;
      case ManipulateMode::DragNegZ:
        pos.z = -m_vNegSize.z;
        dir.Set(0, 0, -1);
        break;
      case ManipulateMode::DragPosZ:
        pos.z = m_vPosSize.z;
        dir.Set(0, 0, +1);
        break;
    }
    pos = pos.CompMul(transform.m_vScale);

    ezTransform t;
    t.m_qRotation = transform.m_qRotation;
    t.m_vPosition = transform.m_vPosition + t.m_qRotation * pos;
    t.m_vScale.Set(0.15f);

    m_vMainAxis[i] = t.m_qRotation * dir;

    m_Nobs[i].SetTransformation(t);
  }
}

void ezNonUniformBoxGizmo::DoFocusLost(bool bCancel)
{
  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = bCancel ? ezGizmoEvent::Type::CancelInteractions : ezGizmoEvent::Type::EndInteractions;
  m_GizmoEvents.Broadcast(ev);

  ezViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_ManipulateMode = ManipulateMode::None;
}

ezEditorInput ezNonUniformBoxGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return ezEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return ezEditorInput::MayBeHandledByOthers;
  if (e->modifiers() != 0)
    return ezEditorInput::MayBeHandledByOthers;

  for (ezUInt32 i = 0; i < 6; ++i)
  {
    if (m_pInteractionGizmoHandle == &m_Nobs[i])
    {
      m_ManipulateMode = (ManipulateMode)i;
      m_vMoveAxis = m_vMainAxis[i];
      m_vStartPosition = m_pInteractionGizmoHandle->GetTransformation().m_vPosition;
      goto modify;
    }
  }

  return ezEditorInput::MayBeHandledByOthers;

modify:

{
  ezMat4 mView = m_pCamera->GetViewMatrix();
  ezMat4 mProj;
  m_pCamera->GetProjectionMatrix((float)m_vViewport.x / (float)m_vViewport.y, mProj);
  ezMat4 mViewProj = mProj * mView;
  m_mInvViewProj = mViewProj.GetInverse();
}

  m_vStartNegSize = m_vNegSize;
  m_vStartPosSize = m_vPosSize;

  GetPointOnAxis(e->pos().x(), m_vViewport.y - e->pos().y(), m_vInteractionPivot).IgnoreResult();

  ezViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_LastInteraction = ezTime::Now();

  m_vLastMousePos = SetMouseMode(ezEditorInputContext::MouseMode::Normal);

  SetActiveInputContext(this);

  m_fStartScale = (m_vInteractionPivot - m_pCamera->GetPosition()).GetLength() * 0.125;

  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = ezGizmoEvent::Type::BeginInteractions;
  m_GizmoEvents.Broadcast(ev);

  return ezEditorInput::WasExclusivelyHandled;
}

ezEditorInput ezNonUniformBoxGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return ezEditorInput::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return ezEditorInput::WasExclusivelyHandled;
}

ezEditorInput ezNonUniformBoxGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInput::MayBeHandledByOthers;

  const ezTime tNow = ezTime::Now();

  if (tNow - m_LastInteraction < ezTime::MakeFromSeconds(1.0 / 25.0))
    return ezEditorInput::WasExclusivelyHandled;

  m_LastInteraction = tNow;

  m_vNegSize = m_vStartNegSize;
  m_vPosSize = m_vStartPosSize;

  {
    ezVec3 vCurrentInteractionPoint;

    if (GetPointOnAxis(e->pos().x(), m_vViewport.y - e->pos().y(), vCurrentInteractionPoint).Failed())
    {
      m_vLastMousePos = UpdateMouseMode(e);
      return ezEditorInput::WasExclusivelyHandled;
    }

    const float fPerspectiveScale = (vCurrentInteractionPoint - m_pCamera->GetPosition()).GetLength() * 0.125;
    const ezVec3 vOffset = (m_vInteractionPivot - m_vStartPosition);

    const ezVec3 vNewPos = vCurrentInteractionPoint - vOffset * fPerspectiveScale / m_fStartScale;

    ezVec3 vTranslate = GetTransformation().m_qRotation.GetInverse() * (vNewPos - m_vStartPosition);

    // disable snapping when SHIFT is pressed
    if (!e->modifiers().testFlag(Qt::ShiftModifier))
    {
      ezSnapProvider::SnapTranslation(vTranslate);
    }

    switch (m_ManipulateMode)
    {
      case None:
        break;
      case DragNegX:
        m_vNegSize.x -= vTranslate.x;
        if (m_bLinkAxis)
          m_vPosSize.x -= vTranslate.x;
        break;
      case DragPosX:
        m_vPosSize.x += vTranslate.x;
        if (m_bLinkAxis)
          m_vNegSize.x += vTranslate.x;
        break;
      case DragNegY:
        m_vNegSize.y -= vTranslate.y;
        if (m_bLinkAxis)
          m_vPosSize.y -= vTranslate.y;
        break;
      case DragPosY:
        m_vPosSize.y += vTranslate.y;
        if (m_bLinkAxis)
          m_vNegSize.y += vTranslate.y;
        break;
      case DragNegZ:
        m_vNegSize.z -= vTranslate.z;
        if (m_bLinkAxis)
          m_vPosSize.z -= vTranslate.z;
        break;
      case DragPosZ:
        m_vPosSize.z += vTranslate.z;
        if (m_bLinkAxis)
          m_vNegSize.z += vTranslate.z;
        break;
    }
  }

  m_vLastMousePos = UpdateMouseMode(e);

  // update the scale
  OnTransformationChanged(GetTransformation());

  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = ezGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  return ezEditorInput::WasExclusivelyHandled;
}

void ezNonUniformBoxGizmo::SetSize(const ezVec3& vNegSize, const ezVec3& vPosSize, bool bLinkAxis)
{
  m_vNegSize = vNegSize;
  m_vPosSize = vPosSize;
  m_bLinkAxis = bLinkAxis;

  // update the scale
  OnTransformationChanged(GetTransformation());
}

ezResult ezNonUniformBoxGizmo::GetPointOnAxis(ezInt32 iScreenPosX, ezInt32 iScreenPosY, ezVec3& out_Result) const
{
  out_Result = m_vStartPosition;

  ezVec3 vPos, vRayDir;
  if (ezGraphicsUtils::ConvertScreenPosToWorldPos(m_mInvViewProj, 0, 0, m_vViewport.x, m_vViewport.y, ezVec3(iScreenPosX, iScreenPosY, 0), vPos, &vRayDir).Failed())
    return EZ_FAILURE;

  const ezVec3 vDir = m_pCamera->GetDirForwards();

  if (ezMath::Abs(vDir.Dot(m_vMoveAxis)) > 0.999f)
    return EZ_FAILURE;

  const ezVec3 vPlaneTangent = m_vMoveAxis.CrossRH(vDir).GetNormalized();
  const ezVec3 vPlaneNormal = m_vMoveAxis.CrossRH(vPlaneTangent);

  ezPlane Plane;
  Plane = ezPlane::MakeFromNormalAndPoint(vPlaneNormal, m_vStartPosition);

  ezVec3 vIntersection;
  if (m_pCamera->IsPerspective())
  {
    if (!Plane.GetRayIntersection(m_pCamera->GetPosition(), vRayDir, nullptr, &vIntersection))
      return EZ_FAILURE;
  }
  else
  {
    if (!Plane.GetRayIntersectionBiDirectional(vPos - vRayDir, vRayDir, nullptr, &vIntersection))
      return EZ_FAILURE;
  }

  const ezVec3 vDirAlongRay = vIntersection - m_vStartPosition;
  const float fProjectedLength = vDirAlongRay.Dot(m_vMoveAxis);

  out_Result = m_vStartPosition + fProjectedLength * m_vMoveAxis;
  return EZ_SUCCESS;
}
