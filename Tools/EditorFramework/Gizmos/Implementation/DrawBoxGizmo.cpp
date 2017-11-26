#include <PCH.h>
#include <EditorFramework/Gizmos/DrawBoxGizmo.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <Foundation/Logging/Log.h>
#include <Core/Graphics/Camera.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <Foundation/Math/Mat4.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <QMouseEvent>
#include <EditorFramework/Gizmos/SnapProvider.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDrawBoxGizmo, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezDrawBoxGizmo::ezDrawBoxGizmo()
{
  m_ManipulateMode = ManipulateMode::None;

  m_Box.Configure(this, ezEngineGizmoHandleType::LineBox, ezColorLinearUB(255, 100, 0), false, false, false, true, false);

  SetVisible(false);
  SetTransformation(ezTransform::Identity());
}

ezDrawBoxGizmo::~ezDrawBoxGizmo()
{
}

void ezDrawBoxGizmo::OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_Box);
}

void ezDrawBoxGizmo::OnVisibleChanged(bool bVisible)
{
}

void ezDrawBoxGizmo::OnTransformationChanged(const ezTransform& transform)
{
}

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

      if (res.m_PickedObject.IsValid())
        m_vCurrentPosition = res.m_vPickedPosition;
      else
      {
        ezPlane plane;
        plane.SetFromNormalAndPoint(ezVec3(0, 0, 1), ezVec3(0, 0, 0));

        if (GetOwnerView()->PickPlane(e->pos().x(), e->pos().y(), plane, m_vCurrentPosition).Failed())
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
    plane.SetFromNormalAndPoint(ezVec3(0, 0, 1), m_vFirstCorner);

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
    m_vSecondCorner.z = m_vFirstCorner.z;
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
    m_fBoxHeight = snapDummy.z;
  }

  ezVec3 vCenter = ezMath::Lerp(m_vFirstCorner, m_vSecondCorner, 0.5f);
  vCenter.z += m_fBoxHeight * 0.5f;

  ezVec3 vSize;
  vSize.x = ezMath::Abs(m_vSecondCorner.x - m_vFirstCorner.x);
  vSize.y = ezMath::Abs(m_vSecondCorner.y - m_vFirstCorner.y);
  vSize.z = m_fBoxHeight;

  m_Box.SetTransformation(ezTransform(vCenter, ezQuat::IdentityQuaternion(), vSize));
  m_Box.SetVisible(true);
}

void ezDrawBoxGizmo::GetResult(ezVec3& out_Origin, float& out_fSizeNegX, float& out_fSizePosX, float& out_fSizeNegY, float& out_fSizePosY, float& out_fSizeNegZ, float& out_fSizePosZ) const
{
  out_Origin = m_vFirstCorner;

  const float fBoxWidth = m_vSecondCorner.x - m_vFirstCorner.x;
  const float fBoxDepth = m_vSecondCorner.y - m_vFirstCorner.y;

  if (fBoxWidth > 0)
  {
    out_fSizeNegX = 0;
    out_fSizePosX = fBoxWidth;
  }
  else
  {
    out_fSizeNegX = -fBoxWidth;
    out_fSizePosX = 0;
  }

  if (fBoxDepth > 0)
  {
    out_fSizeNegY = 0;
    out_fSizePosY = fBoxDepth;
  }
  else
  {
    out_fSizeNegY = -fBoxDepth;
    out_fSizePosY = 0;
  }

  if (m_fBoxHeight > 0)
  {
    out_fSizeNegZ = 0;
    out_fSizePosZ = m_fBoxHeight;
  }
  else
  {
    out_fSizeNegZ = -m_fBoxHeight;
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

