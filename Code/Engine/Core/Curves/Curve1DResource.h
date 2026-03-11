#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Tracks/Curve1D.h>

/// \brief Descriptor for 1D curve resources containing multiple curves and serialization methods.
///
/// A curve resource can contain more than one curve, but all curves are of the same type.
/// This allows grouping related curves together for efficiency and logical organization.
struct EZ_CORE_DLL ezCurve1DResourceDescriptor
{
  ezDynamicArray<ezCurve1D> m_Curves;

  void Save(ezStreamWriter& inout_stream) const;
  void Load(ezStreamReader& inout_stream);
};

using ezCurve1DResourceHandle = ezTypedResourceHandle<class ezCurve1DResource>;

/// \brief A resource that stores multiple 1D curves for animation and value interpolation.
///
/// 1D curve resources contain mathematical curves that map time or other input values to
/// output values. Commonly used for animations, easing functions, and procedural value
/// generation where smooth interpolation is needed.
class EZ_CORE_DLL ezCurve1DResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCurve1DResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezCurve1DResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezCurve1DResource, ezCurve1DResourceDescriptor);

public:
  ezCurve1DResource();

  /// \brief Returns all the data that is stored in this resource.
  const ezCurve1DResourceDescriptor& GetDescriptor() const { return m_Descriptor; }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  ezCurve1DResourceDescriptor m_Descriptor;
};
