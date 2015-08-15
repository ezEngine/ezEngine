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
  //ezDocumentObjectStorage<ezSceneObjectEditorProperties>* pObj = EZ_DEFAULT_NEW(ezDocumentObjectStorage<ezSceneObjectEditorProperties>, pRtti);
  ezDocumentObjectBase* pObj = EZ_DEFAULT_NEW(ezDocumentObject, ezSceneObjectEditorProperties::GetStaticRTTI(), pRtti);

  ezStringBuilder sName;
  sName.Format("%s %03d", pRtti->GetTypeName(), iCount);
  iCount++;
  pObj->GetEditorTypeAccessor().SetValue("Name", sName.GetData());  
  return pObj;
}

void ezSceneObjectManager::InternalDestroyObject(ezDocumentObjectBase* pObject)
{
  EZ_DEFAULT_DELETE(pObject);
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

bool ezSceneObjectManager::InternalCanAdd(const ezRTTI* pRtti, const ezDocumentObjectBase* pParent, const char* szParentProperty, const ezVariant& index, bool bEditorProperty) const
{
  const ezRTTI* pGameObjectType = ezRTTI::FindTypeByName(ezGetStaticRTTI<ezGameObject>()->GetTypeName());
  // TODO: BLA
  //const ezRTTI* pComponentType  = ezRTTI::FindTypeByName(ezGetStaticRTTI<ezComponent>()->GetTypeName());

  if (pParent == nullptr)
  {
    return pRtti->IsDerivedFrom(pGameObjectType);
  }
  
  return true;
}

bool ezSceneObjectManager::InternalCanRemove(const ezDocumentObjectBase* pObject) const
{
  return true;
}

bool ezSceneObjectManager::InternalCanMove(const ezDocumentObjectBase* pObject, const ezDocumentObjectBase* pNewParent, const char* szParentProperty, const ezVariant& index) const
{
  return true;
}
