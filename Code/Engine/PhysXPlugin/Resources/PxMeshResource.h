#pragma once

#include <PhysXPlugin/Basics.h>
#include <Core/ResourceManager/Resource.h>

typedef ezTypedResourceHandle<class ezPxMeshResource> ezPxMeshResourceHandle;
typedef ezTypedResourceHandle<class ezSurfaceResource> ezSurfaceResourceHandle;

struct ezMsgBuildNavMesh;

struct EZ_PHYSXPLUGIN_DLL ezPxMeshResourceDescriptor
{
  // empty, these types of resources must be loaded from file
};

class EZ_PHYSXPLUGIN_DLL ezPxMeshResource : public ezResource<ezPxMeshResource, ezPxMeshResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPxMeshResource, ezResourceBase);

public:
  ezPxMeshResource();
  ~ezPxMeshResource();

  /// \brief Returns the triangle collision mesh. If the mesh is a convex mesh, this will be a nullptr.
  physx::PxTriangleMesh* GetTriangleMesh() const { return m_pPxTriangleMesh; }

  /// \brief Returns the convex collision mesh. If the mesh is a triangle mesh, this will be a nullptr.
  physx::PxConvexMesh* GetConvexMesh() const { return m_pPxConvexMesh; }

  /// \brief Returns the array of default surfaces to be used with this mesh.
  ///
  /// Note the array may contain less surfaces than the mesh does. It may also contain invalid surface handles.
  /// Use the default physics material as a fallback.
  const ezDynamicArray<ezSurfaceResourceHandle>& GetSurfaces() const { return m_Surfaces; }

  /// \brief Adds the geometry of the triangle or convex mesh to the nav mesh descriptor
  void AddToNavMesh(const ezTransform& transform, ezMsgBuildNavMesh& msg) const;

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;
  virtual ezResourceLoadDesc CreateResource(const ezPxMeshResourceDescriptor& descriptor) override;

private:
  ezDynamicArray<ezSurfaceResourceHandle> m_Surfaces;
  physx::PxTriangleMesh* m_pPxTriangleMesh = nullptr;
  physx::PxConvexMesh* m_pPxConvexMesh = nullptr;
};

