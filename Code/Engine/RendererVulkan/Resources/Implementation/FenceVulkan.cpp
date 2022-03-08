#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/RendererVulkanDLL.h>
#include <RendererVulkan/Resources/FenceVulkan.h>

#include <d3d11.h>

ezGALFenceVulkan::ezGALFenceVulkan()
  : m_fence(nullptr)
{
}

ezGALFenceVulkan::~ezGALFenceVulkan() {}

ezResult ezGALFenceVulkan::InitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);

  vk::FenceCreateInfo createInfo = {};
  m_fence = pVulkanDevice->GetVulkanDevice().createFence(createInfo);

  if (m_fence)
  {
    return EZ_SUCCESS;
  }
  else
  {
    ezLog::Error("Creation of native Vulkan fence failed!");
    return EZ_FAILURE;
  }
}

ezResult ezGALFenceVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  if (m_fence)
  {
    ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);
    pVulkanDevice->GetVulkanDevice().destroyFence(m_fence);
    m_fence = nullptr;
  }

  return EZ_SUCCESS;
}

EZ_STATICLINK_FILE(RendererVulkan, RendererVulkan_Resources_Implementation_FenceVulkan);
