#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/Implementation/ReflectionPoolData.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeMapping.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Texture.h>

ezCVarInt cvar_RenderingReflectionPoolSkyLightRefreshFrames("Rendering.ReflectionPool.SkyLightRefreshFrames", 60, ezCVarFlags::Default, "How many frames must pass between two updates of a dynamic sky light. Each update invalidates every other reflection probe, so a low value keeps the scene busy re-rendering probes. Set to 0 to update the sky light every frame.");

ezReflectionProbeMapping::ezReflectionProbeMapping(ezUInt32 uiAtlasSize)
  : m_uiAtlasSize(uiAtlasSize)
{
  m_MappedCubes.SetCount(m_uiAtlasSize);
  m_ActiveProbes.Reserve(m_uiAtlasSize);
  m_UnusedProbeSlots.Reserve(m_uiAtlasSize);
  m_AddProbes.Reserve(m_uiAtlasSize);

  EZ_ASSERT_DEV(m_hReflectionSpecularTexture.IsInvalidated(), "World data already created.");
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezGALTextureCreationDescription desc;
  desc.m_uiWidth = s_uiReflectionCubeMapSize;
  desc.m_uiHeight = s_uiReflectionCubeMapSize;
  desc.m_uiMipLevelCount = GetMipLevels();
  desc.m_uiArraySize = s_uiNumReflectionProbeCubeMaps;
  desc.m_Format = ezGALResourceFormat::RGBAHalf;
  desc.m_Type = ezGALTextureType::TextureCubeArray;
  desc.m_TextureFlags.Add(ezGALTextureUsageFlags::UnorderedAccess | ezGALTextureUsageFlags::RenderTarget);
  desc.m_ResourceAccess.m_bImmutable = false;

  m_hReflectionSpecularTexture = pDevice->CreateTexture(desc);
  pDevice->GetTexture(m_hReflectionSpecularTexture)->SetDebugName("Reflection Specular Texture");
}

ezReflectionProbeMapping::~ezReflectionProbeMapping()
{
  EZ_ASSERT_DEV(!m_hReflectionSpecularTexture.IsInvalidated(), "World data not created.");
  ezGALDevice::GetDefaultDevice()->DestroyTexture(m_hReflectionSpecularTexture);
  m_hReflectionSpecularTexture.Invalidate();
}

void ezReflectionProbeMapping::AddProbe(ezReflectionProbeId probe, ezBitflags<ezProbeFlags> flags)
{
  m_RegisteredProbes.EnsureCount(probe.m_InstanceIndex + 1);
  ProbeDataInternal& probeData = m_RegisteredProbes[probe.m_InstanceIndex];
  EZ_ASSERT_DEBUG(probeData.m_Flags == 0, "");
  probeData.m_id = probe;
  probeData.m_Flags.SetValue(flags.GetValue());
  probeData.m_Flags.Add(ezProbeMappingFlags::Dirty);
  if (probeData.m_Flags.IsSet(ezProbeMappingFlags::SkyLight))
  {
    m_SkyLight = probe;
    MapProbe(probe, 0);
  }
}

void ezReflectionProbeMapping::UpdateProbe(ezReflectionProbeId probe, ezBitflags<ezProbeFlags> flags)
{
  ProbeDataInternal& probeData = m_RegisteredProbes[probe.m_InstanceIndex];
  if (!probeData.m_Flags.IsSet(ezProbeMappingFlags::SkyLight) && probeData.m_Flags.IsSet(ezProbeMappingFlags::Dynamic) != flags.IsSet(ezProbeFlags::Dynamic))
  {
    UnmapProbe(probe);
  }
  ezBitflags<ezProbeMappingFlags> preserveFlags = probeData.m_Flags & ezProbeMappingFlags::Usable;
  probeData.m_Flags.SetValue(flags.GetValue());
  probeData.m_Flags.Add(preserveFlags | ezProbeMappingFlags::Dirty);
}

void ezReflectionProbeMapping::ProbeUpdateFinished(ezReflectionProbeId probe)
{
  ProbeDataInternal& probeData0 = m_RegisteredProbes[probe.m_InstanceIndex];
  if (m_SkyLight == probe)
  {
    m_uiLastSkyLightUpdateFrame = ezRenderWorld::GetFrameCounter();
    m_bSkyLightUpdatedOnce = true;
  }
  if (m_SkyLight == probe && probeData0.m_Flags.IsSet(ezProbeMappingFlags::Dirty))
  {
    // If the sky irradiance changed all other probes are no longer valid and need to be marked as dirty.
    for (ProbeDataInternal& probeData : m_RegisteredProbes)
    {
      if (!probeData.m_id.IsInvalidated() && probeData.m_id != probe)
      {
        probeData.m_Flags.Add(ezProbeMappingFlags::Dirty);
      }
    }
  }
  probeData0.m_Flags.Add(ezProbeMappingFlags::Usable);
  probeData0.m_Flags.Remove(ezProbeMappingFlags::Dirty);
}

void ezReflectionProbeMapping::RemoveProbe(ezReflectionProbeId probe)
{
  if (m_SkyLight == probe)
  {
    m_SkyLight.Invalidate();
    // If the sky irradiance changed all other probes are no longer valid and need to be marked as dirty.
    for (ProbeDataInternal& probeData : m_RegisteredProbes)
    {
      if (!probeData.m_id.IsInvalidated() && probeData.m_id != probe)
      {
        probeData.m_Flags.Add(ezProbeMappingFlags::Dirty);
      }
    }
  }
  UnmapProbe(probe);
  ProbeDataInternal& probeData = m_RegisteredProbes[probe.m_InstanceIndex];
  probeData = {};
}

ezInt32 ezReflectionProbeMapping::GetReflectionIndex(ezReflectionProbeId probe, bool bForExtraction) const
{
  const ProbeDataInternal& probeData = m_RegisteredProbes[probe.m_InstanceIndex];
  if (bForExtraction && !probeData.m_Flags.IsSet(ezProbeMappingFlags::Usable))
  {
    return -1;
  }
  return probeData.m_uiReflectionIndex;
}

void ezReflectionProbeMapping::PreExtraction()
{
  // Reset priorities
  for (ProbeDataInternal& probeData : m_RegisteredProbes)
  {
    probeData.m_fPriority = 0.0f;
  }
  if (!m_SkyLight.IsInvalidated())
  {
    ProbeDataInternal& probeData = m_RegisteredProbes[m_SkyLight.m_InstanceIndex];
    probeData.m_fPriority = ezMath::MaxValue<float>();
  }

  m_SortedProbes.Clear();
  m_ActiveProbes.Clear();
  m_UnusedProbeSlots.Clear();
  m_AddProbes.Clear();
}

void ezReflectionProbeMapping::AddWeight(ezReflectionProbeId probe, float fPriority)
{
  ProbeDataInternal& probeData = m_RegisteredProbes[probe.m_InstanceIndex];
  probeData.m_fPriority = ezMath::Max(probeData.m_fPriority, fPriority);
}

void ezReflectionProbeMapping::PostExtraction()
{
  {
    // Sort all active non-skylight probes so we can find the best candidates to evict from the atlas.
    for (ezUInt32 i = 1; i < s_uiNumReflectionProbeCubeMaps; i++)
    {
      auto id = m_MappedCubes[i];
      if (!id.IsInvalidated())
      {
        m_ActiveProbes.PushBack({id, m_RegisteredProbes[id.m_InstanceIndex].m_fPriority});
      }
      else
      {
        m_UnusedProbeSlots.PushBack(i);
      }
    }
    m_ActiveProbes.Sort();
  }

  {
    // Sort all exiting probes by priority.
    m_SortedProbes.Reserve(m_RegisteredProbes.GetCount());
    for (const ProbeDataInternal& probeData : m_RegisteredProbes)
    {
      if (!probeData.m_id.IsInvalidated())
      {
        m_SortedProbes.PushBack({probeData.m_id, probeData.m_fPriority});
      }
    }
    m_SortedProbes.Sort();
  }

  {
    // Look at the first N best probes that would ideally be mapped in the atlas and find unmapped ones.
    const ezUInt32 uiMaxCount = ezMath::Min(m_uiAtlasSize, m_SortedProbes.GetCount());
    for (ezUInt32 i = 0; i < uiMaxCount; i++)
    {
      const SortedProbes& probe = m_SortedProbes[i];
      const ProbeDataInternal& probeData = m_RegisteredProbes[probe.m_uiIndex.m_InstanceIndex];

      if (probeData.m_uiReflectionIndex < 0)
      {
        // We found a better probe to be mapped to the atlas.
        m_AddProbes.PushBack(probe);
      }
    }
  }

  {
    // Trigger resource loading of static or updates of dynamic probes.
    const ezUInt32 uiMaxCount = m_AddProbes.GetCount();
    for (ezUInt32 i = 0; i < uiMaxCount; i++)
    {
      const SortedProbes& probe = m_AddProbes[i];
      const ProbeDataInternal& probeData = m_RegisteredProbes[probe.m_uiIndex.m_InstanceIndex];
      // #TODO static probe resource loading
    }
  }

  // Unmap probes in case we need free slots using results from last frame
  {
    // Only unmap one probe per frame
    // #TODO better heuristic to decide how many if any should be unmapped.
    if (m_UnusedProbeSlots.GetCount() == 0 && m_AddProbes.GetCount() > 0)
    {
      const SortedProbes probe = m_ActiveProbes.PeekBack();
      UnmapProbe(probe.m_uiIndex);
    }
  }

  // Map probes with higher priority
  {
    const ezUInt32 uiMaxCount = ezMath::Min(m_AddProbes.GetCount(), m_UnusedProbeSlots.GetCount());
    for (ezUInt32 i = 0; i < uiMaxCount; i++)
    {
      ezInt32 iReflectionIndex = m_UnusedProbeSlots[i];
      const SortedProbes probe = m_AddProbes[i];
      MapProbe(probe.m_uiIndex, iReflectionIndex);
    }
  }

  // Enqueue probe updates
  {
    // Newly mapped probes were appended by MapProbe, so restore the priority order before enqueuing.
    m_ActiveProbes.Sort();

    // The sky light drives the ambient of every other probe and its completion marks them all dirty, so it
    // has to be requested before everything else. It is not part of m_ActiveProbes (it occupies the fixed
    // atlas index 0, which the loops above skip), so it is handled separately here.
    if (!m_SkyLight.IsInvalidated())
    {
      const ProbeDataInternal& skyLightData = m_RegisteredProbes[m_SkyLight.m_InstanceIndex];
      const bool bFirstBake = !skyLightData.m_Flags.IsSet(ezProbeMappingFlags::Usable);

      // A sky light that has no content yet is always requested. Once it has content, refreshes are rate
      // limited, as each one invalidates all other probes and would otherwise keep the scene from settling.
      if (bFirstBake || IsSkyLightRefreshDue())
      {
        RequestUpdate(skyLightData);
      }
    }

    const ezUInt32 uiMaxCount = m_ActiveProbes.GetCount();
    for (ezUInt32 i = 0; i < uiMaxCount; i++)
    {
      const SortedProbes probe = m_ActiveProbes[i];
      const ProbeDataInternal& probeData = m_RegisteredProbes[probe.m_uiIndex.m_InstanceIndex];

      // #TODO Add static probes once resources are loaded.
      if (probeData.m_Flags.IsSet(ezProbeMappingFlags::Dynamic) || probeData.m_Flags.IsSet(ezProbeMappingFlags::Dirty))
      {
        RequestUpdate(probeData);
      }
    }
  }
}

void ezReflectionProbeMapping::RequestUpdate(const ProbeDataInternal& probeData)
{
  ezReflectionProbeMappingEvent e = {probeData.m_id, ezReflectionProbeMappingEvent::Type::ProbeUpdateRequested};
  e.m_fPriority = probeData.m_fPriority;
  e.m_bFirstBake = !probeData.m_Flags.IsSet(ezProbeMappingFlags::Usable);
  m_Events.Broadcast(e);
}

bool ezReflectionProbeMapping::IsSkyLightRefreshDue() const
{
  if (!m_bSkyLightUpdatedOnce)
    return true;

  const ezUInt64 uiInterval = (ezUInt64)ezMath::Max<ezInt64>(0, cvar_RenderingReflectionPoolSkyLightRefreshFrames);

  const ezUInt64 uiCurrentFrame = ezRenderWorld::GetFrameCounter();
  return uiCurrentFrame >= m_uiLastSkyLightUpdateFrame + uiInterval;
}

void ezReflectionProbeMapping::MapProbe(ezReflectionProbeId id, ezInt32 iReflectionIndex)
{
  ProbeDataInternal& probeData = m_RegisteredProbes[id.m_InstanceIndex];

  probeData.m_uiReflectionIndex = iReflectionIndex;
  m_MappedCubes[probeData.m_uiReflectionIndex] = id;
  // Push the actual priority so that the update enqueue order below stays sorted. A freshly mapped probe
  // has no content yet, so sorting it to the back would delay exactly the probes that need an update most.
  m_ActiveProbes.PushBack({id, probeData.m_fPriority});

  ezReflectionProbeMappingEvent e = {id, ezReflectionProbeMappingEvent::Type::ProbeMapped};
  m_Events.Broadcast(e);
}

void ezReflectionProbeMapping::UnmapProbe(ezReflectionProbeId id)
{
  ProbeDataInternal& probeData = m_RegisteredProbes[id.m_InstanceIndex];
  if (probeData.m_uiReflectionIndex != -1)
  {
    m_MappedCubes[probeData.m_uiReflectionIndex].Invalidate();
    probeData.m_uiReflectionIndex = -1;

    // The atlas slot is given up, so whatever content was rendered into it is gone. If the probe is mapped
    // again later it has to be treated as a first bake, otherwise it would be sampled before being rendered.
    probeData.m_Flags.Remove(ezProbeMappingFlags::Usable);
    probeData.m_Flags.Add(ezProbeMappingFlags::Dirty);

    ezReflectionProbeMappingEvent e = {id, ezReflectionProbeMappingEvent::Type::ProbeUnmapped};
    m_Events.Broadcast(e);
  }
}
