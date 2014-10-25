#include <PCH.h>
#include <EditorPluginTest/Objects/TestObjectManager.h>
#include <EditorPluginTest/Objects/TestObject.h>
#include <ToolsFoundation/Reflection/ReflectedTypeManager.h>

ezTestObjectManager::ezTestObjectManager(const ezDocumentBase* pDocument) : ezDocumentObjectManagerBase(pDocument)
{
}

ezDocumentObjectBase* ezTestObjectManager::InternalCreateObject(ezReflectedTypeHandle hType)
{
  if (ezStringUtils::IsEqual(hType.GetType()->GetTypeName().GetData(), ezGetStaticRTTI<ezTestObjectProperties>()->GetTypeName()))
  {
    // TODO
    // create ezTestObjectProperties dynamically
    // create wrapper object (ezTestObject), put ezTestObjectProperties instance in it
    // return wrapper

    return new ezTestObject(new ezTestObjectProperties);
  }

  if (ezStringUtils::IsEqual(hType.GetType()->GetTypeName().GetData(), ezGetStaticRTTI<ezTestEditorProperties>()->GetTypeName()))
  {
    // TODO
    // create ezTestObjectProperties dynamically
    // create wrapper object (ezTestObject), put ezTestObjectProperties instance in it
    // return wrapper

    return new ezTestObject(new ezTestEditorProperties);
  }

  return nullptr;
}

void ezTestObjectManager::GetCreateableTypes(ezHybridArray<ezReflectedTypeHandle, 32>& Types) const
{
  Types.PushBack(ezReflectedTypeManager::GetTypeHandleByName(ezGetStaticRTTI<ezTestObjectProperties>()->GetTypeName()));
  Types.PushBack(ezReflectedTypeManager::GetTypeHandleByName(ezGetStaticRTTI<ezTestEditorProperties>()->GetTypeName()));
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
