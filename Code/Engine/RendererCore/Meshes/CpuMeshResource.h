#pragma once

#include <RendererCore/Meshes/MeshResourceDescriptor.h>

/// CPU-accessible mesh resource that stores mesh data in system memory.
///
/// Unlike regular ezMeshResource which stores data on the GPU, this resource keeps
/// the mesh descriptor in CPU memory. Used for scenarios requiring CPU access to
/// mesh data such as collision detection, raycasting, or procedural mesh generation.
class EZ_RENDERERCORE_DLL ezCpuMeshResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCpuMeshResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezCpuMeshResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezCpuMeshResource, ezMeshResourceDescriptor);

public:
  ezCpuMeshResource();

  /// Returns the mesh descriptor containing vertex and index data.
  const ezMeshResourceDescriptor& GetDescriptor() const { return m_Descriptor; }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  ezMeshResourceDescriptor m_Descriptor;
};

using ezCpuMeshResourceHandle = ezTypedResourceHandle<class ezCpuMeshResource>;
