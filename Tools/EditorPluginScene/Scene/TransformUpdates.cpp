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


