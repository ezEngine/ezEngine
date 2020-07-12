#include <RendererVulkanPCH.h>

#include <RendererVulkan/RendererVulkanDLL.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Resources/FenceVulkan.h>

#include <d3d11.h>

ezGALFenceVulkan::ezGALFenceVulkan()
    : m_pDXFence(nullptr)
{
}

ezGALFenceVulkan::~ezGALFenceVulkan() {}


ezResult ezGALFenceVulkan::InitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceVulkan* pDXDevice = static_cast<ezGALDeviceVulkan*>(pDevice);

  D3D11_QUERY_DESC QueryDesc;
  QueryDesc.Query = D3D11_QUERY_EVENT;
  QueryDesc.MiscFlags = 0;

  if (SUCCEEDED(pDXDevice->GetDXDevice()->CreateQuery(&QueryDesc, &m_pDXFence)))
  {
    return EZ_SUCCESS;
  }
  else
  {
    ezLog::Error("Creation of native DirectX fence failed!");
    return EZ_FAILURE;
  }
}

ezResult ezGALFenceVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  EZ_GAL_Vulkan_RELEASE(m_pDXFence);

  return EZ_SUCCESS;
}



EZ_STATICLINK_FILE(RendererVulkan, RendererVulkan_Resources_Implementation_FenceVulkan);
