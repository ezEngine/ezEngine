#pragma once

#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <ShaderCompilerDXC/ShaderCompilerDXC.h>
#include <ShaderCompilerWebGPU/ShaderCompilerWebGPUDLL.h>

class EZ_SHADERCOMPILERWEBGPU_DLL ezShaderCompilerWebGPU : public ezShaderCompilerDXC
{
  EZ_ADD_DYNAMIC_REFLECTION(ezShaderCompilerWebGPU, ezShaderCompilerDXC);

public:
  virtual void GetSupportedPlatforms(ezHybridArray<ezString, 4>& out_platforms) override;

  virtual ezResult Compile(ezShaderProgramData& inout_data, ezLogInterface* pLog) override;

  virtual void ConfigureDxcArgs(ezDynamicArray<ezStringWChar>& inout_Args) override;
  virtual bool AllowCombinedImageSamplers() const override { return false; }
};
