
#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Shader/Shader.h>
#include <RendererWebGPU/RendererWebGPUDLL.h>

class EZ_RENDERERWEBGPU_DLL ezGALShaderWebGPU : public ezGALShader
{
public:
  virtual void SetDebugName(ezStringView sName) const override;

protected:
  friend class ezGALDeviceWebGPU;
  friend class ezMemoryUtils;
  friend class ezGALCommandEncoderImplWebGPU;

  ezGALShaderWebGPU(const ezGALShaderCreationDescription& description);
  virtual ~ezGALShaderWebGPU();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  ezResult CreateShaderModule(ezGALDeviceWebGPU* pDevice, const ezGALShaderByteCode* pByteCode, wgpu::ShaderModule& shaderModule);
  static void WGPUCompilationInfoCallback(WGPUCompilationInfoRequestStatus status, struct WGPUCompilationInfo const* pCompilationInfo, void* userdata);
  void ShaderCompilationInfo(WGPUCompilationInfoRequestStatus status, struct WGPUCompilationInfo const* pCompilationInfo);

  bool m_bLoadedSuccessfully = false;
  const char* m_szCurSource = nullptr;
  wgpu::ShaderModule m_ShaderModuleVS;
  wgpu::ShaderModule m_ShaderModuleFS;
  wgpu::ShaderModule m_ShaderModuleCS;
};

#include <RendererWebGPU/Shader/Implementation/ShaderWebGPU_inl.h>
