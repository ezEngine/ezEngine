#pragma once

#include <Core/ResourceManager/Resource.h>
#include <JoltPlugin/JoltPluginDLL.h>

using ezJoltHeightfieldResourceHandle = ezTypedResourceHandle<class ezJoltHeightfieldResource>;
using ezSurfaceResourceHandle = ezTypedResourceHandle<class ezSurfaceResource>;

struct EZ_JOLTPLUGIN_DLL ezJoltHeightfieldResourceDescriptor
{
  ezVec2 m_vHalfExtents;
  ezUInt32 m_uiResolution = 0;

  /// Row-major height samples, m_uiResolution * m_uiResolution entries.
  ezDynamicArray<float> m_Heights;

  /// Per-cell material indices, (m_uiResolution-1)^2 entries. May be empty.
  ezDynamicArray<ezUInt8> m_MaterialIndices;

  /// Surface handles indexed by m_MaterialIndices. May be empty.
  ezDynamicArray<ezSurfaceResourceHandle> m_Surfaces;

  ezUInt8 m_uiCollisionLayer = 0;
};

/// Stores a Jolt heightfield shape.
class EZ_JOLTPLUGIN_DLL ezJoltHeightfieldResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezJoltHeightfieldResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezJoltHeightfieldResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezJoltHeightfieldResource, ezJoltHeightfieldResourceDescriptor);

public:
  ezJoltHeightfieldResource();
  ~ezJoltHeightfieldResource();

  /// Returns the content hash that was stored at export time. Used by the export modifier
  /// to detect whether an existing file is still valid (compare against current brush hash).
  ezUInt64 GetContentHash() const { return m_uiContentHash; }

  /// Surface resources indexed by material index. May contain invalid handles; use default material as fallback.
  const ezDynamicArray<ezSurfaceResourceHandle>& GetSurfaces() const { return m_Surfaces; }

  ezUInt8 GetCollisionLayer() const { return m_uiCollisionLayer; }

  /// Raw JPH::HeightFieldShape binary state. Pass to JPH::Shape::sRestoreFromBinaryState().
  const ezDataBuffer& GetShapeData() const { return m_ShapeData; }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  ezUInt64 m_uiContentHash = 0;
  ezUInt8 m_uiCollisionLayer = 0;
  ezDynamicArray<ezSurfaceResourceHandle> m_Surfaces;
  ezDataBuffer m_ShapeData;
};
