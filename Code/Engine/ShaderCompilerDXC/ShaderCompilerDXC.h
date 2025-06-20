#pragma once

#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <ShaderCompilerDXC/ShaderCompilerDXCDLL.h>

struct SpvReflectDescriptorBinding;
struct SpvReflectBlockVariable;

class EZ_SHADERCOMPILERDXC_DLL ezShaderCompilerDXC : public ezShaderProgramCompiler
{
  EZ_ADD_DYNAMIC_REFLECTION(ezShaderCompilerDXC, ezShaderProgramCompiler);

public:
  virtual ezResult ModifyShaderSource(ezShaderProgramData& inout_data, ezLogInterface* pLog) override;
  virtual ezResult Compile(ezShaderProgramData& inout_Data, ezLogInterface* pLog) override;

protected:
  virtual void ConfigureDxcArgs(ezDynamicArray<ezStringWChar>& inout_Args);
  virtual bool AllowCombinedImageSamplers() const { return true; }

private:
  /// \brief Sets fixed set / slot bindings to each resource.
  /// The end result will have these properties:
  /// 1. Every binding name has a unique set / slot.
  /// 2. Bindings that already had a fixed set or slot (e.g. != -1) should not have these changed.
  /// 2. Set / slots can only be the same for two bindings if they have been changed to ezGALShaderResourceType::TextureAndSampler.
  ezResult DefineShaderResourceBindings(const ezShaderProgramData& data, ezHashTable<ezHashedString, ezShaderResourceBinding>& inout_resourceBinding, ezLogInterface* pLog);

  void CreateNewShaderResourceDeclaration(ezStringView sPlatform, ezStringView sDeclaration, const ezShaderResourceBinding& binding, ezStringBuilder& out_sDeclaration);

  ezResult ReflectShaderStage(ezShaderProgramData& inout_Data, ezGALShaderStage::Enum Stage);
  ezSharedPtr<ezShaderConstantBufferLayout> ReflectStructuredBufferLayout(ezStringView sName, const SpvReflectBlockVariable& block);
  ezSharedPtr<ezShaderConstantBufferLayout> ReflectConstantBufferLayout(ezStringView sName, const SpvReflectBlockVariable& block);
  ezSharedPtr<ezShaderConstantBufferLayout> ReflectBufferLayout(const SpvReflectBlockVariable& block);
  ezResult FillResourceBinding(ezShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info);
  ezResult FillSRVResourceBinding(ezShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info);
  ezResult FillUAVResourceBinding(ezShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info);
  static ezGALShaderTextureType::Enum GetTextureType(const SpvReflectDescriptorBinding& info);
  ezResult Initialize();
  ezResult CompileSPIRVShader(ezStringView sFile, ezStringView sSource, bool bDebug, ezStringView sProfile, ezStringView sEntryPoint, ezDynamicArray<ezUInt8>& out_ByteCode);
  ezStringView GetProfileName(ezStringView sPlatform, ezGALShaderStage::Enum Stage);

private:
  ezMap<const char*, ezGALVertexAttributeSemantic::Enum, CompareConstChar> m_VertexInputMapping;
};
