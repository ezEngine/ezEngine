#include <RendererVulkan/RendererVulkanPCH.h>

EZ_STATICLINK_LIBRARY(RendererVulkan)
{
  if (bReturn)
    return;

  EZ_STATICLINK_REFERENCE(RendererVulkan_Device_Implementation_DeviceVulkan);
  EZ_STATICLINK_REFERENCE(RendererVulkan_Resources_Implementation_FallbackResourcesVulkan);
}
