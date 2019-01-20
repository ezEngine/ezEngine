#pragma once

#include <RecastPlugin/Basics.h>
#include <Core/ResourceManager/Resource.h>

typedef ezTypedResourceHandle<class ezRecastNavMeshResource> ezRecastNavMeshResourceHandle;

struct EZ_RECASTPLUGIN_DLL ezRecastNavMeshResourceDescriptor
{
};

class EZ_RECASTPLUGIN_DLL ezRecastNavMeshResource : public ezResource<ezRecastNavMeshResource, ezRecastNavMeshResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRecastNavMeshResource, ezResourceBase);

public:
  ezRecastNavMeshResource();
  ~ezRecastNavMeshResource();

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;
  virtual ezResourceLoadDesc CreateResource(ezRecastNavMeshResourceDescriptor&& descriptor) override;

};

