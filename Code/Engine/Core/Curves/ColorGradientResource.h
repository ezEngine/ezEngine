#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Tracks/ColorGradient.h>

struct EZ_CORE_DLL ezColorGradientResourceDescriptor
{
  ezColorGradient m_Gradient;

  void Save(ezStreamWriter& inout_stream) const;
  void Load(ezStreamReader& inout_stream);
};

using ezColorGradientResourceHandle = ezTypedResourceHandle<class ezColorGradientResource>;

/// \brief A resource that stores a single color gradient. The data is stored in the descriptor.
class EZ_CORE_DLL ezColorGradientResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezColorGradientResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezColorGradientResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezColorGradientResource, ezColorGradientResourceDescriptor);

public:
  ezColorGradientResource();

  /// \brief Returns all the data that is stored in this resource.
  const ezColorGradientResourceDescriptor& GetDescriptor() const { return m_Descriptor; }

  inline ezColor Evaluate(double x) const
  {
    ezColor result;
    m_Descriptor.m_Gradient.Evaluate(x, result);
    return result;
  }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  ezColorGradientResourceDescriptor m_Descriptor;
};
