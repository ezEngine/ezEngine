#include <RendererCore/PCH.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/ShaderCompiler/ShaderParser.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezShaderResource, 1, ezRTTIDefaultAllocator<ezShaderResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE

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

ezResourceLoadDesc ezShaderResource::UpdateContent(ezStreamReader* stream)
{
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  m_bShaderResourceIsValid = false;

  if (stream == nullptr)
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    ezString sAbsFilePath;
    (*stream) >> sAbsFilePath;
  }

  ezShaderParser::ParsePermutationSection(*stream, m_PermutationVarsUsed);

  res.m_State = ezResourceState::Loaded;
  m_bShaderResourceIsValid = true;

  return res;
}

void ezShaderResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezShaderResource) + (ezUInt32) m_PermutationVarsUsed.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}



ezResourceLoadDesc ezShaderResource::CreateResource(const ezShaderResourceDescriptor& descriptor)
{
  ezResourceLoadDesc ret;
  ret.m_State = ezResourceState::Loaded;
  ret.m_uiQualityLevelsDiscardable = 0;
  ret.m_uiQualityLevelsLoadable = 0;

  m_bShaderResourceIsValid = false;

  return ret;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Shader_ShaderResource);

