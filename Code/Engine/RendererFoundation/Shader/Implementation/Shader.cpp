#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Shader/Shader.h>
#include <RendererFoundation/Shader/ShaderUtils.h>

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

ezGALShader::~ezGALShader() = default;

ezDelegate<void(ezShaderUtils::ezBuiltinShaderType type, ezShaderUtils::ezBuiltinShader& out_shader)> ezShaderUtils::g_RequestBuiltinShaderCallback;


