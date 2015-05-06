#include <PCH.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetObjectsManager.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetObjects.h>
#include <ToolsFoundation/Reflection/ReflectedTypeManager.h>

ezMaterialAssetObjectManager::ezMaterialAssetObjectManager() : ezDocumentObjectManagerBase()
{
}

ezDocumentObjectBase* ezMaterialAssetObjectManager::InternalCreateObject(ezReflectedTypeHandle hType)
{
  ezMaterialAssetObject* pObject = EZ_DEFAULT_NEW(ezMaterialAssetObject);
 
  return pObject;
}

void ezMaterialAssetObjectManager::InternalDestroyObject(ezDocumentObjectBase* pObject)
{
  EZ_DEFAULT_DELETE(pObject);
}

void ezMaterialAssetObjectManager::GetCreateableTypes(ezHybridArray<ezReflectedTypeHandle, 32>& Types) const
{
  Types.PushBack(ezReflectedTypeManager::GetTypeHandleByName(ezGetStaticRTTI<ezMaterialAssetProperties>()->GetTypeName()));

}

bool ezMaterialAssetObjectManager::InternalCanAdd(ezReflectedTypeHandle hType, const ezDocumentObjectBase* pParent) const
{
  return true;
}

bool ezMaterialAssetObjectManager::InternalCanRemove(const ezDocumentObjectBase* pObject) const
{
  return true;
}

bool ezMaterialAssetObjectManager::InternalCanMove(const ezDocumentObjectBase* pObject, const ezDocumentObjectBase* pNewParent, ezInt32 iChildIndex) const
{
  return false;
}
