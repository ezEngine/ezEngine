#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptAttributes.h>
#include <Core/Scripting/ScriptClasses/ScriptExtensionClass_Prefabs.h>

#include <Core/Prefabs/PrefabResource.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezScriptExtensionClass_Prefabs, ezNoBase, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(SpawnPrefab, In, "World", In, "Prefab", In, "GlobalTransform", In, "RelativePos", In, "RelativeRotation")->AddAttributes(new ezFunctionArgumentAttributes(1, new ezAssetBrowserAttribute("CompatibleAsset_Prefab"))),

    EZ_SCRIPT_FUNCTION_PROPERTY(SpawnPrefabAsChild, In, "World", In, "Prefab", In, "Parent", In, "RelativePos", In, "RelativeRotation")->AddAttributes(new ezFunctionArgumentAttributes(1, new ezAssetBrowserAttribute("CompatibleAsset_Prefab"))),
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


void ezScriptExtensionClass_Prefabs::SpawnPrefab(ezWorld* pWorld, ezStringView sPrefab, const ezTransform& globalTransform, const ezVec3& vRelativePosition, const ezQuat& qRelativeRotation)
{
  if (sPrefab.IsEmpty())
    return;

  ezPrefabResourceHandle hPrefab = ezResourceManager::LoadResource<ezPrefabResource>(sPrefab);

  ezResourceLock<ezPrefabResource> pPrefab(hPrefab, ezResourceAcquireMode::BlockTillLoaded_NeverFail);

  if (pPrefab.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  ezTransform trans = ezTransform::MakeGlobalTransform(globalTransform, ezTransform(vRelativePosition, qRelativeRotation));

  pPrefab->InstantiatePrefab(*pWorld, trans, {});
}

void ezScriptExtensionClass_Prefabs::SpawnPrefabAsChild(ezWorld* pWorld, ezStringView sPrefab, ezGameObject* pParent, const ezVec3& vRelativePosition, const ezQuat& qRelativeRotation)
{
  if (sPrefab.IsEmpty())
    return;

  ezPrefabResourceHandle hPrefab = ezResourceManager::LoadResource<ezPrefabResource>(sPrefab);

  ezResourceLock<ezPrefabResource> pPrefab(hPrefab, ezResourceAcquireMode::BlockTillLoaded_NeverFail);

  if (pPrefab.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  ezPrefabInstantiationOptions opt;
  opt.m_hParent = pParent ? pParent->GetHandle() : ezGameObjectHandle();

  pPrefab->InstantiatePrefab(*pWorld, ezTransform(vRelativePosition, qRelativeRotation), opt);
}
