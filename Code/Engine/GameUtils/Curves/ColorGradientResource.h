#pragma once

#include <GameUtils/Basics.h>
#include <Core/ResourceManager/Resource.h>
#include <Foundation/Math/ColorGradient.h>

struct EZ_GAMEUTILS_DLL ezColorGradientResourceDescriptor
{
  ezColorGradient m_Gradient;

  void Save(ezStreamWriter& stream) const;
  void Load(ezStreamReader& stream);
};

typedef ezTypedResourceHandle<class ezColorGradientResource> ezColorGradientResourceHandle;

class EZ_GAMEUTILS_DLL ezColorGradientResource : public ezResource<ezColorGradientResource, ezColorGradientResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezColorGradientResource, ezResourceBase);

public:
  ezColorGradientResource();

  const ezColorGradientResourceDescriptor& GetDescriptor() const { return m_Descriptor; }

private:
  virtual ezResourceLoadDesc CreateResource(const ezColorGradientResourceDescriptor& descriptor) override;
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  ezColorGradientResourceDescriptor m_Descriptor;
};


