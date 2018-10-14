#pragma once

#include <KrautPlugin/Basics.h>
#include <Core/ResourceManager/Resource.h>
#include <Foundation/Math/BoundingBoxSphere.h>

typedef ezTypedResourceHandle<class ezKrautTreeResource> ezKrautTreeResourceHandle;

struct EZ_KRAUTPLUGIN_DLL ezKrautTreeResourceDescriptor
{
  void Save(ezStreamWriter& stream) const;
  ezResult Load(ezStreamReader& stream);

  ezBoundingBoxSphere m_Bounds;

  ezDynamicArray<ezVec3> m_Positions;
  ezDynamicArray<ezUInt32> m_TriangleIndices;
};

class EZ_KRAUTPLUGIN_DLL ezKrautTreeResource : public ezResource<ezKrautTreeResource, ezKrautTreeResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezKrautTreeResource, ezResourceBase);

public:
  ezKrautTreeResource();

  /// \brief Returns the bounds of this tree.
  const ezBoundingBoxSphere& GetBounds() const { return m_Descriptor.m_Bounds; }

  ezKrautTreeResourceDescriptor m_Descriptor;

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;
  virtual ezResourceLoadDesc CreateResource(const ezKrautTreeResourceDescriptor& descriptor) override;

};

