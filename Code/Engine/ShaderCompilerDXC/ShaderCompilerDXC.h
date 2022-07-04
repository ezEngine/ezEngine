#pragma once

#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <ShaderCompilerDXC/ShaderCompilerDXCDLL.h>

struct SpvReflectDescriptorBinding;

class EZ_SHADERCOMPILERDXC_DLL ezShaderCompilerDXC : public ezShaderProgramCompiler
{
  EZ_ADD_DYNAMIC_REFLECTION(ezShaderCompilerDXC, ezShaderProgramCompiler);

public:
  virtual void GetSupportedPlatforms(ezHybridArray<ezString, 4>& Platforms) override { Platforms.PushBack("VULKAN"); }

  virtual ezResult Compile(ezShaderProgramData& inout_Data, ezLogInterface* pLog) override;

private:
  ezResult ReflectShaderStage(ezShaderProgramData& inout_Data, ezGALShaderStage::Enum Stage);
  ezShaderConstantBufferLayout* ReflectConstantBufferLayout(ezShaderStageBinary& pStageBinary, const SpvReflectDescriptorBinding& pConstantBufferReflection);
  ezResult FillResourceBinding(ezShaderStageBinary& shaderBinary, ezShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info);
  ezResult FillSRVResourceBinding(ezShaderStageBinary& shaderBinary, ezShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info);
  ezResult FillUAVResourceBinding(ezShaderStageBinary& shaderBinary, ezShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info);

  ezResult Initialize();

private:
  ezMap<const char*, ezGALVertexAttributeSemantic::Enum, CompareConstChar> m_VertexInputMapping;
};
