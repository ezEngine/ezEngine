#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/ShaderCompiler/ShaderParser.h>
#include <RendererFoundation/Device/Device.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezShaderResource, 1, ezRTTIDefaultAllocator<ezShaderResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezShaderResource);
// clang-format on

ezShaderResource::ezShaderResource()
  : ezResource(DoUpdate::OnAnyThread, 1)
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

  // the standard file reader writes the absolute file path into the stream
  ezStringBuilder sAbsFilePath;
  (*stream) >> sAbsFilePath;

  ezString sContent;
  sContent.ReadAll(*stream);

  ezShaderHelper::ezTextSectionizer Sections;
  ezShaderHelper::GetShaderSections(sContent.GetData(), Sections);

  ezUInt32 uiFirstLine = 0;
  ezHybridArray<ezPermutationVar, 16> fixedPermVars; // ignored here
  ezStringView sPermutations = Sections.GetSectionContent(ezShaderHelper::ezShaderSections::PERMUTATIONS, uiFirstLine);
  ezShaderParser::ParsePermutationSection(sPermutations, m_PermutationVarsUsed, fixedPermVars);

  uiFirstLine = 0;
  ezStringView sShader = Sections.GetSectionContent(ezShaderHelper::ezShaderSections::MATERIALCONSTANTS, uiFirstLine);
  if (!sShader.IsEmpty())
  {
    if (ezShaderParser::ParseMaterialConstantsSection(sShader, m_pLayout).Succeeded() && m_pLayout != nullptr)
    {
      ezShaderParser::LayoutMaterialConstants(*m_pLayout, ezGALDevice::GetDefaultDevice()->GetCapabilities().m_materialBufferLayout);
    }
  }

  res.m_State = ezResourceState::Loaded;
  m_bShaderResourceIsValid = true;

  return res;
}

void ezShaderResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezShaderResource) + (ezUInt32)m_PermutationVarsUsed.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

EZ_RESOURCE_IMPLEMENT_CREATEABLE(ezShaderResource, ezShaderResourceDescriptor)
{
  ezResourceLoadDesc ret;
  ret.m_State = ezResourceState::Loaded;
  ret.m_uiQualityLevelsDiscardable = 0;
  ret.m_uiQualityLevelsLoadable = 0;

  m_bShaderResourceIsValid = false;

  return ret;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Shader_Implementation_ShaderResource);
