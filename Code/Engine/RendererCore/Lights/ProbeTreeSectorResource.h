#pragma once

#include <Core/Graphics/AmbientCubeBasis.h>
#include <Core/ResourceManager/Resource.h>
#include <RendererCore/Declarations.h>

typedef ezTypedResourceHandle<class ezProbeTreeSectorResource> ezProbeTreeSectorResourceHandle;

struct EZ_RENDERERCORE_DLL ezProbeTreeSectorResourceDescriptor
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezProbeTreeSectorResourceDescriptor);

  ezProbeTreeSectorResourceDescriptor();
  ~ezProbeTreeSectorResourceDescriptor();
  void operator=(ezProbeTreeSectorResourceDescriptor&& other);

  ezDynamicArray<ezVec3> m_ProbePositions;
  ezDynamicArray<ezAmbientCube<ezUInt8>> m_SkyVisibility;

  void Clear();
  ezUInt32 GetHeapMemoryUsage() const;

  ezResult Serialize(ezStreamWriter& stream) const;
  ezResult Deserialize(ezStreamReader& stream);
};

class EZ_RENDERERCORE_DLL ezProbeTreeSectorResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProbeTreeSectorResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezProbeTreeSectorResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezProbeTreeSectorResource, ezProbeTreeSectorResourceDescriptor);

public:
  ezProbeTreeSectorResource();
  ~ezProbeTreeSectorResource();

  ezArrayPtr<const ezVec3> GetProbePositions() const { return m_Desc.m_ProbePositions; }
  ezArrayPtr<const ezAmbientCube<ezUInt8>> GetSkyVisibility() const { return m_Desc.m_SkyVisibility; }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  ezProbeTreeSectorResourceDescriptor m_Desc;
};
