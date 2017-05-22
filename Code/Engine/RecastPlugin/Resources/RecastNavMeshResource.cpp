#include <PCH.h>
#include <RecastPlugin/Resources/RecastNavMeshResource.h>
#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/IO/ChunkStream.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRecastNavMeshResource, 1, ezRTTIDefaultAllocator<ezRecastNavMeshResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezRecastNavMeshResource::ezRecastNavMeshResource() 
  : ezResource<ezRecastNavMeshResource, ezRecastNavMeshResourceDescriptor>(DoUpdate::OnAnyThread, 1)
{
  ModifyMemoryUsage().m_uiMemoryCPU = sizeof(ezRecastNavMeshResource);
}

ezRecastNavMeshResource::~ezRecastNavMeshResource()
{
}

ezResourceLoadDesc ezRecastNavMeshResource::UnloadData(Unload WhatToUnload)
{
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  return res;
}

ezResourceLoadDesc ezRecastNavMeshResource::UpdateContent(ezStreamReader* Stream)
{
  EZ_LOG_BLOCK("ezRecastNavMeshResource::UpdateContent", GetResourceDescription().GetData());

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

  res.m_State = ezResourceState::Loaded;
  return res;
}

void ezRecastNavMeshResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  /// \todo Implement

  out_NewMemoryUsage.m_uiMemoryCPU = ModifyMemoryUsage().m_uiMemoryCPU;
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

ezResourceLoadDesc ezRecastNavMeshResource::CreateResource(const ezRecastNavMeshResourceDescriptor& descriptor)
{
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}





