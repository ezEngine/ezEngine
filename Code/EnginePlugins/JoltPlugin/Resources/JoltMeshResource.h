#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <JoltPlugin/JoltPluginDLL.h>

using ezJoltMeshResourceHandle = ezTypedResourceHandle<class ezJoltMeshResource>;
using ezSurfaceResourceHandle = ezTypedResourceHandle<class ezSurfaceResource>;
using ezCpuMeshResourceHandle = ezTypedResourceHandle<class ezCpuMeshResource>;

struct ezMsgExtractGeometry;
class ezJoltMaterial;

namespace JPH
{
  class MeshShape;
  class ConvexHullShape;
  class Shape;
} // namespace JPH

struct EZ_JOLTPLUGIN_DLL ezJoltMeshResourceDescriptor
{
  // empty, these types of resources must be loaded from file
};

class EZ_JOLTPLUGIN_DLL ezJoltMeshResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezJoltMeshResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezJoltMeshResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezJoltMeshResource, ezJoltMeshResourceDescriptor);

public:
  ezJoltMeshResource();
  ~ezJoltMeshResource();

  /// \brief Returns the bounds of the collision mesh
  const ezBoundingBoxSphere& GetBounds() const { return m_Bounds; }

  /// \brief Returns the array of default surfaces to be used with this mesh.
  ///
  /// Note the array may contain less surfaces than the mesh does. It may also contain invalid surface handles.
  /// Use the default physics material as a fallback.
  const ezDynamicArray<ezSurfaceResourceHandle>& GetSurfaces() const { return m_Surfaces; }

  /// \brief Returns whether the mesh resource contains a triangle mesh. Triangle meshes and convex meshes are mutually exclusive.
  bool HasTriangleMesh() const { return m_pTriangleMeshInstance != nullptr || !m_TriangleMeshData.IsEmpty(); }

  /// \brief Creates a new instance (shape) of the triangle mesh.
  JPH::Shape* InstantiateTriangleMesh(ezUInt64 uiUserData, const ezDynamicArray<const ezJoltMaterial*>& materials) const;

  /// \brief Returns the number of convex meshes. Triangle meshes and convex meshes are mutually exclusive.
  ezUInt32 GetNumConvexParts() const { return !m_ConvexMeshInstances.IsEmpty() ? m_ConvexMeshInstances.GetCount() : m_ConvexMeshesData.GetCount(); }

  /// \brief Creates a new instance (shape) of the triangle mesh.
  JPH::Shape* InstantiateConvexPart(ezUInt32 uiPartIdx, ezUInt64 uiUserData, const ezJoltMaterial* pMaterial, float fDensity) const;

  /// \brief Converts the geometry of the triangle or convex mesh to a CPU mesh resource
  ezCpuMeshResourceHandle ConvertToCpuMesh() const;

  ezUInt32 GetNumTriangles() const { return m_uiNumTriangles; }
  ezUInt32 GetNumVertices() const { return m_uiNumVertices; }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  ezBoundingBoxSphere m_Bounds;
  ezDynamicArray<ezSurfaceResourceHandle> m_Surfaces;
  mutable ezHybridArray<ezDataBuffer*, 1> m_ConvexMeshesData;
  mutable ezDataBuffer m_TriangleMeshData;
  mutable JPH::Shape* m_pTriangleMeshInstance = nullptr;
  mutable ezHybridArray<JPH::Shape*, 1> m_ConvexMeshInstances;

  ezUInt32 m_uiNumVertices = 0;
  ezUInt32 m_uiNumTriangles = 0;
};
