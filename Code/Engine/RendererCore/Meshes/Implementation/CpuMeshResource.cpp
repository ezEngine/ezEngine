#include <PCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <RendererCore/Meshes/CpuMeshResource.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCpuMeshResource, 1, ezRTTIDefaultAllocator<ezCpuMeshResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezCpuMeshResource);
// clang-format on

ezCpuMeshResource::ezCpuMeshResource()
    : ezResource(DoUpdate::OnAnyThread, 1)
{
}

ezResourceLoadDesc ezCpuMeshResource::UnloadData(Unload WhatToUnload)
{
  ezResourceLoadDesc res;
  res.m_State = GetLoadingState();
  res.m_uiQualityLevelsDiscardable = GetNumQualityLevelsDiscardable();
  res.m_uiQualityLevelsLoadable = GetNumQualityLevelsLoadable();

  // we currently can only unload the entire mesh
  // if (WhatToUnload == Unload::AllQualityLevels)
  {
    m_Descriptor.Clear();

    res.m_uiQualityLevelsDiscardable = 0;
    res.m_uiQualityLevelsLoadable = 0;
    res.m_State = ezResourceState::Unloaded;
  }

  return res;
}

ezResourceLoadDesc ezCpuMeshResource::UpdateContent(ezStreamReader* Stream)
{
  ezMeshResourceDescriptor desc;
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
  AssetHash.Read(*Stream);

  if (m_Descriptor.Load(*Stream).Failed())
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  res.m_State = ezResourceState::Loaded;
  return res;
}

void ezCpuMeshResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezCpuMeshResource);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

EZ_RESOURCE_IMPLEMENT_CREATEABLE(ezCpuMeshResource, ezMeshResourceDescriptor)
{
  m_Descriptor = descriptor;

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}
