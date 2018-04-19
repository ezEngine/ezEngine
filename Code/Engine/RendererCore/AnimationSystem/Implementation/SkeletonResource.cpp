#include <PCH.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <Core/Assets/AssetFileHeader.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSkeletonResource, 1, ezRTTIDefaultAllocator<ezSkeletonResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezSkeletonResource::ezSkeletonResource()
  : ezResource<ezSkeletonResource, ezSkeletonResourceDescriptor>(DoUpdate::OnAnyThread, 1)
{
}

ezSkeletonResource::~ezSkeletonResource() = default;

ezResourceLoadDesc ezSkeletonResource::CreateResource(const ezSkeletonResourceDescriptor& descriptor)
{
  m_Descriptor = descriptor;

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}

ezResourceLoadDesc ezSkeletonResource::UnloadData(Unload WhatToUnload)
{
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 1;
  res.m_State = ezResourceState::Unloaded;

  return res;
}

ezResourceLoadDesc ezSkeletonResource::UpdateContent(ezStreamReader* Stream)
{
  EZ_LOG_BLOCK("ezSkeletonResource::UpdateContent", GetResourceDescription().GetData());

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

  // skip the asset file header at the start of the file
  ezAssetFileHeader AssetHash;
  AssetHash.Read(*Stream);

  m_Descriptor.Load(*Stream);

  res.m_State = ezResourceState::Loaded;
  return res;
}

void ezSkeletonResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezSkeletonResource); // TODO
}

void ezSkeletonResourceDescriptor::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = 1;
  stream << uiVersion;

  m_Skeleton.Save(stream);
}

void ezSkeletonResourceDescriptor::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion == 1, "Invalid skeleton descriptor version {0}", uiVersion);

  m_Skeleton.Load(stream);
}


