#pragma once

#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>

typedef ezResourceHandle<class ezMaterialResource> ezMaterialResourceHandle;

class EZ_RENDERERCORE_DLL ezMeshResource : public ezResource<ezMeshResource, ezMeshResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMeshResource);

public:
  ezMeshResource();

  /// \brief Returns the array of sub-meshes in this mesh.
  const ezDynamicArray<ezMeshResourceDescriptor::SubMesh>& GetSubMeshes() const
  {
    return m_SubMeshes;
  }

  /// \brief Returns the mesh buffer that is used by this resource.
  const ezMeshBufferResourceHandle& GetMeshBuffer() const
  {
    return m_hMeshBuffer;
  }

  /// \brief Returns the default materials for this mesh.
  const ezDynamicArray<ezMaterialResourceHandle>& GetMaterials() const
  {
    return m_Materials;
  }

  /// \brief Returns the bounds of this mesh.
  const ezBoundingBoxSphere& GetBounds() const
  {
    return m_Bounds;
  }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReaderBase* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;
  virtual ezResourceLoadDesc CreateResource(const ezMeshResourceDescriptor& descriptor) override;

  ezDynamicArray<ezMeshResourceDescriptor::SubMesh> m_SubMeshes;
  ezMeshBufferResourceHandle m_hMeshBuffer;
  ezDynamicArray<ezMaterialResourceHandle> m_Materials;

  ezBoundingBoxSphere m_Bounds;

  static ezUInt32 s_MeshBufferNameSuffix;
};

typedef ezResourceHandle<ezMeshResource> ezMeshResourceHandle;
