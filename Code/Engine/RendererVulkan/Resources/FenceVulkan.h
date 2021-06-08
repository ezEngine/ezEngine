
#pragma once

#include <RendererFoundation/Resources/Fence.h>

#include <vulkan/vulkan.hpp>

class EZ_RENDERERVULKAN_DLL ezGALFenceVulkan : public ezGALFence
{
public:
  EZ_ALWAYS_INLINE vk::Fence GetFence() const
  {
    return m_fence;
  }

protected:
  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALFenceVulkan();

  virtual ~ezGALFenceVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  vk::Fence m_fence;
};
