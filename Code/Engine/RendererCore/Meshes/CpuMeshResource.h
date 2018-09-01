#pragma once

#include <RendererCore/Meshes/MeshResourceDescriptor.h>

class EZ_RENDERERCORE_DLL ezCpuMeshResource : public ezResource<ezCpuMeshResource, ezMeshResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCpuMeshResource, ezResourceBase);

public:
  ezCpuMeshResource();

  const ezMeshResourceDescriptor& GetDescriptor() const { return m_Descriptor; }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;
  virtual ezResourceLoadDesc CreateResource(const ezMeshResourceDescriptor& descriptor) override;

  ezMeshResourceDescriptor m_Descriptor;
};

typedef ezTypedResourceHandle<class ezCpuMeshResource> ezCpuMeshResourceHandle;
