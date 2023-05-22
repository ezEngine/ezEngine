#pragma once

#include <Core/Graphics/AmbientCubeBasis.h>
#include <Core/ResourceManager/Resource.h>
#include <RendererCore/BakedProbes/BakingUtils.h>

using ezProbeTreeSectorResourceHandle = ezTypedResourceHandle<class ezProbeTreeSectorResource>;

struct EZ_RENDERERCORE_DLL ezProbeTreeSectorResourceDescriptor
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezProbeTreeSectorResourceDescriptor);

  ezProbeTreeSectorResourceDescriptor();
  ~ezProbeTreeSectorResourceDescriptor();
  ezProbeTreeSectorResourceDescriptor& operator=(ezProbeTreeSectorResourceDescriptor&& other);

  ezVec3 m_vGridOrigin;
  ezVec3 m_vProbeSpacing;
  ezVec3U32 m_vProbeCount;

  ezDynamicArray<ezVec3> m_ProbePositions;
  ezDynamicArray<ezCompressedSkyVisibility> m_SkyVisibility;

  void Clear();
  ezUInt64 GetHeapMemoryUsage() const;

  ezResult Serialize(ezStreamWriter& inout_stream) const;
  ezResult Deserialize(ezStreamReader& inout_stream);
};

class EZ_RENDERERCORE_DLL ezProbeTreeSectorResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProbeTreeSectorResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezProbeTreeSectorResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezProbeTreeSectorResource, ezProbeTreeSectorResourceDescriptor);

public:
  ezProbeTreeSectorResource();
  ~ezProbeTreeSectorResource();

  const ezVec3& GetGridOrigin() const { return m_Desc.m_vGridOrigin; }
  const ezVec3& GetProbeSpacing() const { return m_Desc.m_vProbeSpacing; }
  const ezVec3U32& GetProbeCount() const { return m_Desc.m_vProbeCount; }

  ezArrayPtr<const ezVec3> GetProbePositions() const { return m_Desc.m_ProbePositions; }
  ezArrayPtr<const ezCompressedSkyVisibility> GetSkyVisibility() const { return m_Desc.m_SkyVisibility; }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  ezProbeTreeSectorResourceDescriptor m_Desc;
};
