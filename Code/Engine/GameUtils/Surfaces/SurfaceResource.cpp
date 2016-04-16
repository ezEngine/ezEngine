#include <GameUtils/PCH.h>
#include <GameUtils/Surfaces/SurfaceResource.h>
#include <CoreUtils/Assets/AssetFileHeader.h>
#include <Core/WorldSerializer/ResourceHandleReader.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSurfaceResource, 1, ezRTTIDefaultAllocator<ezSurfaceResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezEvent<const ezSurfaceResource::Event&, ezMutex> ezSurfaceResource::s_Events;

ezSurfaceResource::ezSurfaceResource() : ezResource<ezSurfaceResource, ezSurfaceResourceDescriptor>(DoUpdate::OnMainThread, 1)
{
  m_pPhysicsMaterial = nullptr;
}

ezSurfaceResource::~ezSurfaceResource()
{
  EZ_ASSERT_DEV(m_pPhysicsMaterial == nullptr, "Physics material has not been cleaned up properly");
}

ezResourceLoadDesc ezSurfaceResource::UnloadData(Unload WhatToUnload)
{
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  Event e;
  e.m_pSurface = this;
  e.m_Type = Event::Type::Destroyed;
  s_Events.Broadcast(e);

  return res;
}

ezResourceLoadDesc ezSurfaceResource::UpdateContent(ezStreamReader* Stream)
{
  EZ_LOG_BLOCK("ezSurfaceResource::UpdateContent", GetResourceDescription().GetData());

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

  {
    ezResourceHandleReadContext context;
    context.BeginReadingFromStream(Stream);
    context.BeginRestoringHandles(Stream);

    m_Descriptor.Load(*Stream);

    context.EndReadingFromStream(Stream);
    context.EndRestoringHandles();

    CreateResource(m_Descriptor);
  }

  res.m_State = ezResourceState::Loaded;
  return res;
}

void ezSurfaceResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezSurfaceResource);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

ezResourceLoadDesc ezSurfaceResource::CreateResource(const ezSurfaceResourceDescriptor& descriptor)
{
  m_Descriptor = descriptor;

  Event e;
  e.m_pSurface = this;
  e.m_Type = Event::Type::Created;
  s_Events.Broadcast(e);

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}

