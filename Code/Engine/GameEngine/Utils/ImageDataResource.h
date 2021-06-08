#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/World/Declarations.h>
#include <Texture/Image/Image.h>

struct ezImageDataResourceDescriptor
{
  ezImage m_Image;

  // ezResult Serialize(ezStreamWriter& stream) const;
  // ezResult Deserialize(ezStreamReader& stream);
};

class EZ_GAMEENGINE_DLL ezImageDataResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezImageDataResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezImageDataResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezImageDataResource, ezImageDataResourceDescriptor);

public:
  ezImageDataResource();
  ~ezImageDataResource();

  const ezImageDataResourceDescriptor& GetDescriptor() const { return *m_Descriptor; }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  ezUniquePtr<ezImageDataResourceDescriptor> m_Descriptor;
};

using ezImageDataResourceHandle = ezTypedResourceHandle<ezImageDataResource>;
