#include <RendererCore/PCH.h>
#include <RendererCore/Pipeline/RenderPipelineResource.h>
#include <RendererCore/Pipeline/Implementation/RenderPipelineResourceLoader.h>
#include <Foundation/IO/ExtendedJSONReader.h>
#include <CoreUtils/Assets/AssetFileHeader.h>

void ezRenderPipelineResourceDescriptor::CreateFromRenderPipeline(const ezRenderPipeline* pPipeline)
{
  ezRenderPipelineResourceLoader::CreateRenderPipelineResourceDescriptor(pPipeline, *this);
}

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderPipelineResource, 1, ezRTTIDefaultAllocator<ezRenderPipelineResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezRenderPipelineResource::ezRenderPipelineResource() : ezResource<ezRenderPipelineResource, ezRenderPipelineResourceDescriptor>(DoUpdate::OnAnyThread, 1)
{
}

ezRenderPipeline* ezRenderPipelineResource::CreateRenderPipeline() const
{
  if (GetLoadingState() != ezResourceState::Loaded)
  {
    ezLog::Error("Can't create render pipeline '%s', the resource is not loaded!", GetResourceID().GetData());
    return nullptr;
  }

  return ezRenderPipelineResourceLoader::CreateRenderPipeline(m_Desc);
}

ezResourceLoadDesc ezRenderPipelineResource::UnloadData(Unload WhatToUnload)
{
  m_Desc.Clear();

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  return res;
}

ezResourceLoadDesc ezRenderPipelineResource::UpdateContent(ezStreamReader* Stream)
{
  m_Desc.Clear();

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  if (Stream == nullptr)
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  ezStringBuilder sAbsFilePath;
  (*Stream) >> sAbsFilePath;

  if (sAbsFilePath.HasExtension("ezRenderPipelineBin"))
  {
    ezStringBuilder sTemp, sTemp2;

    ezAssetFileHeader AssetHash;
    AssetHash.Read(*Stream);

    ezUInt8 uiVersion = 0;
    (*Stream) >> uiVersion;
    EZ_ASSERT_DEV(uiVersion == 1, "Unknown ezRenderPipelineBin version %u", uiVersion);

    ezUInt32 uiSize = 0;
    (*Stream) >> uiSize;

    m_Desc.m_SerializedPipeline.SetCount(uiSize);
    ezMemoryStreamStorage storage;
    Stream->ReadBytes(m_Desc.m_SerializedPipeline.GetData(), uiSize);

    EZ_ASSERT_DEV(uiSize > 0, "RenderPipeline resourse contains no pipeline data!");
  }
  else
  {
    EZ_REPORT_FAILURE("The file '%s' is unsupported, only '.ezRenderPipelineBin' files can be loaded as ezRenderPipelineResource", sAbsFilePath.GetData());
  }

  return res;
}

void ezRenderPipelineResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezRenderPipelineResource) +
                                     (ezUInt32) (m_Desc.m_SerializedPipeline.GetCount());

  out_NewMemoryUsage.m_uiMemoryGPU = 0;

}

ezResourceLoadDesc ezRenderPipelineResource::CreateResource(const ezRenderPipelineResourceDescriptor& descriptor)
{
  m_Desc = descriptor;

  ezResourceLoadDesc res;
  res.m_State = ezResourceState::Loaded;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  return res;
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderPipelineResource);

