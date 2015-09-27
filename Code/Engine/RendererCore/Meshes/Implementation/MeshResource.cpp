#include <RendererCore/PCH.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Material/MaterialResource.h>
#include <Foundation/Strings/StringBuilder.h>
#include <CoreUtils/Assets/AssetFileHeader.h>

#include <Core/ResourceManager/ResourceManager.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshResource, ezResourceBase, 1, ezRTTIDefaultAllocator<ezMeshResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezUInt32 ezMeshResource::s_MeshBufferNameSuffix = 0;

ezMeshResource::ezMeshResource() : ezResource<ezMeshResource, ezMeshResourceDescriptor>(DoUpdate::OnAnyThread, 1)
{
}

ezResourceLoadDesc ezMeshResource::UnloadData(Unload WhatToUnload)
{
  ezResourceLoadDesc res;
  res.m_State = GetLoadingState();
  res.m_uiQualityLevelsDiscardable = GetNumQualityLevelsDiscardable();
  res.m_uiQualityLevelsLoadable = GetNumQualityLevelsLoadable();

  // we currently can only unload the entire mesh
  //if (WhatToUnload == Unload::AllQualityLevels)
  {
    m_SubMeshes.Clear();
    m_hMeshBuffer.Invalidate();
    m_Materials.Clear();

    res.m_uiQualityLevelsDiscardable = 0;
    res.m_uiQualityLevelsLoadable = 0;
    res.m_State = ezResourceState::Unloaded;
  }

  return res;
}

ezResourceLoadDesc ezMeshResource::UpdateContent(ezStreamReaderBase* Stream)
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

  if (desc.Load(*Stream).Failed())
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  return CreateResource(desc);
}

void ezMeshResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezMeshResource) + (ezUInt32) m_SubMeshes.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

ezResourceLoadDesc ezMeshResource::CreateResource(const ezMeshResourceDescriptor& desc)
{
  // if there is an existing mesh buffer to use, take that
  m_hMeshBuffer = desc.GetExistingMeshBuffer();

  // otherwise create a new mesh buffer from the descriptor
  if (!m_hMeshBuffer.IsValid())
  {
    s_MeshBufferNameSuffix++;
    ezStringBuilder sMbName;
    sMbName.Format("%s  [MeshBuffer %04X]", GetResourceID().GetData(), s_MeshBufferNameSuffix);

    m_hMeshBuffer = ezResourceManager::CreateResource<ezMeshBufferResource>(sMbName, desc.MeshBufferDesc(), GetResourceDescription());
  }

  m_SubMeshes = desc.GetSubMeshes();

  m_Materials.Clear();
  m_Materials.Reserve(desc.GetMaterials().GetCount());

  // copy all the material assignments and load the materials
  for (const auto& mat : desc.GetMaterials())
  {
    ezMaterialResourceHandle hMat;

    if (!mat.m_sPath.IsEmpty())
      hMat = ezResourceManager::LoadResource<ezMaterialResource>(mat.m_sPath);

    m_Materials.PushBack(hMat); // may be an invalid handle
  }

  m_Bounds = desc.GetBounds();

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshResource);

