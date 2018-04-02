#pragma once

#include <RendererCore/Basics.h>
#include <Core/ResourceManager/Resource.h>

struct EZ_RENDERERCORE_DLL ezSkeletonResourceDescriptor
{


  void Save(ezStreamWriter& stream) const;
  void Load(ezStreamReader& stream);
};

typedef ezTypedResourceHandle<class ezSkeletonResource> ezSkeletonResourceHandle;

class EZ_RENDERERCORE_DLL ezSkeletonResource : public ezResource<ezSkeletonResource, ezSkeletonResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSkeletonResource, ezResourceBase);

public:
  ezSkeletonResource();


private:
  virtual ezResourceLoadDesc CreateResource(const ezSkeletonResourceDescriptor& descriptor) override;
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  ezSkeletonResourceDescriptor m_Descriptor;
};


