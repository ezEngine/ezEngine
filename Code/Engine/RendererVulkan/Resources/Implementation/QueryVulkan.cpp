#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Resources/QueryVulkan.h>

#include <d3d11.h>

ezGALQueryVulkan::ezGALQueryVulkan(const ezGALQueryCreationDescription& Description)
  : ezGALQuery(Description)
{
}

ezGALQueryVulkan::~ezGALQueryVulkan() {}

ezResult ezGALQueryVulkan::InitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);

  if (true)
  {
    return EZ_SUCCESS;
  }
  else
  {
    ezLog::Error("Creation of native Vulkan query failed!");
    return EZ_FAILURE;
  }
}

ezResult ezGALQueryVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  // TODO
  return EZ_SUCCESS;
}

void ezGALQueryVulkan::SetDebugNamePlatform(const char* szName) const
{
  ezUInt32 uiLength = ezStringUtils::GetStringElementCount(szName);

  // TODO
}

EZ_STATICLINK_FILE(RendererVulkan, RendererVulkan_Resources_Implementation_QueryVulkan);
