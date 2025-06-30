#pragma once

#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <ShaderCompilerHLSL/ShaderCompilerHLSLDLL.h>

struct ID3D11ShaderReflectionConstantBuffer;
struct _D3D11_SIGNATURE_PARAMETER_DESC;

class EZ_SHADERCOMPILERHLSL_DLL ezShaderCompilerHLSL : public ezShaderProgramCompiler
{
  EZ_ADD_DYNAMIC_REFLECTION(ezShaderCompilerHLSL, ezShaderProgramCompiler);

public:
  virtual void GetSupportedPlatforms(ezHybridArray<ezString, 4>& out_platforms) override
  {
    out_platforms.PushBack("DX11_SM40_93");
    out_platforms.PushBack("DX11_SM40");
    out_platforms.PushBack("DX11_SM41");
    out_platforms.PushBack("DX11_SM50");
  }

  virtual ezEnum<ezGALBufferLayout> GetMaterialBufferLayout(ezStringView sPlatform) const override
  {
    return ezGALBufferLayout::DirectX_ConstantButter;
  }

  virtual ezResult ModifyShaderSource(ezShaderProgramData& inout_data, ezLogInterface* pLog) override;
  virtual ezResult Compile(ezShaderProgramData& inout_data, ezLogInterface* pLog) override;

private:
  ezResult DefineShaderResourceBindings(const ezShaderProgramData& data, ezHashTable<ezHashedString, ezShaderResourceBinding>& inout_resourceBinding, ezLogInterface* pLog);

  void CreateNewShaderResourceDeclaration(ezStringView sPlatform, ezStringView sDeclaration, const ezShaderResourceBinding& binding, ezStringBuilder& out_sDeclaration);

  void ReflectShaderStage(ezShaderProgramData& inout_Data, ezGALShaderStage::Enum Stage);
  ezSharedPtr<ezShaderConstantBufferLayout> ReflectConstantBufferLayout(ezGALShaderByteCode& pStageBinary, ID3D11ShaderReflectionConstantBuffer* pConstantBufferReflection);
  ezResult AddFakeBindGroupAssignments(ezShaderProgramData& inout_Data, ezGALShaderStage::Enum Stage, ezLogInterface* pLog);
  void Initialize();
  static ezGALResourceFormat::Enum GetEZFormat(const _D3D11_SIGNATURE_PARAMETER_DESC& paramDesc);

private:
  ezMap<const char*, ezGALVertexAttributeSemantic::Enum, CompareConstChar> m_VertexInputMapping;
};
