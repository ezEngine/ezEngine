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
  GetCommandHistory()->m_Events.AddEventHandler(ezMakeDelegate(&ezSceneDocument::CommandHistoryEventHandler, this));
}

ezSceneDocument::~ezSceneDocument()
{
  GetObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezSceneDocument::ObjectPropertyEventHandler, this));
  GetCommandHistory()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezSceneDocument::CommandHistoryEventHandler, this));
}

ezStatus ezSceneDocument::InternalSaveDocument()
{
  return ezDocumentBase::InternalSaveDocument();
}

void ezSceneDocument::SetActiveGizmo(ActiveGizmo gizmo)
{
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

void ezSceneDocument::SetGizmoWorldSpace(bool bWorldSpace)
{
  m_bGizmoWorldSpace = bWorldSpace;

  SceneEvent e;
  e.m_Type = SceneEvent::Type::ActiveGizmoChanged;
  m_SceneEvents.Broadcast(e);
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
    m_UpdateGlobalTransform.PushBack(e.m_pObject);
  }
}

void ezSceneDocument::UpdateObjectGlobalPosition(const ezDocumentObjectBase* pObject, const ezTransform& tParent)
{
  if (pObject->GetTypeAccessor().GetType() != ezGetStaticRTTI<ezGameObject>())
    return;

  const ezTransform tLocal = ComputeLocalTransform(pObject);

  ezTransform tGlobal;
  tGlobal.SetGlobalTransform(tParent, tLocal);

  ezSetObjectPropertyCommand cmd;
  cmd.m_bEditorProperty = false;
  cmd.m_Object = pObject->GetGuid();

  {
    cmd.SetPropertyPath("GlobalPosition");
    cmd.m_NewValue = tGlobal.m_vPosition;
    GetCommandHistory()->AddCommand(cmd);
  }

  {
    ezMat3 mRotScale = tGlobal.m_Rotation;
    mRotScale.SetScalingFactors(ezVec3(1.0f));

    ezQuat qRot;
    qRot.SetFromMat3(mRotScale);
    cmd.SetPropertyPath("GlobalRotation");
    cmd.m_NewValue = qRot;
    GetCommandHistory()->AddCommand(cmd);
  }

  {
    cmd.SetPropertyPath("GlobalScaling");
    cmd.m_NewValue = tGlobal.m_Rotation.GetScalingFactors();
    GetCommandHistory()->AddCommand(cmd);
  }

  // update all children as well
  for (const auto* pChild : pObject->GetChildren())
  {
    UpdateObjectGlobalPosition(pChild, tGlobal);
  }
}

void ezSceneDocument::UpdateObjectGlobalPosition(const ezDocumentObjectBase* pObject)
{
  if (pObject->GetTypeAccessor().GetType() != ezGetStaticRTTI<ezGameObject>())
    return;

  ezTransform tParent;

  if (pObject->GetParent() != GetObjectManager()->GetRootObject())
    tParent = ComputeGlobalTransform(pObject->GetParent());
  else
    tParent.SetIdentity();

  UpdateObjectGlobalPosition(pObject, tParent);
}

void ezSceneDocument::CommandHistoryEventHandler(const ezCommandHistory::Event& e)
{
  if (e.m_Type == ezCommandHistory::Event::Type::AfterEndTransaction)
  {
    m_bInObjectTransformFixup = false;
    m_UpdateGlobalTransform.Clear();
    return;
  }

  if (e.m_Type == ezCommandHistory::Event::Type::BeforeEndTransaction)
  {
    m_bInObjectTransformFixup = true;

    for (auto pObj : m_UpdateGlobalTransform)
      UpdateObjectGlobalPosition(pObj);

    m_UpdateGlobalTransform.Clear();
    m_bInObjectTransformFixup = false;
    return;
  }
}

ezTransform ezSceneDocument::ComputeLocalTransform(const ezDocumentObjectBase* pObject)
{
  const ezVec3 vTranslation = pObject->GetTypeAccessor().GetValue("LocalPosition").ConvertTo<ezVec3>();
  const ezVec3 vScaling = pObject->GetTypeAccessor().GetValue("LocalScaling").ConvertTo<ezVec3>();
  const ezQuat qRotation = pObject->GetTypeAccessor().GetValue("LocalRotation").ConvertTo<ezQuat>();

  return ezTransform(vTranslation, qRotation, vScaling);
}

ezTransform ezSceneDocument::ComputeGlobalTransform(const ezDocumentObjectBase* pObject)
{
  ezTransform tGlobal;
  if (pObject == nullptr || pObject->GetTypeAccessor().GetType() != ezGetStaticRTTI<ezGameObject>())
  {
    tGlobal.SetIdentity();
    return tGlobal;
  }

  const ezTransform tParent = ComputeGlobalTransform(pObject->GetParent());
  const ezTransform tLocal = ComputeLocalTransform(pObject);

  tGlobal.SetGlobalTransform(tParent, tLocal);

  return tGlobal;
}
