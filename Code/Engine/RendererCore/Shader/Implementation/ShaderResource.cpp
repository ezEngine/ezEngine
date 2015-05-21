#include <RendererCore/PCH.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Shader/Implementation/Helper.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <Foundation/Logging/Log.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezShaderResource, ezResourceBase, 1, ezRTTIDefaultAllocator<ezShaderResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezShaderResource::ezShaderResource() : ezResource<ezShaderResource, ezShaderResourceDescriptor>(DoUpdate::OnAnyThread, 1)
{
  m_bShaderResourceIsValid = false;
}

ezResourceLoadDesc ezShaderResource::UnloadData(Unload WhatToUnload)
{
  m_bShaderResourceIsValid = false;
  m_PermutationVarsUsed.Clear();
  
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  return res;
}

ezResourceLoadDesc ezShaderResource::UpdateContent(ezStreamReaderBase* Stream)
{
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  m_bShaderResourceIsValid = false;

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

  ezString sContent;
  sContent.ReadAll(*Stream);

  ezTextSectionizer Sections;
  GetShaderSections(sContent.GetData(), Sections);

  ezUInt32 uiFirstLine = 0;
  m_PermutationVarsUsed = Sections.GetSectionContent(ezShaderSections::PERMUTATIONS, uiFirstLine);

  res.m_State = ezResourceState::Loaded;
  m_bShaderResourceIsValid = true;

  return res;
}

void ezShaderResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezShaderResource) + (ezUInt32) m_PermutationVarsUsed.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Shader_ShaderResource);

