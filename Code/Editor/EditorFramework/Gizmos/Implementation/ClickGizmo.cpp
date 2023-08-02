#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Graphics/Camera.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/ClickGizmo.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/Utilities/GraphicsUtils.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezClickGizmo, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezClickGizmo::ezClickGizmo()
{
  m_hShape.ConfigureHandle(this, ezEngineGizmoHandleType::Sphere, ezColor::White, ezGizmoFlags::Pickable);

  SetVisible(false);
  SetTransformation(ezTransform::MakeIdentity());
}

void ezClickGizmo::SetColor(const ezColor& color)
{
  m_hShape.SetColor(color);
}

void ezClickGizmo::OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hShape);
}

void ezClickGizmo::OnVisibleChanged(bool bVisible)
{
  m_hShape.SetVisible(bVisible);
}

void ezClickGizmo::OnTransformationChanged(const ezTransform& transform)
{
  m_hShape.SetTransformation(transform);
}

void ezClickGizmo::DoFocusLost(bool bCancel)
{
  ezViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);
}

ezEditorInput ezClickGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return ezEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return ezEditorInput::MayBeHandledByOthers;

  if (m_pInteractionGizmoHandle != &m_hShape)
    return ezEditorInput::MayBeHandledByOthers;

  ezViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  SetActiveInputContext(this);

  return ezEditorInput::WasExclusivelyHandled;
}

ezEditorInput ezClickGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return ezEditorInput::WasExclusivelyHandled;

  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = ezGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return ezEditorInput::WasExclusivelyHandled;
}
