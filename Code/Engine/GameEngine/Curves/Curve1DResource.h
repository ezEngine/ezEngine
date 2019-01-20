#pragma once

#include <GameEngine/Basics.h>
#include <Core/ResourceManager/Resource.h>
#include <Foundation/Tracks/Curve1D.h>

/// \brief A curve resource can contain more than one curve, but all of the same type.
struct EZ_GAMEENGINE_DLL ezCurve1DResourceDescriptor
{
  ezDynamicArray<ezCurve1D> m_Curves;

  void Save(ezStreamWriter& stream) const;
  void Load(ezStreamReader& stream);
};

typedef ezTypedResourceHandle<class ezCurve1DResource> ezCurve1DResourceHandle;

/// \brief A resource that stores 1D curves. The curves are stored in the descriptor.
class EZ_GAMEENGINE_DLL ezCurve1DResource : public ezResource
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


