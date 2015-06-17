#include <PCH.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <EditorPluginScene/Objects/TestObjects.h>
#include <EditorPluginScene/Objects/SceneObjectManager.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <Core/World/GameObject.h>
#include <ToolsFoundation/Command/TreeCommands.h>


ezTransform ezSceneDocument::QueryLocalTransform(const ezDocumentObjectBase* pObject)
{
  const ezVec3 vTranslation = pObject->GetTypeAccessor().GetValue("LocalPosition").ConvertTo<ezVec3>();
  const ezVec3 vScaling = pObject->GetTypeAccessor().GetValue("LocalScaling").ConvertTo<ezVec3>();
  const ezQuat qRotation = pObject->GetTypeAccessor().GetValue("LocalRotation").ConvertTo<ezQuat>();

  return ezTransform(vTranslation, qRotation, vScaling);
}

ezTransform ezSceneDocument::QueryGlobalTransform(const ezDocumentObjectBase* pObject)
{
  const ezVec3 vTranslation = pObject->GetTypeAccessor().GetValue("GlobalPosition").ConvertTo<ezVec3>();
  const ezVec3 vScaling = pObject->GetTypeAccessor().GetValue("GlobalScaling").ConvertTo<ezVec3>();
  const ezQuat qRotation = pObject->GetTypeAccessor().GetValue("GlobalRotation").ConvertTo<ezQuat>();

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
  const ezTransform tLocal = QueryLocalTransform(pObject);

  tGlobal.SetGlobalTransform(tParent, tLocal);

  return tGlobal;
}

void ezSceneDocument::UpdateObjectGlobalPosition(const ezDocumentObjectBase* pObject, const ezTransform& tParent)
{
  if (pObject->GetTypeAccessor().GetType() != ezGetStaticRTTI<ezGameObject>())
    return;

  const ezTransform tLocal = QueryLocalTransform(pObject);

  ezTransform tGlobal;
  tGlobal.SetGlobalTransform(tParent, tLocal);

  ezSetObjectPropertyCommand cmd;
  cmd.m_bEditorProperty = false;
  cmd.m_Object = pObject->GetGuid();

  ezVec3 vNewPos, vNewScale;
  ezQuat qNewRot;
  tGlobal.Decompose(vNewPos, qNewRot, vNewScale);

  {
    cmd.SetPropertyPath("GlobalPosition");
    cmd.m_NewValue = vNewPos;
    GetCommandHistory()->AddCommand(cmd);
  }

  {
    cmd.SetPropertyPath("GlobalRotation");
    cmd.m_NewValue = qNewRot;
    GetCommandHistory()->AddCommand(cmd);
  }

  {
    cmd.SetPropertyPath("GlobalScaling");
    cmd.m_NewValue = vNewScale;
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

  UpdateObjectGlobalPosition(pObject, ComputeGlobalTransform(pObject->GetParent()));
}

void ezSceneDocument::UpdateObjectLocalPosition(const ezDocumentObjectBase* pObject)
{
  if (pObject->GetTypeAccessor().GetType() != ezGetStaticRTTI<ezGameObject>())
    return;

  ezTransform tParent = ComputeGlobalTransform(pObject->GetParent());
  ezTransform tGlobal = QueryGlobalTransform(pObject);

  ezTransform tLocal;
  tLocal.SetLocalTransform(tParent, tGlobal);

  ezSetObjectPropertyCommand cmd;
  cmd.m_bEditorProperty = false;
  cmd.m_Object = pObject->GetGuid();

  ezVec3 vNewPos, vNewScale;
  ezQuat qNewRot;
  tLocal.Decompose(vNewPos, qNewRot, vNewScale);

  {
    cmd.SetPropertyPath("LocalPosition");
    cmd.m_NewValue = vNewPos;
    GetCommandHistory()->AddCommand(cmd);
  }

  {
    cmd.SetPropertyPath("LocalRotation");
    cmd.m_NewValue = qNewRot;
    GetCommandHistory()->AddCommand(cmd);
  }

  {
    cmd.SetPropertyPath("LocalScaling");
    cmd.m_NewValue = vNewScale;
    GetCommandHistory()->AddCommand(cmd);
  }

  // update all children as well
  for (const auto* pChild : pObject->GetChildren())
  {
    UpdateObjectGlobalPosition(pChild, tGlobal);
  }
}
