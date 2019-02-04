#pragma once

#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>

typedef ezTypedResourceHandle<class ezMaterialResource> ezMaterialResourceHandle;

class EZ_RENDERERCORE_DLL ezMeshResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMeshResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezMeshResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezMeshResource, ezMeshResourceDescriptor);

public:
  ezMeshResource();

  /// \brief Returns the array of sub-meshes in this mesh.
  ezArrayPtr<const ezMeshResourceDescriptor::SubMesh> GetSubMeshes() const { return m_SubMeshes; }

  /// \brief Returns the mesh buffer that is used by this resource.
  const ezMeshBufferResourceHandle& GetMeshBuffer() const { return m_hMeshBuffer; }

  /// \brief Returns the default materials for this mesh.
  ezArrayPtr<const ezMaterialResourceHandle> GetMaterials() const { return m_Materials; }

  /// \brief Returns the bounds of this mesh.
  const ezBoundingBoxSphere& GetBounds() const { return m_Bounds; }

  /// \brief Returns the skeleton for this mesh. Will be an invalid handle for static meshes.
  const ezSkeletonResourceHandle& GetSkeleton() const { return m_hSkeleton; }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  ezDynamicArray<ezMeshResourceDescriptor::SubMesh> m_SubMeshes;
  ezMeshBufferResourceHandle m_hMeshBuffer;
  ezDynamicArray<ezMaterialResourceHandle> m_Materials;
  ezSkeletonResourceHandle m_hSkeleton;

  ezBoundingBoxSphere m_Bounds;

  static ezUInt32 s_MeshBufferNameSuffix;
};

typedef ezTypedResourceHandle<class ezMeshResource> ezMeshResourceHandle;

