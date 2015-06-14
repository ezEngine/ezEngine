#include <PCH.h>
#include <EditorPluginScene/Objects/SceneObjectManager.h>
#include <EditorPluginScene/Objects/TestObjects.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <Core/World/GameObject.h>

ezSceneObjectManager::ezSceneObjectManager() : ezDocumentObjectManager()
{
}

ezDocumentObjectBase* ezSceneObjectManager::InternalCreateObject(const ezRTTI* pRtti)
{
  static int iCount = 0;
  ezDocumentObjectStorage<ezSceneObjectEditorProperties>* pObj = new ezDocumentObjectStorage<ezSceneObjectEditorProperties>(pRtti);
  
  ezStringBuilder sName;
  sName.Format("%s %03d", pRtti->GetTypeName(), iCount);
  iCount++;
  pObj->m_EditorProperties.SetName(sName);
  
  return pObj;
}

void ezSceneObjectManager::InternalDestroyObject(ezDocumentObjectBase* pObject)
{
  delete pObject;
}

void ezSceneObjectManager::GetCreateableTypes(ezHybridArray<ezRTTI*, 32>& Types) const
{
  Types.PushBack(ezRTTI::FindTypeByName(ezGetStaticRTTI<ezGameObject>()->GetTypeName()));

  const ezRTTI* pComponentType = ezRTTI::FindTypeByName(ezGetStaticRTTI<ezComponent>()->GetTypeName());
  
  for (auto it = ezRTTI::GetFirstInstance(); it != nullptr; it = it->GetNextInstance())
  {
    if (it->IsDerivedFrom(pComponentType) && !it->GetTypeFlags().IsSet(ezTypeFlags::Abstract))
      Types.PushBack(it);
  }
}

bool ezSceneObjectManager::InternalCanAdd(const ezRTTI* pRtti, const ezDocumentObjectBase* pParent) const
{
  const ezRTTI* pGameObjectType = ezRTTI::FindTypeByName(ezGetStaticRTTI<ezGameObject>()->GetTypeName());
  const ezRTTI* pComponentType  = ezRTTI::FindTypeByName(ezGetStaticRTTI<ezComponent>()->GetTypeName());

  if (pRtti->IsDerivedFrom(pGameObjectType))
  {
    if (pParent == nullptr)
      return true;

    return (pParent->GetTypeAccessor().GetType() == pGameObjectType);
  }
  else if (pRtti->IsDerivedFrom(pComponentType))
  {
    if (pParent == nullptr)
      return false;

    return (pParent->GetTypeAccessor().GetType() == pGameObjectType);
  }
  return false;
}

bool ezSceneObjectManager::InternalCanRemove(const ezDocumentObjectBase* pObject) const
{
  return true;
}

bool ezSceneObjectManager::InternalCanMove(const ezDocumentObjectBase* pObject, const ezDocumentObjectBase* pNewParent, ezInt32 iChildIndex) const
{
  return InternalCanAdd(pObject->GetTypeAccessor().GetType(), pNewParent);
}
