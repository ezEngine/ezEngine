#include <PCH.h>
#include <EditorPluginTest/Objects/TestObjectManager.h>
#include <EditorPluginTest/Objects/TestObject.h>
#include <ToolsFoundation/Reflection/ReflectedTypeManager.h>
#include <Core/World/GameObject.h>

ezTestObjectManager::ezTestObjectManager(const ezDocumentBase* pDocument) : ezDocumentObjectManagerBase(pDocument)
{
}

ezDocumentObjectBase* ezTestObjectManager::InternalCreateObject(ezReflectedTypeHandle hType)
{
  return new ezDocumentObjectStorage<ezTestEditorProperties>(hType);

  //if (ezStringUtils::IsEqual(hType.GetType()->GetTypeName().GetData(), ezGetStaticRTTI<ezTestObjectProperties>()->GetTypeName()))
  //{
  //  // TODO
  //  // create ezTestObjectProperties dynamically
  //  // create wrapper object (ezTestObject), put ezTestObjectProperties instance in it
  //  // return wrapper

  //  return new ezTestObject(new ezTestObjectProperties);
  //}

  //if (ezStringUtils::IsEqual(hType.GetType()->GetTypeName().GetData(), ezGetStaticRTTI<ezTestEditorProperties>()->GetTypeName()))
  //{
  //  // TODO
  //  // create ezTestObjectProperties dynamically
  //  // create wrapper object (ezTestObject), put ezTestObjectProperties instance in it
  //  // return wrapper

  //  return new ezTestObject(new ezTestEditorProperties);
  //}

  //if (ezStringUtils::IsEqual(hType.GetType()->GetTypeName().GetData(), ezGetStaticRTTI<ezGameObject>()->GetTypeName()))
  //{
  //  return new ezTestObject2(hType);
  //}

  return nullptr;
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
  return true;
}

bool ezTestObjectManager::InternalCanRemove(const ezDocumentObjectBase* pObject) const
{
  return true;
}

bool ezTestObjectManager::InternalCanMove(const ezDocumentObjectBase* pObject, const ezDocumentObjectBase* pNewParent, ezInt32 iChildIndex) const
{
  return true;
}
