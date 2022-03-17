
#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererVulkan/RendererVulkanDLL.h>

#include <vulkan/vulkan.hpp>

class ezGALSwapChainVulkan : public ezGALSwapChain
{
public:
  EZ_ALWAYS_INLINE vk::SwapchainKHR GetVulkanSwapChain() const;
  void AcquireNextImage(vk::Semaphore imageAvailable) const;
  void PresentMemoryBarrier(vk::CommandBuffer commandBuffer) const;
  void PresentNextImage(vk::Semaphore renderFinished, vk::Fence renderFinishedFence) const;

protected:
  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALSwapChainVulkan(const ezGALSwapChainCreationDescription& Description);

  virtual ~ezGALSwapChainVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  mutable ezGALDeviceVulkan* m_pVulkanDevice = nullptr;

  mutable vk::SurfaceKHR m_vulkanSurface;
  mutable vk::SwapchainKHR m_vulkanSwapChain;
  mutable ezHybridArray<vk::Image, 3> m_swapChainImages;
  mutable ezHybridArray<vk::Fence, 3> m_swapChainImageInUseFences;
  mutable ezUInt32 m_uiCurrentSwapChainImage = 0;

  // We can't do screenshots if we're using any of the FLIP swap effects.
  // If the user requests screenshots anyways, we need to put another buffer in between.
  // For ease of use, this is m_hBackBufferTexture and the actual "OS backbuffer" is this texture.
  // In any other case this handle is unused.
  ezGALTextureHandle m_hActualBackBufferTexture;
};

#include <RendererVulkan/Device/Implementation/SwapChainVulkan_inl.h>
