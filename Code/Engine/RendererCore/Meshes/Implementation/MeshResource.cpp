#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshResource, 1, ezRTTIDefaultAllocator<ezMeshResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezMeshResource);
// clang-format on

ezUInt32 ezMeshResource::s_uiMeshBufferNameSuffix = 0;

ezMeshResource::ezMeshResource()
  : ezResource(DoUpdate::OnAnyThread, 1)
{
  m_Bounds = ezBoundingBoxSphere::MakeInvalid();
}

ezResourceLoadDesc ezMeshResource::UnloadData(Unload WhatToUnload)
{
  ezResourceLoadDesc res;
  res.m_State = GetLoadingState();
  res.m_uiQualityLevelsDiscardable = GetNumQualityLevelsDiscardable();
  res.m_uiQualityLevelsLoadable = GetNumQualityLevelsLoadable();

  // we currently can only unload the entire mesh
  // if (WhatToUnload == Unload::AllQualityLevels)
  {
    m_SubMeshes.Clear();
    m_SubMeshes.Compact();
    m_Materials.Clear();
    m_Materials.Compact();
    m_Bones.Clear();
    m_Bones.Compact();

    m_hMeshBuffer.Invalidate();
    m_hDefaultSkeleton.Invalidate();

    res.m_uiQualityLevelsDiscardable = 0;
    res.m_uiQualityLevelsLoadable = 0;
    res.m_State = ezResourceState::Unloaded;
  }

  return res;
}

ezResourceLoadDesc ezMeshResource::UpdateContent(ezStreamReader* Stream)
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
    ezStringBuilder sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  ezAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  if (desc.Load(*Stream).Failed())
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  return CreateResource(std::move(desc));
}

void ezMeshResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezMeshResource) + (ezUInt32)m_SubMeshes.GetHeapMemoryUsage() + (ezUInt32)m_Materials.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

EZ_RESOURCE_IMPLEMENT_CREATEABLE(ezMeshResource, ezMeshResourceDescriptor)
{
  // if there is an existing mesh buffer to use, take that
  m_hMeshBuffer = descriptor.GetExistingMeshBuffer();

  m_hDefaultSkeleton = descriptor.m_hDefaultSkeleton;
  m_Bones = descriptor.m_Bones;
  m_fMaxBoneVertexOffset = descriptor.m_fMaxBoneVertexOffset;

  // otherwise create a new mesh buffer from the descriptor
  if (!m_hMeshBuffer.IsValid())
  {
    s_uiMeshBufferNameSuffix++;
    ezStringBuilder sMbName;
    sMbName.SetFormat("{0}  [MeshBuffer {1}]", GetResourceID(), ezArgU(s_uiMeshBufferNameSuffix, 4, true, 16, true));

    // note: this gets move'd, might be invalid afterwards
    ezMeshBufferResourceDescriptor& mb = descriptor.MeshBufferDesc();

    m_hMeshBuffer = ezResourceManager::CreateResource<ezMeshBufferResource>(sMbName, std::move(mb), GetResourceDescription());
  }

  m_SubMeshes = descriptor.GetSubMeshes();

  m_Materials.Clear();
  m_Materials.Reserve(descriptor.GetMaterials().GetCount());

  // copy all the material assignments and load the materials
  for (const auto& mat : descriptor.GetMaterials())
  {
    ezMaterialResourceHandle hMat;

    if (!mat.m_sPath.IsEmpty())
      hMat = ezResourceManager::LoadResource<ezMaterialResource>(mat.m_sPath);

    m_Materials.PushBack(hMat); // may be an invalid handle
  }

  m_Bounds = descriptor.GetBounds();
  EZ_ASSERT_DEV(m_Bounds.IsValid(), "The mesh bounds are invalid. Make sure to call ezMeshResourceDescriptor::ComputeBounds()");

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshResource);
