#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

class ezWorld;
class ezGameObject;

class EZ_CORE_DLL ezScriptExtensionClass_Prefabs
{
public:
  static void SpawnPrefab(ezVariantArray& out_rootObjects, ezWorld* pWorld, ezStringView sPrefab, const ezTransform& globalTransform, ezUInt32 uiUniqueID, bool bSetCreatedByPrefab, bool bSetHideShapeIcon);
  static void SpawnPrefabAsChild(ezVariantArray& out_rootObjects, ezWorld* pWorld, ezStringView sPrefab, ezGameObject* pParent, const ezTransform& localTransform, ezUInt32 uiUniqueID, bool bSetCreatedByPrefab, bool bSetHideShapeIcon);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezScriptExtensionClass_Prefabs);
