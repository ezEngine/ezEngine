#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptAttributes.h>
#include <Core/Scripting/ScriptClasses/ScriptExtensionClass_Prefabs.h>

#include <Core/Prefabs/PrefabResource.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezScriptExtensionClass_Prefabs, ezNoBase, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(SpawnPrefab, Out, "RootObjects", In, "World", In, "Prefab", In, "GlobalTransform", In, "UniqueID", In, "SetCreatedByPrefab", In, "SetHideShapeIcon")->AddAttributes(
      new ezFunctionArgumentAttributes(2, new ezAssetBrowserAttribute("CompatibleAsset_Prefab")),
      new ezFunctionArgumentAttributes(4, new ezDefaultValueAttribute(ezVariant(ezInvalidIndex))),
      new ezFunctionArgumentAttributes(5, new ezDefaultValueAttribute(true)),
      new ezFunctionArgumentAttributes(6, new ezDefaultValueAttribute(true))),

    EZ_SCRIPT_FUNCTION_PROPERTY(SpawnPrefabAsChild, Out, "RootObjects", In, "World", In, "Prefab", In, "Parent", In, "LocalTransform", In, "UniqueID", In, "SetCreatedByPrefab", In, "SetHideShapeIcon")->AddAttributes(
      new ezFunctionArgumentAttributes(2, new ezAssetBrowserAttribute("CompatibleAsset_Prefab")),
      new ezFunctionArgumentAttributes(5, new ezDefaultValueAttribute(ezVariant(ezInvalidIndex))),
      new ezFunctionArgumentAttributes(6, new ezDefaultValueAttribute(true)),
      new ezFunctionArgumentAttributes(7, new ezDefaultValueAttribute(true))),
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

void SpawnPrefabHelper(ezWorld& world, ezStringView sPrefab, ezGameObjectHandle hParent, const ezTransform& transform, ezUInt32 uiUniqueID, bool bSetCreatedByPrefab, bool bSetHideShapeIcon, ezVariantArray& out_rootObjects)
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

  pPrefab->InstantiatePrefab(world, transform, opt);

  auto FixupObject = [&](ezGameObject* pObject) {
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

void ezScriptExtensionClass_Prefabs::SpawnPrefab(ezVariantArray& out_rootObjects, ezWorld* pWorld, ezStringView sPrefab, const ezTransform& globalTransform, ezUInt32 uiUniqueID, bool bSetCreatedByPrefab, bool bSetHideShapeIcon)
{
  if (pWorld == nullptr || sPrefab.IsEmpty())
    return;

  SpawnPrefabHelper(*pWorld, sPrefab, ezGameObjectHandle(), globalTransform, uiUniqueID, bSetCreatedByPrefab, bSetHideShapeIcon, out_rootObjects);
}

void ezScriptExtensionClass_Prefabs::SpawnPrefabAsChild(ezVariantArray& out_rootObjects, ezWorld* pWorld, ezStringView sPrefab, ezGameObject* pParent, const ezTransform& localTransform, ezUInt32 uiUniqueID, bool bSetCreatedByPrefab, bool bSetHideShapeIcon)
{
  if (pWorld == nullptr || sPrefab.IsEmpty())
    return;

  SpawnPrefabHelper(*pWorld, sPrefab, pParent != nullptr ? pParent->GetHandle() : ezGameObjectHandle(), localTransform, uiUniqueID, bSetCreatedByPrefab, bSetHideShapeIcon, out_rootObjects);
}
