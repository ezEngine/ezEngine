#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
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
  m_pDescriptor = EZ_DEFAULT_NEW(ezSkeletonResourceDescriptor);
  *m_pDescriptor = std::move(descriptor);

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}

ezResourceLoadDesc ezSkeletonResource::UnloadData(Unload WhatToUnload)
{
  m_pDescriptor.Clear();

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  return res;
}

ezResourceLoadDesc ezSkeletonResource::UpdateContent(ezStreamReader* Stream)
{
  EZ_LOG_BLOCK("ezSkeletonResource::UpdateContent", GetResourceIdOrDescription());

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  // the standard file reader writes the absolute file path into the stream
  ezStringBuilder sAbsFilePath;
  (*Stream) >> sAbsFilePath;

  // skip the asset file header at the start of the file
  ezAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  m_pDescriptor = EZ_DEFAULT_NEW(ezSkeletonResourceDescriptor);
  m_pDescriptor->Deserialize(*Stream).IgnoreResult();

  res.m_State = ezResourceState::Loaded;
  return res;
}

void ezSkeletonResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezSkeletonResource); // TODO
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

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

ezUInt64 ezSkeletonResourceDescriptor::GetHeapMemoryUsage() const
{
  return m_Geometry.GetHeapMemoryUsage() + m_Skeleton.GetHeapMemoryUsage();
}

ezResult ezSkeletonResourceDescriptor::Serialize(ezStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(8);

  m_Skeleton.Save(inout_stream);
  inout_stream << m_RootTransform;
  inout_stream << m_fMaxImpulse;

  const ezUInt16 uiNumGeom = static_cast<ezUInt16>(m_Geometry.GetCount());
  inout_stream << uiNumGeom;

  for (ezUInt32 i = 0; i < uiNumGeom; ++i)
  {
    const auto& geo = m_Geometry[i];

    inout_stream << geo.m_uiAttachedToJoint;
    inout_stream << geo.m_Type;
    inout_stream << geo.m_Transform;

    EZ_SUCCEED_OR_RETURN(inout_stream.WriteArray(geo.m_VertexPositions));
    EZ_SUCCEED_OR_RETURN(inout_stream.WriteArray(geo.m_TriangleIndices));
  }

  // version 8
  inout_stream << m_uiLeftFootJoint;
  inout_stream << m_uiRightFootJoint;

  return EZ_SUCCESS;
}

ezResult ezSkeletonResourceDescriptor::Deserialize(ezStreamReader& inout_stream)
{
  const ezTypeVersion version = inout_stream.ReadVersion(8);

  if (version < 6)
    return EZ_FAILURE;

  m_Skeleton.Load(inout_stream);

  inout_stream >> m_RootTransform;

  if (version >= 7)
  {
    inout_stream >> m_fMaxImpulse;
  }

  m_Geometry.Clear();

  ezUInt16 uiNumGeom = 0;
  inout_stream >> uiNumGeom;
  m_Geometry.Reserve(uiNumGeom);

  for (ezUInt32 i = 0; i < uiNumGeom; ++i)
  {
    auto& geo = m_Geometry.ExpandAndGetRef();

    inout_stream >> geo.m_uiAttachedToJoint;
    inout_stream >> geo.m_Type;
    inout_stream >> geo.m_Transform;

    if (version <= 6)
    {
      ezStringBuilder sName;
      ezSurfaceResourceHandle hSurface;
      ezUInt8 uiCollisionLayer;

      inout_stream >> sName;
      inout_stream >> hSurface;
      inout_stream >> uiCollisionLayer;
    }

    if (version >= 7)
    {
      EZ_SUCCEED_OR_RETURN(inout_stream.ReadArray(geo.m_VertexPositions));
      EZ_SUCCEED_OR_RETURN(inout_stream.ReadArray(geo.m_TriangleIndices));
    }
  }

  if (version >= 8)
  {
    inout_stream >> m_uiLeftFootJoint;
    inout_stream >> m_uiRightFootJoint;
  }

  // make sure the geometry is sorted by bones
  // this allows to make the algorithm for creating the bone geometry more efficient
  m_Geometry.Sort([](const ezSkeletonResourceGeometry& lhs, const ezSkeletonResourceGeometry& rhs) -> bool
    { return lhs.m_uiAttachedToJoint < rhs.m_uiAttachedToJoint; });

  return EZ_SUCCESS;
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_Implementation_SkeletonResource);
