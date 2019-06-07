#pragma once

#include <Core/ResourceManager/Resource.h>
#include <RecastPlugin/RecastPluginDLL.h>

typedef ezTypedResourceHandle<class ezRecastNavMeshResource> ezRecastNavMeshResourceHandle;

struct EZ_RECASTPLUGIN_DLL ezRecastNavMeshResourceDescriptor
{
};

class EZ_RECASTPLUGIN_DLL ezRecastNavMeshResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRecastNavMeshResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezRecastNavMeshResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezRecastNavMeshResource, ezRecastNavMeshResourceDescriptor);

public:
  ezRecastNavMeshResource();
  ~ezRecastNavMeshResource();

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;
};
