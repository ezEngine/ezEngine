
#pragma once

#include <RendererWebGPU/RendererWebGPUDLL.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/SwapChain.h>

struct IDXGISwapChain;

class ezGALSwapChainWebGPU : public ezGALWindowSwapChain
{
public:
  virtual void AcquireNextRenderTarget(ezGALDevice* pDevice) override;
  virtual void PresentRenderTarget(ezGALDevice* pDevice) override;
  virtual ezResult UpdateSwapChain(ezGALDevice* pDevice, ezEnum<ezGALPresentMode> newPresentMode) override;

protected:
  friend class ezGALDeviceWebGPU;
  friend class ezMemoryUtils;

  ezGALSwapChainWebGPU(const ezGALWindowSwapChainCreationDescription& Description);

  virtual ~ezGALSwapChainWebGPU();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  wgpu::Surface m_Surface;
  wgpu::TextureFormat m_Format;
};

#include <RendererWebGPU/Device/Implementation/SwapChainWebGPU_inl.h>
