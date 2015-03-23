#include <PCH.h>
#include <EditorPluginTest/Objects/TestObjectManager.h>
#include <EditorPluginTest/Objects/TestObject.h>
#include <ToolsFoundation/Reflection/ReflectedTypeManager.h>
#include <Core/World/GameObject.h>

ezTestObjectManager::ezTestObjectManager() : ezDocumentObjectManagerBase()
{
}

ezDocumentObjectBase* ezTestObjectManager::InternalCreateObject(ezReflectedTypeHandle hType)
{
  static int iCount = 0;
  ezDocumentObjectStorage<ezTestEditorProperties>* pObj = new ezDocumentObjectStorage<ezTestEditorProperties>(hType);
  
  ezStringBuilder sName;
  sName.Format("%s %03d", hType.GetType()->GetTypeName().GetData(), iCount);
  iCount++;
  pObj->m_EditorProperties.SetName(sName);
  
  return pObj;
}

void ezTestObjectManager::InternalDestroyObject(ezDocumentObjectBase* pObject)
{
  delete pObject;
}

void ezTestObjectManager::GetCreateableTypes(ezHybridArray<ezReflectedTypeHandle, 32>& Types) const
{
  Types.PushBack(ezReflectedTypeManager::GetTypeHandleByName(ezGetStaticRTTI<ezGameObject>()->GetTypeName()));

  ezReflectedTypeHandle hComponent = ezReflectedTypeManager::GetTypeHandleByName(ezGetStaticRTTI<ezComponent>()->GetTypeName());
  for (auto it = ezReflectedTypeManager::GetTypeIterator(); it.IsValid(); ++it)
  {
    if (it.Value()->IsDerivedFrom(hComponent))
      Types.PushBack(it.Value()->GetTypeHandle());
  }

  //Types.PushBack(ezReflectedTypeManager::GetTypeHandleByName(ezGetStaticRTTI<ezTestObjectProperties>()->GetTypeName()));
  //Types.PushBack(ezReflectedTypeManager::GetTypeHandleByName(ezGetStaticRTTI<ezTestEditorProperties>()->GetTypeName()));
}

bool ezTestObjectManager::InternalCanAdd(ezReflectedTypeHandle hType, const ezDocumentObjectBase* pParent) const
{
  ezReflectedTypeHandle hGameObject = ezReflectedTypeManager::GetTypeHandleByName(ezGetStaticRTTI<ezGameObject>()->GetTypeName());
  ezReflectedTypeHandle hComponent  = ezReflectedTypeManager::GetTypeHandleByName(ezGetStaticRTTI<ezComponent>()->GetTypeName());

  if (hType.GetType()->IsDerivedFrom(hGameObject))
  {
    if (pParent == nullptr)
      return true;

    return (pParent->GetTypeAccessor().GetReflectedTypeHandle() == hGameObject);
  }
  else if (hType.GetType()->IsDerivedFrom(hComponent))
  {
    if (pParent == nullptr)
      return false;

    return (pParent->GetTypeAccessor().GetReflectedTypeHandle() == hGameObject);
  }
  return false;
}

bool ezTestObjectManager::InternalCanRemove(const ezDocumentObjectBase* pObject) const
{
  return true;
}

bool ezTestObjectManager::InternalCanMove(const ezDocumentObjectBase* pObject, const ezDocumentObjectBase* pNewParent, ezInt32 iChildIndex) const
{
  return InternalCanAdd(pObject->GetTypeAccessor().GetReflectedTypeHandle(), pNewParent);
}
