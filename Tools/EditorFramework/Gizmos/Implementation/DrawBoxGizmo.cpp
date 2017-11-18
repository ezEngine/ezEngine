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

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDrawBoxGizmo, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezDrawBoxGizmo::ezDrawBoxGizmo()
{
  m_ManipulateMode = ManipulateMode::None;

  m_Box.Configure(this, ezEngineGizmoHandleType::Box, ezColorLinearUB(200, 200, 255, 128), false, false, false, true, false);

  SetVisible(false);
  SetTransformation(ezTransform::Identity());
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
  m_Box.SetVisible(false);

  if (IsActiveInputContext())
    SetActiveInputContext(nullptr);
}

ezEditorInut ezDrawBoxGizmo::DoMousePressEvent(QMouseEvent* e)
{
  return ezEditorInut::MayBeHandledByOthers;
}

ezEditorInut ezDrawBoxGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  return ezEditorInut::MayBeHandledByOthers;
}

ezEditorInut ezDrawBoxGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInut::MayBeHandledByOthers;

  const ezObjectPickingResult& res = GetOwnerView()->PickObject(e->pos().x(), e->pos().y());

  m_Box.SetTransformation(ezTransform(res.m_vPickedPosition));

  return ezEditorInut::WasExclusivelyHandled;
}

ezEditorInut ezDrawBoxGizmo::DoKeyPressEvent(QKeyEvent* e)
{
  // is the gizmo in general visible == is it active
  if (!IsVisible())
    return ezEditorInut::MayBeHandledByOthers;

  if (e->key() == Qt::Key_Escape)
  {
    if (m_ManipulateMode != ManipulateMode::None)
    {
      FocusLost(true);
      return ezEditorInut::WasExclusivelyHandled;
    }
  }

  if (e->key() == Qt::Key_Space)
  {
    if (m_ManipulateMode == ManipulateMode::None)
    {
      SetActiveInputContext(this);
      m_ManipulateMode = ManipulateMode::DrawBase;
      m_Box.SetVisible(true);
      return ezEditorInut::WasExclusivelyHandled;
    }
  }

  return ezEditorInut::MayBeHandledByOthers;
}

ezEditorInut ezDrawBoxGizmo::DoKeyReleaseEvent(QKeyEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInut::MayBeHandledByOthers;

  if (e->key() == Qt::Key_Space)
  {
    if (m_ManipulateMode == ManipulateMode::DrawBase)
    {
      m_ManipulateMode = ManipulateMode::DrawHeight;
      return ezEditorInut::WasExclusivelyHandled;
    }

    if (m_ManipulateMode == ManipulateMode::DrawHeight)
    {
      FocusLost(false);

      // FInish box
      return ezEditorInut::WasExclusivelyHandled;
    }
  }

  return ezEditorInut::MayBeHandledByOthers;
}

