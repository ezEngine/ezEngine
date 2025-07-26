#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptAttributes.h>
#include <Core/Scripting/ScriptClasses/ScriptExtensionClass_Prefabs.h>

#include <Core/Prefabs/PrefabResource.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezScriptExtensionClass_Prefabs, ezNoBase, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(SpawnPrefab, In, "World", In, "Prefab", In, "GlobalTransform", In, "UniqueID", In, "SetCreatedByPrefab", In, "SetHideShapeIcon")->AddAttributes(
      new ezFunctionArgumentAttributes(1, new ezAssetBrowserAttribute("CompatibleAsset_Prefab")),
      new ezFunctionArgumentAttributes(3, new ezDefaultValueAttribute(ezVariant(ezInvalidIndex))),
      new ezFunctionArgumentAttributes(4, new ezDefaultValueAttribute(true)),
      new ezFunctionArgumentAttributes(5, new ezDefaultValueAttribute(true))),

    EZ_SCRIPT_FUNCTION_PROPERTY(SpawnPrefabAsChild, In, "World", In, "Prefab", In, "Parent", In, "LocalTransform", In, "UniqueID", In, "SetCreatedByPrefab", In, "SetHideShapeIcon")->AddAttributes(
      new ezFunctionArgumentAttributes(1, new ezAssetBrowserAttribute("CompatibleAsset_Prefab")),
      new ezFunctionArgumentAttributes(4, new ezDefaultValueAttribute(ezVariant(ezInvalidIndex))),
      new ezFunctionArgumentAttributes(5, new ezDefaultValueAttribute(true)),
      new ezFunctionArgumentAttributes(6, new ezDefaultValueAttribute(true))),
  }
  EZ_END_FUNCTIONS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezScriptExtensionAttribute("Prefabs"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

void SpawnPrefabHelper(ezWorld& ref_world, ezStringView sPrefab, ezGameObjectHandle hParent, const ezTransform& transform, ezUInt32 uiUniqueID, bool bSetCreatedByPrefab, bool bSetHideShapeIcon, ezVariantArray& out_rootObjects)
{
  ezPrefabResourceHandle hPrefab = ezResourceManager::LoadResource<ezPrefabResource>(sPrefab);

  ezResourceLock<ezPrefabResource> pPrefab(hPrefab, ezResourceAcquireMode::BlockTillLoaded_NeverFail);

  if (pPrefab.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  ezHybridArray<ezGameObject*, 8> createdRootObjects;
  ezHybridArray<ezGameObject*, 8> createdChildObjects;

  ezPrefabInstantiationOptions opt;
  opt.m_hParent = hParent;
  opt.m_pCreatedRootObjectsOut = &createdRootObjects;
  opt.m_pCreatedChildObjectsOut = &createdChildObjects;

  pPrefab->InstantiatePrefab(ref_world, transform, opt);

  auto FixupObject = [&](ezGameObject* pObject)
  {
    if (uiUniqueID != ezInvalidIndex)
    {
      for (auto pComponent : pObject->GetComponents())
      {
        pComponent->SetUniqueID(uiUniqueID);
      }
    }

    if (bSetCreatedByPrefab)
      pObject->SetCreatedByPrefab();

    if (bSetHideShapeIcon)
      pObject->SetHideShapeIcon();
  };

  for (auto pObject : createdRootObjects)
  {
    FixupObject(pObject);
    out_rootObjects.PushBack(pObject->GetHandle());
  }

  for (auto pObject : createdChildObjects)
  {
    FixupObject(pObject);
  }
}

ezVariantArray ezScriptExtensionClass_Prefabs::SpawnPrefab(ezWorld* pWorld, ezStringView sPrefab, const ezTransform& globalTransform, ezUInt32 uiUniqueID, bool bSetCreatedByPrefab, bool bSetHideShapeIcon)
{
  if (pWorld == nullptr || sPrefab.IsEmpty())
    return {};

  ezVariantArray rootObjects;
  SpawnPrefabHelper(*pWorld, sPrefab, ezGameObjectHandle(), globalTransform, uiUniqueID, bSetCreatedByPrefab, bSetHideShapeIcon, rootObjects);
  return rootObjects;
}

ezVariantArray ezScriptExtensionClass_Prefabs::SpawnPrefabAsChild(ezWorld* pWorld, ezStringView sPrefab, ezGameObject* pParent, const ezTransform& localTransform, ezUInt32 uiUniqueID, bool bSetCreatedByPrefab, bool bSetHideShapeIcon)
{
  if (pWorld == nullptr || sPrefab.IsEmpty())
    return {};

  ezVariantArray rootObjects;
  SpawnPrefabHelper(*pWorld, sPrefab, pParent != nullptr ? pParent->GetHandle() : ezGameObjectHandle(), localTransform, uiUniqueID, bSetCreatedByPrefab, bSetHideShapeIcon, rootObjects);
  return rootObjects;
}
