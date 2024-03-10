#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Reflection/PropertyPath.h>

using ezPrefabResourceHandle = ezTypedResourceHandle<class ezPrefabResource>;

struct EZ_CORE_DLL ezPrefabResourceDescriptor
{
};

struct EZ_CORE_DLL ezExposedPrefabParameterDesc
{
  ezHashedString m_sExposeName;
  ezUInt32 m_uiWorldReaderChildObject : 1; // 0 -> use root object array, 1 -> use child object array
  ezUInt32 m_uiWorldReaderObjectIndex : 31;
  ezHashedString m_sComponentType;         // ezRTTI type name to identify which component is meant, empty string -> affects game object
  ezHashedString m_sProperty;              // which property to override
  ezPropertyPath m_CachedPropertyPath;     // cached ezPropertyPath to apply a value to the specified property

  void Save(ezStreamWriter& inout_stream) const;
  void Load(ezStreamReader& inout_stream);
};

class EZ_CORE_DLL ezPrefabResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPrefabResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezPrefabResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezPrefabResource, ezPrefabResourceDescriptor);

public:
  ezPrefabResource();

  enum class InstantiateResult : ezUInt8
  {
    Success,
    NotYetLoaded,
    Error,
  };

  /// \brief Helper function to instantiate a prefab without having to deal with resource acquisition.
  static ezPrefabResource::InstantiateResult InstantiatePrefab(const ezPrefabResourceHandle& hPrefab, bool bBlockTillLoaded, ezWorld& ref_world, const ezTransform& rootTransform, ezPrefabInstantiationOptions options = {}, const ezArrayMap<ezHashedString, ezVariant>* pExposedParamValues = nullptr);

  /// \brief Creates an instance of this prefab in the given world.
  void InstantiatePrefab(ezWorld& ref_world, const ezTransform& rootTransform, ezPrefabInstantiationOptions options, const ezArrayMap<ezHashedString, ezVariant>* pExposedParamValues = nullptr);

  void ApplyExposedParameterValues(const ezArrayMap<ezHashedString, ezVariant>* pExposedParamValues, const ezDynamicArray<ezGameObject*>& createdChildObjects, const ezDynamicArray<ezGameObject*>& createdRootObjects) const;

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  ezUInt32 FindFirstParamWithName(ezUInt64 uiNameHash) const;

  ezWorldReader m_WorldReader;
  ezDynamicArray<ezExposedPrefabParameterDesc> m_PrefabParamDescs;
};
