#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <PhysXPlugin/PhysXPluginDLL.h>

typedef ezTypedResourceHandle<class ezPxMeshResource> ezPxMeshResourceHandle;
typedef ezTypedResourceHandle<class ezSurfaceResource> ezSurfaceResourceHandle;

struct ezMsgExtractGeometry;

struct EZ_PHYSXPLUGIN_DLL ezPxMeshResourceDescriptor
{
  // empty, these types of resources must be loaded from file
};

class EZ_PHYSXPLUGIN_DLL ezPxMeshResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPxMeshResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezPxMeshResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezPxMeshResource, ezPxMeshResourceDescriptor);

public:
  ezPxMeshResource();
  ~ezPxMeshResource();

  /// \brief Returns the triangle collision mesh. If the mesh is a convex mesh, this will be a nullptr.
  physx::PxTriangleMesh* GetTriangleMesh() const { return m_pPxTriangleMesh; }

  /// \brief Returns the convex collision meshes. Can contain more than one part, for convex decomposition meshes. If the mesh is a triangle mesh, this will be empty.
  const ezArrayPtr<physx::PxConvexMesh* const> GetConvexParts() const { return m_PxConvexParts.GetArrayPtr(); }

  /// \brief Returns the bounds of the collision mesh
  const ezBoundingBoxSphere& GetBounds() const { return m_Bounds; }

  /// \brief Returns the array of default surfaces to be used with this mesh.
  ///
  /// Note the array may contain less surfaces than the mesh does. It may also contain invalid surface handles.
  /// Use the default physics material as a fallback.
  const ezDynamicArray<ezSurfaceResourceHandle>& GetSurfaces() const { return m_Surfaces; }

  /// \brief Adds the geometry of the triangle or convex mesh to the descriptor
  void ExtractGeometry(const ezTransform& transform, ezMsgExtractGeometry& msg) const;

  ezUInt32 GetNumPolygons() const;
  ezUInt32 GetNumVertices() const;

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  ezDynamicArray<ezSurfaceResourceHandle> m_Surfaces;
  physx::PxTriangleMesh* m_pPxTriangleMesh = nullptr;
  ezBoundingBoxSphere m_Bounds;
  ezHybridArray<physx::PxConvexMesh*, 1> m_PxConvexParts;
};
