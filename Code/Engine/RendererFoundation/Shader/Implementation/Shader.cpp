

#include <RendererFoundation/RendererFoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/ImmutableSamplers.h>
#include <RendererFoundation/Shader/BindGroupLayout.h>
#include <RendererFoundation/Shader/Shader.h>
#include <RendererFoundation/Shader/ShaderUtils.h>
#include <RendererFoundation/Shader/Types.h>

bool ezShaderMat3::TransposeShaderMatrices = false;

ezGALShader::ezGALShader(const ezGALShaderCreationDescription& Description)
  : ezGALObject(Description)
{
}
ezArrayPtr<const ezShaderResourceBinding> ezGALShader::GetBindingMapping() const
{
  return m_BindingMapping;
}

const ezShaderResourceBinding* ezGALShader::GetShaderResourceBinding(const ezTempHashedString& sName) const
{
  for (auto& binding : m_BindingMapping)
  {
    if (binding.m_sName == sName)
    {
      return &binding;
    }
  }
  return nullptr;
}

ezArrayPtr<const ezShaderVertexInputAttribute> ezGALShader::GetVertexInputAttributes() const
{
  if (m_Description.HasByteCodeForStage(ezGALShaderStage::VertexShader))
  {
    return m_Description.m_ByteCodes[ezGALShaderStage::VertexShader]->m_ShaderVertexInput;
  }
  return {};
}

ezResult ezGALShader::CreateBindingMapping(bool bAllowMultipleBindingPerName)
{
  ezHybridArray<ezArrayPtr<const ezShaderResourceBinding>, ezGALShaderStage::ENUM_COUNT> resourceBinding;
  resourceBinding.SetCount(ezGALShaderStage::ENUM_COUNT);
  for (ezUInt32 stage = ezGALShaderStage::VertexShader; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (m_Description.HasByteCodeForStage((ezGALShaderStage::Enum)stage))
    {
      resourceBinding[stage] = m_Description.m_ByteCodes[stage]->m_ShaderResourceBindings;
    }
  }
  return ezShaderResourceBinding::CreateMergedShaderResourceBinding(resourceBinding, m_BindingMapping, bAllowMultipleBindingPerName);
}

void ezGALShader::DestroyBindingMapping()
{
  m_BindingMapping.Clear();
}

ezResult ezGALShader::CreateLayouts(ezGALDevice* pDevice, bool bSupportsImmutableSamplers)
{
  ezGALBindGroupLayoutCreationDescription BindGroupLayoutDesc[EZ_GAL_MAX_SETS];
  ezGALPipelineLayoutCreationDescription PipelineLayoutDesc;

  const ezGALImmutableSamplers::ImmutableSamplers& immutableSamplers = ezGALImmutableSamplers::GetImmutableSamplers();
  for (const ezShaderResourceBinding& binding : m_BindingMapping)
  {
    if (binding.m_ResourceType == ezGALShaderResourceType::PushConstants)
    {
      if (PipelineLayoutDesc.m_PushConstants.m_uiSize != 0)
      {
        ezLog::Error("Only one push constants block is supported per shader.");
        return EZ_FAILURE;
      }
      PipelineLayoutDesc.m_PushConstants.m_Stages = binding.m_Stages;
      PipelineLayoutDesc.m_PushConstants.m_uiOffset = 0;
      PipelineLayoutDesc.m_PushConstants.m_uiSize = (ezUInt16)binding.m_pLayout->m_uiTotalSize;
      continue;
    }

    if (binding.m_iSet >= EZ_GAL_MAX_SETS)
    {
      ezLog::Error("Binding set {} for shader resource '{}' is bigger than EZ_GAL_MAX_SETS.", binding.m_sName.GetData(), binding.m_iSet);
      return EZ_FAILURE;
    }

    if (bSupportsImmutableSamplers && binding.m_ResourceType == ezGALShaderResourceType::Sampler && immutableSamplers.Contains(binding.m_sName))
    {
      BindGroupLayoutDesc[binding.m_iSet].m_ImmutableSamplers.PushBack(binding);
    }
    else
    {
      BindGroupLayoutDesc[binding.m_iSet].m_ResourceBindings.PushBack(binding);
    }
  }

  // There must always be at least one empty set.
  ezUInt32 uiMaxSets = 1;
  for (ezUInt32 uiSetIndex = 1; uiSetIndex < EZ_GAL_MAX_SETS; ++uiSetIndex)
  {
    if (!BindGroupLayoutDesc[uiSetIndex].m_ResourceBindings.IsEmpty())
    {
      uiMaxSets = uiSetIndex + 1;
    }
  }

  m_BindGroupLayouts.SetCount(uiMaxSets);
  for (ezUInt32 uiSetIndex = 0; uiSetIndex < uiMaxSets; ++uiSetIndex)
  {
    m_BindGroupLayouts[uiSetIndex] = pDevice->CreateBindGroupLayout(BindGroupLayoutDesc[uiSetIndex]);
    if (m_BindGroupLayouts[uiSetIndex].IsInvalidated())
    {
      return EZ_FAILURE;
    }
    PipelineLayoutDesc.m_BindGroups[uiSetIndex] = m_BindGroupLayouts[uiSetIndex];
  }
  m_hPipelineLayout = pDevice->CreatePipelineLayout(PipelineLayoutDesc);
  if (m_hPipelineLayout.IsInvalidated())
  {
    return EZ_FAILURE;
  }
  return EZ_SUCCESS;
}

void ezGALShader::DestroyLayouts(ezGALDevice* pDevice)
{
  pDevice->DestroyPipelineLayout(m_hPipelineLayout);
  for (ezUInt32 uiSetIndex = 0; uiSetIndex < m_BindGroupLayouts.GetCount(); ++uiSetIndex)
  {
    pDevice->DestroyBindGroupLayout(m_BindGroupLayouts[uiSetIndex]);
  }
  m_BindGroupLayouts.Clear();
}

ezArrayPtr<const ezShaderResourceBinding> ezGALShader::GetBindings(ezUInt32 uiSet) const
{
  EZ_ASSERT_DEBUG(uiSet < GetSetCount(), "Set index out of range.");
  return m_pDevice->GetBindGroupLayout(m_BindGroupLayouts[uiSet])->GetDescription().m_ResourceBindings;
}

ezGALShader::~ezGALShader() = default;

ezDelegate<void(ezShaderUtils::ezBuiltinShaderType type, ezShaderUtils::ezBuiltinShader& out_shader)> ezShaderUtils::g_RequestBuiltinShaderCallback;
