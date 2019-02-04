#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Reflection/PropertyPath.h>
#include <GameEngine/Basics.h>

typedef ezTypedResourceHandle<class ezPrefabResource> ezPrefabResourceHandle;

struct EZ_GAMEENGINE_DLL ezPrefabResourceDescriptor
{
};

struct EZ_GAMEENGINE_DLL ezExposedPrefabParameterDesc
{
  ezHashedString m_sExposeName;
  ezUInt32 m_uiWorldReaderChildObject : 1; // 0 -> use root object array, 1 -> use child object array
  ezUInt32 m_uiWorldReaderObjectIndex : 31;
  ezUInt32 m_uiComponentTypeHash = 0;  // ezRTTI type name hash to identify which component is meant, 0 -> affects game object
  ezHashedString m_sProperty;          // which property to override
  ezPropertyPath m_CachedPropertyPath; // cached ezPropertyPath to apply a value to the specified property

  void Save(ezStreamWriter& stream) const;
  void Load(ezStreamReader& stream);
};

class EZ_GAMEENGINE_DLL ezPrefabResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPrefabResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezPrefabResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezPrefabResource, ezPrefabResourceDescriptor);

public:
  ezPrefabResource();

  /// \brief Creates an instance of this prefab in the given world.
  void InstantiatePrefab(ezWorld& world, const ezTransform& rootTransform, ezGameObjectHandle hParent,
                         ezHybridArray<ezGameObject*, 8>* out_CreatedRootObjects, const ezUInt16* pOverrideTeamID,
                         const ezArrayMap<ezHashedString, ezVariant>* pExposedParamValues);

  void ApplyExposedParameterValues(const ezArrayMap<ezHashedString, ezVariant>* pExposedParamValues,
                                   const ezHybridArray<ezGameObject*, 8>& createdChildObjects,
                                   const ezHybridArray<ezGameObject*, 8>& createdRootObjects) const;

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  ezUInt32 FindFirstParamWithName(ezUInt32 uiNameHash) const;

  ezWorldReader m_WorldReader;
  ezDynamicArray<ezExposedPrefabParameterDesc> m_PrefabParamDescs;
};

