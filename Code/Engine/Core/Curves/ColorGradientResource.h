#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Tracks/ColorGradient.h>

/// \brief Descriptor for color gradient resources containing the gradient data and serialization methods.
struct EZ_CORE_DLL ezColorGradientResourceDescriptor
{
  ezColorGradient m_Gradient;

  void Save(ezStreamWriter& inout_stream) const;
  void Load(ezStreamReader& inout_stream);
};

using ezColorGradientResourceHandle = ezTypedResourceHandle<class ezColorGradientResource>;

/// \brief A resource that stores a single color gradient for use in rendering and effects.
///
/// Color gradient resources allow artists to define color transitions that can be evaluated
/// at runtime. Commonly used for particle effects, UI elements, and other visual systems
/// that need smooth color transitions.
class EZ_CORE_DLL ezColorGradientResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezColorGradientResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezColorGradientResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezColorGradientResource, ezColorGradientResourceDescriptor);

public:
  ezColorGradientResource();

  /// \brief Returns all the data that is stored in this resource.
  const ezColorGradientResourceDescriptor& GetDescriptor() const { return m_Descriptor; }

  /// \brief Evaluates the color gradient at the given position and returns the interpolated color.
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
