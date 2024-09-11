#pragma once

#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <ShaderCompilerDXC/ShaderCompilerDXCDLL.h>

struct SpvReflectDescriptorBinding;
struct SpvReflectBlockVariable;

class EZ_SHADERCOMPILERDXC_DLL ezShaderCompilerDXC : public ezShaderProgramCompiler
{
  EZ_ADD_DYNAMIC_REFLECTION(ezShaderCompilerDXC, ezShaderProgramCompiler);

public:
  virtual void GetSupportedPlatforms(ezHybridArray<ezString, 4>& out_platforms) override { out_platforms.PushBack("VULKAN"); }

  virtual ezResult ModifyShaderSource(ezShaderProgramData& inout_data, ezLogInterface* pLog) override;
  virtual ezResult Compile(ezShaderProgramData& inout_Data, ezLogInterface* pLog) override;

private:
  /// \brief Sets fixed set / slot bindings to each resource.
  /// The end result will have these properties:
  /// 1. Every binding name has a unique set / slot.
  /// 2. Bindings that already had a fixed set or slot (e.g. != -1) should not have these changed.
  /// 2. Set / slots can only be the same for two bindings if they have been changed to ezGALShaderResourceType::TextureAndSampler.
  ezResult DefineShaderResourceBindings(const ezShaderProgramData& data, ezHashTable<ezHashedString, ezShaderResourceBinding>& inout_resourceBinding, ezLogInterface* pLog);

  void CreateNewShaderResourceDeclaration(ezStringView sPlatform, ezStringView sDeclaration, const ezShaderResourceBinding& binding, ezStringBuilder& out_sDeclaration);

  ezResult ReflectShaderStage(ezShaderProgramData& inout_Data, ezGALShaderStage::Enum Stage);
  ezShaderConstantBufferLayout* ReflectConstantBufferLayout(const char* szName, const SpvReflectBlockVariable& block);
  ezResult FillResourceBinding(ezShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info);
  ezResult FillSRVResourceBinding(ezShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info);
  ezResult FillUAVResourceBinding(ezShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info);
  static ezGALShaderTextureType::Enum GetTextureType(const SpvReflectDescriptorBinding& info);
  ezResult Initialize();

private:
  ezMap<const char*, ezGALVertexAttributeSemantic::Enum, CompareConstChar> m_VertexInputMapping;
};
