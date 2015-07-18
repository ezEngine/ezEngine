#include <PCH.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <EditorPluginScene/Objects/TestObjects.h>
#include <EditorPluginScene/Objects/SceneObjectManager.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <Core/World/GameObject.h>
#include <ToolsFoundation/Command/TreeCommands.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneDocument, ezDocumentBase, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezSceneDocument::ezSceneDocument(const char* szDocumentPath) : ezDocumentBase(szDocumentPath, EZ_DEFAULT_NEW(ezSceneObjectManager))
{
  m_ActiveGizmo = ActiveGizmo::None;
  m_bInObjectTransformFixup = false;
}

void ezSceneDocument::InitializeAfterLoading()
{
  ezDocumentBase::InitializeAfterLoading();

  GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezSceneDocument::ObjectPropertyEventHandler, this));
  GetObjectManager()->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezSceneDocument::ObjectStructureEventHandler, this));

  GetCommandHistory()->m_Events.AddEventHandler(ezMakeDelegate(&ezSceneDocument::CommandHistoryEventHandler, this));
}

ezSceneDocument::~ezSceneDocument()
{
  GetObjectManager()->m_StructureEvents.RemoveEventHandler(ezMakeDelegate(&ezSceneDocument::ObjectStructureEventHandler, this));
  GetObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezSceneDocument::ObjectPropertyEventHandler, this));
  GetCommandHistory()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezSceneDocument::CommandHistoryEventHandler, this));
}

ezStatus ezSceneDocument::InternalSaveDocument()
{
  return ezDocumentBase::InternalSaveDocument();
}

void ezSceneDocument::SetActiveGizmo(ActiveGizmo gizmo)
{
  if (m_ActiveGizmo == gizmo)
    return;

  m_ActiveGizmo = gizmo;

  SceneEvent e;
  e.m_Type = SceneEvent::Type::ActiveGizmoChanged;
  m_SceneEvents.Broadcast(e);
}

ActiveGizmo ezSceneDocument::GetActiveGizmo() const
{
  return m_ActiveGizmo;
}

void ezSceneDocument::TriggerShowSelectionInScenegraph()
{
  if (GetSelectionManager()->GetSelection().IsEmpty())
    return;

  SceneEvent e;
  e.m_Type = SceneEvent::Type::ShowSelectionInScenegraph;
  m_SceneEvents.Broadcast(e);
}

void ezSceneDocument::TriggerFocusOnSelection()
{
  if (GetSelectionManager()->GetSelection().IsEmpty())
    return;

  SceneEvent e;
  e.m_Type = SceneEvent::Type::FocusOnSelection;
  m_SceneEvents.Broadcast(e);
}

void ezSceneDocument::SetGizmoWorldSpace(bool bWorldSpace)
{
  if (m_bGizmoWorldSpace == bWorldSpace)
    return;

  m_bGizmoWorldSpace = bWorldSpace;

  SceneEvent e;
  e.m_Type = SceneEvent::Type::ActiveGizmoChanged;
  m_SceneEvents.Broadcast(e);
}

bool ezSceneDocument::GetGizmoWorldSpace() const
{
  if (m_ActiveGizmo == ActiveGizmo::Scale)
    return false;

  if (m_ActiveGizmo == ActiveGizmo::DragToPosition)
    return false;

  return m_bGizmoWorldSpace;
}

void ezSceneDocument::ObjectPropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  if (m_bInObjectTransformFixup)
    return;

  if (e.m_bEditorProperty)
    return;

  if (e.m_sPropertyPath == "LocalPosition" ||
      e.m_sPropertyPath == "LocalRotation" ||
      e.m_sPropertyPath == "LocalScaling")
  {
    /// \todo Use a set ?
    m_UpdateGlobalTransform.PushBack(e.m_pObject);
  }
  else if (e.m_sPropertyPath == "GlobalPosition" ||
           e.m_sPropertyPath == "GlobalRotation" ||
           e.m_sPropertyPath == "GlobalScaling")
  {
    /// \todo Use a set ?
    m_UpdateLocalTransform.PushBack(e.m_pObject);
  }
}

void ezSceneDocument::ObjectStructureEventHandler(const ezDocumentObjectStructureEvent& e)
{
  if (m_bInObjectTransformFixup)
    return;

  switch (e.m_EventType)
  {
  case ezDocumentObjectStructureEvent::Type::AfterObjectMoved:
    {
      m_UpdateLocalTransform.PushBack(e.m_pObject);
    }
    break;
  }
}

void ezSceneDocument::CommandHistoryEventHandler(const ezCommandHistory::Event& e)
{
  if (e.m_Type == ezCommandHistory::Event::Type::AfterEndTransaction)
  {
    m_bInObjectTransformFixup = false;
    m_UpdateGlobalTransform.Clear();
    m_UpdateLocalTransform.Clear();
    return;
  }

  /// \todo Check why EndTransaction is called so often
  if (e.m_Type == ezCommandHistory::Event::Type::BeforeEndTransaction && (!m_UpdateGlobalTransform.IsEmpty() || !m_UpdateLocalTransform.IsEmpty()))
  {
    m_bInObjectTransformFixup = true;

    for (auto pObj : m_UpdateGlobalTransform)
      UpdateObjectGlobalPosition(pObj);

    for (auto pObj : m_UpdateLocalTransform)
      UpdateObjectLocalPosition(pObj);

    m_UpdateGlobalTransform.Clear();
    m_UpdateLocalTransform.Clear();

    m_bInObjectTransformFixup = false;
    return;
  }
}
