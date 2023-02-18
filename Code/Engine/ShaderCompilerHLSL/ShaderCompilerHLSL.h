#pragma once

#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <ShaderCompilerHLSL/ShaderCompilerHLSLDLL.h>

struct ID3D11ShaderReflectionConstantBuffer;

class EZ_SHADERCOMPILERHLSL_DLL ezShaderCompilerHLSL : public ezShaderProgramCompiler
{
  EZ_ADD_DYNAMIC_REFLECTION(ezShaderCompilerHLSL, ezShaderProgramCompiler);

public:
  virtual void GetSupportedPlatforms(ezHybridArray<ezString, 4>& ref_platforms) override
  {
    ref_platforms.PushBack("DX11_SM40_93");
    ref_platforms.PushBack("DX11_SM40");
    ref_platforms.PushBack("DX11_SM41");
    ref_platforms.PushBack("DX11_SM50");
  }

  virtual ezResult Compile(ezShaderProgramData& inout_data, ezLogInterface* pLog) override;

private:
  void ReflectShaderStage(ezShaderProgramData& inout_Data, ezGALShaderStage::Enum Stage);
  ezShaderConstantBufferLayout* ReflectConstantBufferLayout(ezShaderStageBinary& pStageBinary, ID3D11ShaderReflectionConstantBuffer* pConstantBufferReflection);
};
