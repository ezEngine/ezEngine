#include <RendererCore/PCH.h>
#include <RendererCore/Meshes/MeshResource.h>

#include <Core/ResourceManager/ResourceManager.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshResource, ezResourceBase, 1, ezRTTIDefaultAllocator<ezMeshResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezMeshResource::ezMeshResource() : ezResource<ezMeshResource, ezMeshResourceDescriptor>(UpdateResource::OnAnyThread, 1)
{
}

ezResourceLoadDesc ezMeshResource::UnloadData(Unload WhatToUnload)
{
  m_Parts.Clear();
  m_uiMaterialCount = 0;

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  return res;
}

ezResourceLoadDesc ezMeshResource::UpdateContent(ezStreamReaderBase* Stream)
{
  EZ_ASSERT_NOT_IMPLEMENTED;

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;
  
  return res;
}

void ezMeshResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezMeshResource) + (ezUInt32) m_Parts.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

ezResourceLoadDesc ezMeshResource::CreateResource(const ezMeshResourceDescriptor& descriptor)
{
  /// \todo proper implementation
  ezResourceLock<ezMeshBufferResource> pMeshBuffer(descriptor.hMeshBuffer);

  Part part;
  part.m_uiPrimitiveCount = pMeshBuffer->GetPrimitiveCount();
  part.m_uiFirstPrimitive = 0;  
  part.m_uiMaterialIndex = 0;
  part.m_hMeshBuffer = descriptor.hMeshBuffer;

  m_Parts.PushBack(part);

  m_uiMaterialCount = 1;

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshResource);

