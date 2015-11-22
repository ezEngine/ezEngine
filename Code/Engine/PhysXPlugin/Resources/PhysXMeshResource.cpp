#include <PhysXPlugin/PCH.h>
#include <PhysXPlugin/Resources/PhysXMeshResource.h>
#include <CoreUtils/Assets/AssetFileHeader.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPhysXMeshResource, 1, ezRTTIDefaultAllocator<ezPhysXMeshResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezPhysXMeshResource::ezPhysXMeshResource() : ezResource<ezPhysXMeshResource, ezPhysXMeshResourceDescriptor>(DoUpdate::OnMainThread, 1)
{
  m_pPxMesh = nullptr;

  ModifyMemoryUsage().m_uiMemoryCPU = sizeof(ezPhysXMeshResource);
}

ezPhysXMeshResource::~ezPhysXMeshResource()
{
  EZ_ASSERT_DEBUG(m_pPxMesh == nullptr, "Collision mesh was not unloaded correctly");
}

ezResourceLoadDesc ezPhysXMeshResource::UnloadData(Unload WhatToUnload)
{
  if (m_pPxMesh)
  {
    // since it is ref-counted, it may still be in use by the SDK, but we don't bother about that here
    m_pPxMesh->release();
    m_pPxMesh = nullptr;
  }

  // we cannot compute this in UpdateMemoryUsage(), so we only read the data there, therefore we need to update this information here
  /// \todo Compute memory usage
  ModifyMemoryUsage().m_uiMemoryCPU = sizeof(ezPhysXMeshResource);

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  return res;
}

ezResourceLoadDesc ezPhysXMeshResource::UpdateContent(ezStreamReader* Stream)
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
  AssetHash.Read(*Stream);

  /// \todo Load cooked mesh

  //ezPhysXMeshResourceDescriptor desc;
  //if (desc.Load(*Stream).Failed())
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  res.m_State = ezResourceState::Loaded;
  return res;
}

void ezPhysXMeshResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  // we cannot compute this data here, so we update it wherever we know the memory usage

  out_NewMemoryUsage.m_uiMemoryCPU = ModifyMemoryUsage().m_uiMemoryCPU;
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

ezResourceLoadDesc ezPhysXMeshResource::CreateResource(const ezPhysXMeshResourceDescriptor& descriptor)
{
  EZ_REPORT_FAILURE("This resource type does not support creating data.");

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}


