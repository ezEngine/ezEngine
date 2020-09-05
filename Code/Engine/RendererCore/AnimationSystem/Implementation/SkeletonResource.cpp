#include <RendererCorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <RendererCore/AnimationSystem/Implementation/OzzUtils.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/io/archive.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSkeletonResource, 1, ezRTTIDefaultAllocator<ezSkeletonResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezSkeletonResource);
// clang-format on

ezSkeletonResource::ezSkeletonResource()
  : ezResource(DoUpdate::OnAnyThread, 1)
{
}

ezSkeletonResource::~ezSkeletonResource() = default;

EZ_RESOURCE_IMPLEMENT_CREATEABLE(ezSkeletonResource, ezSkeletonResourceDescriptor)
{
  m_Descriptor = std::move(descriptor);

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
  res.m_uiQualityLevelsLoadable = 0;
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
    ezStringBuilder sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  // skip the asset file header at the start of the file
  ezAssetFileHeader AssetHash;
  AssetHash.Read(*Stream);

  m_Descriptor.Deserialize(*Stream);

  res.m_State = ezResourceState::Loaded;
  return res;
}

void ezSkeletonResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezSkeletonResource); // TODO
}

ezSkeletonResourceDescriptor::ezSkeletonResourceDescriptor() = default;
ezSkeletonResourceDescriptor::~ezSkeletonResourceDescriptor() = default;
ezSkeletonResourceDescriptor::ezSkeletonResourceDescriptor(ezSkeletonResourceDescriptor&& rhs)
{
  *this = std::move(rhs);
}

void ezSkeletonResourceDescriptor::operator=(ezSkeletonResourceDescriptor&& rhs)
{
  m_Skeleton = std::move(rhs.m_Skeleton);
  m_Geometry = std::move(rhs.m_Geometry);
}

ezResult ezSkeletonResourceDescriptor::Serialize(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  m_Skeleton.Save(stream);

  const ezUInt16 uiNumGeom = m_Geometry.GetCount();
  stream << uiNumGeom;

  for (ezUInt32 i = 0; i < uiNumGeom; ++i)
  {
    const auto& geo = m_Geometry[i];

    stream << geo.m_uiAttachedToJoint;
    stream << geo.m_Type;
    stream << geo.m_Transform;
  }

  return EZ_SUCCESS;
}

ezResult ezSkeletonResourceDescriptor::Deserialize(ezStreamReader& stream)
{
  stream.ReadVersion(1);

  m_Geometry.Clear();

  m_Skeleton.Load(stream);

  ezUInt16 uiNumGeom = 0;
  stream >> uiNumGeom;
  m_Geometry.Reserve(uiNumGeom);

  for (ezUInt32 i = 0; i < uiNumGeom; ++i)
  {
    auto& geo = m_Geometry.ExpandAndGetRef();

    stream >> geo.m_uiAttachedToJoint;
    stream >> geo.m_Type;
    stream >> geo.m_Transform;
  }

  return EZ_SUCCESS;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_Implementation_SkeletonResource);
