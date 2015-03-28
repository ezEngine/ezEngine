#include <PCH.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetObjectsManager.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetObjects.h>
#include <ToolsFoundation/Reflection/ReflectedTypeManager.h>

ezTextureAssetObjectManager::ezTextureAssetObjectManager() : ezDocumentObjectManagerBase()
{
}

ezDocumentObjectBase* ezTextureAssetObjectManager::InternalCreateObject(ezReflectedTypeHandle hType)
{
  ezTextureAssetObject* pObject = EZ_DEFAULT_NEW(ezTextureAssetObject);
 
  return pObject;
}

void ezTextureAssetObjectManager::InternalDestroyObject(ezDocumentObjectBase* pObject)
{
  EZ_DEFAULT_DELETE(pObject);
}

void ezTextureAssetObjectManager::GetCreateableTypes(ezHybridArray<ezReflectedTypeHandle, 32>& Types) const
{
  Types.PushBack(ezReflectedTypeManager::GetTypeHandleByName(ezGetStaticRTTI<ezTextureAssetProperties>()->GetTypeName()));


}

bool ezTextureAssetObjectManager::InternalCanAdd(ezReflectedTypeHandle hType, const ezDocumentObjectBase* pParent) const
{
  return true;
}

bool ezTextureAssetObjectManager::InternalCanRemove(const ezDocumentObjectBase* pObject) const
{
  return true;
}

bool ezTextureAssetObjectManager::InternalCanMove(const ezDocumentObjectBase* pObject, const ezDocumentObjectBase* pNewParent, ezInt32 iChildIndex) const
{
  return false;
}
