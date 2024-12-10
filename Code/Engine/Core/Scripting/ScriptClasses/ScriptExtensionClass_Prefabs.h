#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

class ezWorld;
class ezGameObject;

class EZ_CORE_DLL ezScriptExtensionClass_Prefabs
{
public:
  static void SpawnPrefab(ezWorld* pWorld, ezStringView sPrefab, const ezTransform& globalTransform, const ezVec3& vRelativePosition, const ezQuat& qRelativeRotation);
  static void SpawnPrefabAsChild(ezWorld* pWorld, ezStringView sPrefab, ezGameObject* pParent, const ezVec3& vRelativePosition, const ezQuat& qRelativeRotation);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezScriptExtensionClass_Prefabs);
