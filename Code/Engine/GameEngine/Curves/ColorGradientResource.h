#pragma once

#include <GameEngine/Basics.h>
#include <Core/ResourceManager/Resource.h>
#include <Foundation/Math/ColorGradient.h>

struct EZ_GAMEENGINE_DLL ezColorGradientResourceDescriptor
{
  ezColorGradient m_Gradient;

  void Save(ezStreamWriter& stream) const;
  void Load(ezStreamReader& stream);
};

typedef ezTypedResourceHandle<class ezColorGradientResource> ezColorGradientResourceHandle;

/// \brief A resource that stores a single color gradient. The data is stored in the descriptor.
class EZ_GAMEENGINE_DLL ezColorGradientResource : public ezResource<ezColorGradientResource, ezColorGradientResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezColorGradientResource, ezResourceBase);

public:
  ezColorGradientResource();

  /// \brief Returns all the data that is stored in this resource.
  const ezColorGradientResourceDescriptor& GetDescriptor() const { return m_Descriptor; }

private:
  virtual ezResourceLoadDesc CreateResource(const ezColorGradientResourceDescriptor& descriptor) override;
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  ezColorGradientResourceDescriptor m_Descriptor;
};


