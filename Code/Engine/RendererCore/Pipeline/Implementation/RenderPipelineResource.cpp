#include <PCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <RendererCore/Pipeline/Implementation/RenderPipelineResourceLoader.h>
#include <RendererCore/Pipeline/Passes/SimpleRenderPass.h>
#include <RendererCore/Pipeline/Passes/SourcePass.h>
#include <RendererCore/Pipeline/Passes/TargetPass.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/RenderPipelineResource.h>

void ezRenderPipelineResourceDescriptor::CreateFromRenderPipeline(const ezRenderPipeline* pPipeline)
{
  ezRenderPipelineResourceLoader::CreateRenderPipelineResourceDescriptor(pPipeline, *this);
}

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderPipelineResource, 1, ezRTTIDefaultAllocator<ezRenderPipelineResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezRenderPipelineResource::ezRenderPipelineResource()
    : ezResource<ezRenderPipelineResource, ezRenderPipelineResourceDescriptor>(DoUpdate::OnAnyThread, 1)
{
}

ezInternal::NewInstance<ezRenderPipeline> ezRenderPipelineResource::CreateRenderPipeline() const
{
  if (GetLoadingState() != ezResourceState::Loaded)
  {
    ezLog::Error("Can't create render pipeline '{0}', the resource is not loaded!", GetResourceID());
    return ezInternal::NewInstance<ezRenderPipeline>(nullptr, nullptr);
  }

  return ezRenderPipelineResourceLoader::CreateRenderPipeline(m_Desc);
}

// static
ezRenderPipelineResourceHandle ezRenderPipelineResource::CreateMissingPipeline()
{
  ezUniquePtr<ezRenderPipeline> pRenderPipeline = EZ_DEFAULT_NEW(ezRenderPipeline);

  ezSourcePass* pColorSourcePass = nullptr;
  {
    ezUniquePtr<ezSourcePass> pPass = EZ_DEFAULT_NEW(ezSourcePass, "ColorSource");
    pColorSourcePass = pPass.Borrow();
    pRenderPipeline->AddPass(std::move(pPass));
  }

  ezSimpleRenderPass* pSimplePass = nullptr;
  {
    ezUniquePtr<ezSimpleRenderPass> pPass = EZ_DEFAULT_NEW(ezSimpleRenderPass);
    pSimplePass = pPass.Borrow();
    pSimplePass->SetMessage("Render pipeline resource is missing. Ensure that the corresponding asset has been transformed.");
    pRenderPipeline->AddPass(std::move(pPass));
  }

  ezTargetPass* pTargetPass = nullptr;
  {
    ezUniquePtr<ezTargetPass> pPass = EZ_DEFAULT_NEW(ezTargetPass);
    pTargetPass = pPass.Borrow();
    pRenderPipeline->AddPass(std::move(pPass));
  }

  EZ_VERIFY(pRenderPipeline->Connect(pColorSourcePass, "Output", pSimplePass, "Color"), "Connect failed!");
  EZ_VERIFY(pRenderPipeline->Connect(pSimplePass, "Color", pTargetPass, "Color0"), "Connect failed!");

  ezRenderPipelineResourceDescriptor desc;
  ezRenderPipelineResourceLoader::CreateRenderPipelineResourceDescriptor(pRenderPipeline.Borrow(), desc);

  return ezResourceManager::CreateResource<ezRenderPipelineResource>("MissingRenderPipeline", desc, "MissingRenderPipeline");
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
    EZ_ASSERT_DEV(uiVersion == 1, "Unknown ezRenderPipelineBin version {0}", uiVersion);

    ezUInt32 uiSize = 0;
    (*Stream) >> uiSize;

    m_Desc.m_SerializedPipeline.SetCountUninitialized(uiSize);
    Stream->ReadBytes(m_Desc.m_SerializedPipeline.GetData(), uiSize);

    EZ_ASSERT_DEV(uiSize > 0, "RenderPipeline resourse contains no pipeline data!");
  }
  else
  {
    EZ_REPORT_FAILURE("The file '{0}' is unsupported, only '.ezRenderPipelineBin' files can be loaded as ezRenderPipelineResource",
                      sAbsFilePath);
  }

  return res;
}

void ezRenderPipelineResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezRenderPipelineResource) + (ezUInt32)(m_Desc.m_SerializedPipeline.GetCount());

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
