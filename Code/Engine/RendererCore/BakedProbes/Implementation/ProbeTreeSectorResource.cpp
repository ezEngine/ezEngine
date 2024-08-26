#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/ChunkStream.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <RendererCore/BakedProbes/ProbeTreeSectorResource.h>

ezProbeTreeSectorResourceDescriptor::ezProbeTreeSectorResourceDescriptor() = default;
ezProbeTreeSectorResourceDescriptor::~ezProbeTreeSectorResourceDescriptor() = default;
ezProbeTreeSectorResourceDescriptor& ezProbeTreeSectorResourceDescriptor::operator=(ezProbeTreeSectorResourceDescriptor&& other) = default;

void ezProbeTreeSectorResourceDescriptor::Clear()
{
  m_ProbePositions.Clear();
  m_SkyVisibility.Clear();
}

ezUInt64 ezProbeTreeSectorResourceDescriptor::GetHeapMemoryUsage() const
{
  ezUInt64 uiMemUsage = 0;
  uiMemUsage += m_ProbePositions.GetHeapMemoryUsage();
  uiMemUsage += m_SkyVisibility.GetHeapMemoryUsage();
  return uiMemUsage;
}

static ezTypeVersion s_ProbeTreeResourceDescriptorVersion = 1;
ezResult ezProbeTreeSectorResourceDescriptor::Serialize(ezStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(s_ProbeTreeResourceDescriptorVersion);

  inout_stream << m_vGridOrigin;
  inout_stream << m_vProbeSpacing;
  inout_stream << m_vProbeCount;

  EZ_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_ProbePositions));
  EZ_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_SkyVisibility));

  return EZ_SUCCESS;
}

ezResult ezProbeTreeSectorResourceDescriptor::Deserialize(ezStreamReader& inout_stream)
{
  Clear();

  const ezTypeVersion version = inout_stream.ReadVersion(s_ProbeTreeResourceDescriptorVersion);
  EZ_IGNORE_UNUSED(version);

  inout_stream >> m_vGridOrigin;
  inout_stream >> m_vProbeSpacing;
  inout_stream >> m_vProbeCount;

  EZ_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_ProbePositions));
  EZ_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_SkyVisibility));

  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProbeTreeSectorResource, 1, ezRTTIDefaultAllocator<ezProbeTreeSectorResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezProbeTreeSectorResource);
// clang-format on

ezProbeTreeSectorResource::ezProbeTreeSectorResource()
  : ezResource(DoUpdate::OnAnyThread, 1)
{
}

ezProbeTreeSectorResource::~ezProbeTreeSectorResource() = default;

ezResourceLoadDesc ezProbeTreeSectorResource::UnloadData(Unload WhatToUnload)
{
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  m_Desc.Clear();

  return res;
}

ezResourceLoadDesc ezProbeTreeSectorResource::UpdateContent(ezStreamReader* Stream)
{
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    ezString sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  ezAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  ezProbeTreeSectorResourceDescriptor descriptor;
  if (descriptor.Deserialize(*Stream).Failed())
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  return CreateResource(std::move(descriptor));
}

void ezProbeTreeSectorResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezProbeTreeSectorResource);
  out_NewMemoryUsage.m_uiMemoryCPU += m_Desc.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

ezResourceLoadDesc ezProbeTreeSectorResource::CreateResource(ezProbeTreeSectorResourceDescriptor&& descriptor)
{
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  m_Desc = std::move(descriptor);

  return res;
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_BakedProbes_Implementation_ProbeTreeSectorResource);
