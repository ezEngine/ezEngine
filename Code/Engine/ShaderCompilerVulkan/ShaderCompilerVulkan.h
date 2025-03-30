#pragma once

#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <ShaderCompilerDXC/ShaderCompilerDXC.h>
#include <ShaderCompilerVulkan/ShaderCompilerVulkanDLL.h>

class EZ_SHADERCOMPILERVULKAN_DLL ezShaderCompilerVulkan : public ezShaderCompilerDXC
{
  EZ_ADD_DYNAMIC_REFLECTION(ezShaderCompilerVulkan, ezShaderCompilerDXC);

public:
  virtual void GetSupportedPlatforms(ezHybridArray<ezString, 4>& out_platforms) override
  {
    out_platforms.PushBack("VULKAN");
  }

  virtual ezEnum<ezGALBufferLayout> GetMaterialBufferLayout(ezStringView sPlatform) const override
  {
    return ezGALBufferLayout::Vulkan_Std430_relaxed;
  }
};
