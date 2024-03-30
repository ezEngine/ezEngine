
#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererVulkan/RendererVulkanDLL.h>

#include <vulkan/vulkan.hpp>

class ezGALDeviceVulkan;

class ezGALSwapChainVulkan : public ezGALWindowSwapChain
{
public:
  virtual void AcquireNextRenderTarget(ezGALDevice* pDevice) override;
  virtual void PresentRenderTarget(ezGALDevice* pDevice) override;
  virtual ezResult UpdateSwapChain(ezGALDevice* pDevice, ezEnum<ezGALPresentMode> newPresentMode) override;

  EZ_ALWAYS_INLINE vk::SwapchainKHR GetVulkanSwapChain() const;

protected:
  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALSwapChainVulkan(const ezGALWindowSwapChainCreationDescription& Description);

  virtual ~ezGALSwapChainVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  ezResult CreateSwapChainInternal();
  void DestroySwapChainInternal(ezGALDeviceVulkan* pVulkanDevice);
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

protected:
  ezGALDeviceVulkan* m_pVulkanDevice = nullptr;
  ezEnum<ezGALPresentMode> m_currentPresentMode;

  vk::SurfaceKHR m_vulkanSurface;
  vk::SwapchainKHR m_vulkanSwapChain;
  ezHybridArray<vk::Image, 3> m_swapChainImages;
  ezHybridArray<ezGALTextureHandle, 3> m_swapChainTextures;
  ezUInt32 m_uiCurrentSwapChainImage = 0;

  vk::Semaphore m_currentPipelineImageAvailableSemaphore;
};

#include <RendererVulkan/Device/Implementation/SwapChainVulkan_inl.h>
