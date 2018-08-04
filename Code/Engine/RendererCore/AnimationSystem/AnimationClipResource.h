#pragma once

#include <Core/ResourceManager/Resource.h>
#include <RendererCore/Basics.h>

struct EZ_RENDERERCORE_DLL ezAnimationClipResourceDescriptor
{


  void Save(ezStreamWriter& stream) const;
  void Load(ezStreamReader& stream);
};

typedef ezTypedResourceHandle<class ezAnimationClipResource> ezAnimationClipResourceHandle;

class EZ_RENDERERCORE_DLL ezAnimationClipResource : public ezResource<ezAnimationClipResource, ezAnimationClipResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimationClipResource, ezResourceBase);

public:
  ezAnimationClipResource();


private:
  virtual ezResourceLoadDesc CreateResource(const ezAnimationClipResourceDescriptor& descriptor) override;
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  ezAnimationClipResourceDescriptor m_Descriptor;
};
