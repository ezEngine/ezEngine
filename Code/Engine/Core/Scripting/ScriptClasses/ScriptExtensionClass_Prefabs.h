#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

class ezWorld;
class ezGameObject;

/// Script extension class providing prefab instantiation functionality for scripts.
class EZ_CORE_DLL ezScriptExtensionClass_Prefabs
{
public:
  /// Spawns a prefab instance at the specified global transform.
  ///
  /// \param sPrefab Path or name of the prefab to spawn
  /// \param globalTransform World position, rotation and scale for the prefab
  /// \param uiUniqueID Unique identifier for deterministic spawning, use 0 for random
  /// \param bSetCreatedByPrefab Whether to mark spawned objects as created by prefab
  /// \param bSetHideShapeIcon Whether to hide shape icons in the editor for spawned objects
  /// \return Array of game object handles for the spawned prefab's top-level objects
  static ezVariantArray SpawnPrefab(ezWorld* pWorld, ezStringView sPrefab, const ezTransform& globalTransform, ezUInt32 uiUniqueID, bool bSetCreatedByPrefab, bool bSetHideShapeIcon);

  /// Spawns a prefab instance as a child of the specified parent object.
  ///
  /// \param sPrefab Path or name of the prefab to spawn
  /// \param pParent Parent game object for the spawned prefab
  /// \param localTransform Local transform relative to the parent
  /// \param uiUniqueID Unique identifier for deterministic spawning, use 0 for random
  /// \param bSetCreatedByPrefab Whether to mark spawned objects as created by prefab
  /// \param bSetHideShapeIcon Whether to hide shape icons in the editor for spawned objects
  /// \return Array of game object handles for the spawned prefab's top-level objects
  static ezVariantArray SpawnPrefabAsChild(ezWorld* pWorld, ezStringView sPrefab, ezGameObject* pParent, const ezTransform& localTransform, ezUInt32 uiUniqueID, bool bSetCreatedByPrefab, bool bSetHideShapeIcon);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezScriptExtensionClass_Prefabs);
