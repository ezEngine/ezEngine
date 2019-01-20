#pragma once

#include <RendererCore/Meshes/MeshResourceDescriptor.h>

class EZ_RENDERERCORE_DLL ezCpuMeshResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCpuMeshResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezCpuMeshResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezCpuMeshResource, ezMeshResourceDescriptor);

public:
  ezCpuMeshResource();

  const ezMeshResourceDescriptor& GetDescriptor() const { return m_Descriptor; }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  ezMeshResourceDescriptor m_Descriptor;
};

typedef ezTypedResourceHandle<class ezCpuMeshResource> ezCpuMeshResourceHandle;
