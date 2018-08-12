#include <PCH.h>

#include <Core/Graphics/Camera.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/Gizmos/DrawBoxGizmo.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <QMouseEvent>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDrawBoxGizmo, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezDrawBoxGizmo::ezDrawBoxGizmo()
{
  m_ManipulateMode = ManipulateMode::None;

  m_vLastStartPoint.SetZero();
  m_Box.Configure(this, ezEngineGizmoHandleType::LineBox, ezColorLinearUB(255, 100, 0), false, false, false, true, false);

  SetVisible(false);
  SetTransformation(ezTransform::Identity());
}

ezDrawBoxGizmo::~ezDrawBoxGizmo() {}

void ezDrawBoxGizmo::OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_Box);
}

void ezDrawBoxGizmo::OnVisibleChanged(bool bVisible) {}

void ezDrawBoxGizmo::OnTransformationChanged(const ezTransform& transform) {}

void ezDrawBoxGizmo::DoFocusLost(bool bCancel)
{
  ezViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_ManipulateMode = ManipulateMode::None;
  UpdateBox();

  if (IsActiveInputContext())
    SetActiveInputContext(nullptr);
}

ezEditorInput ezDrawBoxGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (e->buttons() == Qt::LeftButton && e->modifiers() == Qt::ControlModifier)
  {
    if (m_ManipulateMode == ManipulateMode::None)
    {
      const QPoint mousePos = GetOwnerWindow()->mapFromGlobal(QCursor::pos());

      const ezObjectPickingResult& res = GetOwnerView()->PickObject(mousePos.x(), mousePos.y());

      m_vUpAxis = GetOwnerView()->GetFallbackPickingPlane().m_vNormal;
      m_vUpAxis.x = ezMath::Abs(m_vUpAxis.x);
      m_vUpAxis.y = ezMath::Abs(m_vUpAxis.y);
      m_vUpAxis.z = ezMath::Abs(m_vUpAxis.z);

      if (res.m_PickedObject.IsValid())
      {
        m_vCurrentPosition = res.m_vPickedPosition;
        m_vLastStartPoint = res.m_vPickedPosition;
      }
      else
      {
        if (GetOwnerView()
                ->PickPlane(e->pos().x(), e->pos().y(), GetOwnerView()->GetFallbackPickingPlane(m_vLastStartPoint), m_vCurrentPosition)
                .Failed())
          return ezEditorInput::WasExclusivelyHandled; // failed to pick anything
      }

      ezSnapProvider::SnapTranslation(m_vCurrentPosition);

      SwitchMode(false);
      return ezEditorInput::WasExclusivelyHandled;
    }
  }

  return ezEditorInput::MayBeHandledByOthers;
}

ezEditorInput ezDrawBoxGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInput::MayBeHandledByOthers;

  if (e->button() == Qt::LeftButton)
  {
    if (m_ManipulateMode == ManipulateMode::DrawBase || m_ManipulateMode == ManipulateMode::DrawHeight)
    {
      SwitchMode(m_vFirstCorner == m_vSecondCorner);
      return ezEditorInput::WasExclusivelyHandled;
    }
  }

  return ezEditorInput::MayBeHandledByOthers;
}

ezEditorInput ezDrawBoxGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInput::MayBeHandledByOthers;

  if (m_ManipulateMode == ManipulateMode::DrawHeight)
  {
    const ezVec2I32 vMouseMove = ezVec2I32(e->globalPos().x(), e->globalPos().y()) - m_LastMousePos;
    m_iHeightChange -= vMouseMove.y;

    m_LastMousePos = UpdateMouseMode(e);
  }
  else
  {
    ezPlane plane;
    plane.SetFromNormalAndPoint(m_vUpAxis, m_vFirstCorner);

    GetOwnerView()->PickPlane(e->pos().x(), e->pos().y(), plane, m_vCurrentPosition);

    ezSnapProvider::SnapTranslation(m_vCurrentPosition);
  }

  UpdateBox();

  return ezEditorInput::WasExclusivelyHandled;
}

ezEditorInput ezDrawBoxGizmo::DoKeyPressEvent(QKeyEvent* e)
{
  // is the gizmo in general visible == is it active
  if (!IsVisible())
    return ezEditorInput::MayBeHandledByOthers;

  if (e->key() == Qt::Key_Escape)
  {
    if (m_ManipulateMode != ManipulateMode::None)
    {
      SwitchMode(true);
      return ezEditorInput::WasExclusivelyHandled;
    }
  }

  return ezEditorInput::MayBeHandledByOthers;
}

ezEditorInput ezDrawBoxGizmo::DoKeyReleaseEvent(QKeyEvent* e)
{
  return ezEditorInput::MayBeHandledByOthers;
}

void ezDrawBoxGizmo::SwitchMode(bool bCancel)
{
  ezGizmoEvent e;
  e.m_pGizmo = this;

  if (bCancel)
  {
    FocusLost(true);

    e.m_Type = ezGizmoEvent::Type::CancelInteractions;
    m_GizmoEvents.Broadcast(e);
    return;
  }

  if (m_ManipulateMode == ManipulateMode::None)
  {
    m_ManipulateMode = ManipulateMode::DrawBase;
    m_vFirstCorner = m_vCurrentPosition;

    SetActiveInputContext(this);
    UpdateBox();

    e.m_Type = ezGizmoEvent::Type::BeginInteractions;
    m_GizmoEvents.Broadcast(e);
    return;
  }

  if (m_ManipulateMode == ManipulateMode::DrawBase)
  {
    m_ManipulateMode = ManipulateMode::DrawHeight;
    m_iHeightChange = 0;
    m_fOriginalBoxHeight = m_fBoxHeight;
    m_LastMousePos = SetMouseMode(ezEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);
    UpdateBox();
    return;
  }

  if (m_ManipulateMode == ManipulateMode::DrawHeight)
  {
    e.m_Type = ezGizmoEvent::Type::EndInteractions;
    m_GizmoEvents.Broadcast(e);

    UpdateBox();
    FocusLost(false);
    return;
  }
}

void ezDrawBoxGizmo::UpdateBox()
{
  UpdateStatusBarText(GetOwnerWindow());

  if (m_ManipulateMode == ManipulateMode::DrawBase)
  {
    m_vSecondCorner = m_vCurrentPosition;
    m_vSecondCorner.x = ezMath::Lerp(m_vSecondCorner.x, m_vFirstCorner.x, m_vUpAxis.x);
    m_vSecondCorner.y = ezMath::Lerp(m_vSecondCorner.y, m_vFirstCorner.y, m_vUpAxis.y);
    m_vSecondCorner.z = ezMath::Lerp(m_vSecondCorner.z, m_vFirstCorner.z, m_vUpAxis.z);
  }

  if (m_ManipulateMode == ManipulateMode::None || m_vFirstCorner == m_vSecondCorner)
  {
    m_Box.SetTransformation(ezTransform(ezVec3(0), ezQuat::IdentityQuaternion(), ezVec3(0)));
    m_Box.SetVisible(false);
    return;
  }

  if (m_ManipulateMode == ManipulateMode::DrawHeight)
  {
    m_fBoxHeight = m_fOriginalBoxHeight + ((float)m_iHeightChange * 0.1f * ezSnapProvider::GetTranslationSnapValue());
    ezVec3 snapDummy(m_fBoxHeight);
    ezSnapProvider::SnapTranslation(snapDummy);
    m_fBoxHeight = m_vUpAxis.Dot(snapDummy);
  }

  ezVec3 vCenter = ezMath::Lerp(m_vFirstCorner, m_vSecondCorner, 0.5f);
  vCenter.x += m_fBoxHeight * 0.5f * m_vUpAxis.x;
  vCenter.y += m_fBoxHeight * 0.5f * m_vUpAxis.y;
  vCenter.z += m_fBoxHeight * 0.5f * m_vUpAxis.z;

  ezVec3 vSize;

  if (m_vUpAxis.z != 0)
  {
    vSize.x = ezMath::Abs(m_vSecondCorner.x - m_vFirstCorner.x);
    vSize.y = ezMath::Abs(m_vSecondCorner.y - m_vFirstCorner.y);
    vSize.z = m_fBoxHeight;
  }
  else if (m_vUpAxis.x != 0)
  {
    vSize.z = ezMath::Abs(m_vSecondCorner.z - m_vFirstCorner.z);
    vSize.y = ezMath::Abs(m_vSecondCorner.y - m_vFirstCorner.y);
    vSize.x = m_fBoxHeight;
  }
  else if (m_vUpAxis.y != 0)
  {
    vSize.x = ezMath::Abs(m_vSecondCorner.x - m_vFirstCorner.x);
    vSize.z = ezMath::Abs(m_vSecondCorner.z - m_vFirstCorner.z);
    vSize.y = m_fBoxHeight;
  }

  m_Box.SetTransformation(ezTransform(vCenter, ezQuat::IdentityQuaternion(), vSize));
  m_Box.SetVisible(true);
}

void ezDrawBoxGizmo::GetResult(ezVec3& out_Origin, float& out_fSizeNegX, float& out_fSizePosX, float& out_fSizeNegY, float& out_fSizePosY,
                               float& out_fSizeNegZ, float& out_fSizePosZ) const
{
  out_Origin = m_vFirstCorner;

  float fBoxX = m_vSecondCorner.x - m_vFirstCorner.x;
  float fBoxY = m_vSecondCorner.y - m_vFirstCorner.y;
  float fBoxZ = m_fBoxHeight;

  if (m_vUpAxis.x != 0)
  {
    fBoxY = m_vSecondCorner.y - m_vFirstCorner.y;
    fBoxZ = m_vSecondCorner.z - m_vFirstCorner.z;
    fBoxX = m_fBoxHeight;
  }

  if (m_vUpAxis.y != 0)
  {
    fBoxX = m_vSecondCorner.x - m_vFirstCorner.x;
    fBoxZ = m_vSecondCorner.z - m_vFirstCorner.z;
    fBoxY = m_fBoxHeight;
  }

  if (fBoxX > 0)
  {
    out_fSizeNegX = 0;
    out_fSizePosX = fBoxX;
  }
  else
  {
    out_fSizeNegX = -fBoxX;
    out_fSizePosX = 0;
  }

  if (fBoxY > 0)
  {
    out_fSizeNegY = 0;
    out_fSizePosY = fBoxY;
  }
  else
  {
    out_fSizeNegY = -fBoxY;
    out_fSizePosY = 0;
  }

  if (fBoxZ > 0)
  {
    out_fSizeNegZ = 0;
    out_fSizePosZ = fBoxZ;
  }
  else
  {
    out_fSizeNegZ = -fBoxZ;
    out_fSizePosZ = 0;
  }
}

void ezDrawBoxGizmo::UpdateStatusBarText(ezQtEngineDocumentWindow* pWindow)
{
  switch (m_ManipulateMode)
  {
    case ManipulateMode::None:
      pWindow->SetPermanentStatusBarMsg("Hold CTRL and click-drag to draw a box.");
      break;
    case ManipulateMode::DrawBase:
      pWindow->SetPermanentStatusBarMsg("Release the mouse to finish the base. ESC to cancel.");
      break;
    case ManipulateMode::DrawHeight:
      pWindow->SetPermanentStatusBarMsg("Draw up/down to specify the box height. Click to finish, ESC to cancel.");
      break;
  }
}
