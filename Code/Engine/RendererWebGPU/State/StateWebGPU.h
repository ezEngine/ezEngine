
#pragma once

#include <RendererFoundation/State/State.h>

#include <webgpu/webgpu_cpp.h>

class EZ_RENDERERWEBGPU_DLL ezGALBlendStateWebGPU : public ezGALBlendState
{
protected:
  friend class ezGALDeviceWebGPU;
  friend class ezMemoryUtils;

  ezGALBlendStateWebGPU(const ezGALBlendStateCreationDescription& Description);
  ~ezGALBlendStateWebGPU();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  wgpu::BlendState m_States[EZ_GAL_MAX_RENDERTARGET_COUNT];
};

class EZ_RENDERERWEBGPU_DLL ezGALDepthStencilStateWebGPU : public ezGALDepthStencilState
{
protected:
  friend class ezGALDeviceWebGPU;
  friend class ezMemoryUtils;

  ezGALDepthStencilStateWebGPU(const ezGALDepthStencilStateCreationDescription& Description);
  ~ezGALDepthStencilStateWebGPU();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  wgpu::DepthStencilState m_State;
};

class EZ_RENDERERWEBGPU_DLL ezGALRasterizerStateWebGPU : public ezGALRasterizerState
{
protected:
  friend class ezGALDeviceWebGPU;
  friend class ezMemoryUtils;

  ezGALRasterizerStateWebGPU(const ezGALRasterizerStateCreationDescription& Description);
  ~ezGALRasterizerStateWebGPU();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;
};

class EZ_RENDERERWEBGPU_DLL ezGALSamplerStateWebGPU : public ezGALSamplerState
{
protected:
  friend class ezGALDeviceWebGPU;
  friend class ezMemoryUtils;
  friend class ezGALCommandEncoderImplWebGPU;

  ezGALSamplerStateWebGPU(const ezGALSamplerStateCreationDescription& Description);
  ~ezGALSamplerStateWebGPU();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  wgpu::Sampler m_Sampler;
};


#include <RendererWebGPU/State/Implementation/StateWebGPU_inl.h>
