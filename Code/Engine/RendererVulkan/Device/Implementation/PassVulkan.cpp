#include <RendererVulkanPCH.h>

#include <RendererVulkan/CommandEncoder/ComputeCommandEncoderVulkan.h>
#include <RendererVulkan/CommandEncoder/RenderCommandEncoderVulkan.h>
#include <RendererVulkan/Device/PassVulkan.h>

ezGALPassVulkan::ezGALPassVulkan(ezGALDevice& device)
  : ezGALPass(device)
{
  m_pRenderCommandEncoder = EZ_DEFAULT_NEW(ezGALRenderCommandEncoderVulkan, device);
  m_pComputeCommandEncoder = EZ_DEFAULT_NEW(ezGALComputeCommandEncoderVulkan, device);
}

ezGALPassVulkan::~ezGALPassVulkan() = default;

ezGALRenderCommandEncoder* ezGALPassVulkan::BeginRenderingPlatform(const ezGALRenderingSetup& renderingSetup, const char* szName)
{
  vk::CommandBuffer& commandBuffer = static_cast<ezGALDeviceVulkan&>(m_Device).GetPrimaryCommandBuffer();

  m_pRenderCommandEncoder->BeginEncode(commandBuffer, renderingSetup);

  return m_pRenderCommandEncoder.Borrow();
}

void ezGALPassVulkan::EndRenderingPlatform(ezGALRenderCommandEncoder* pCommandEncoder)
{
  EZ_ASSERT_DEV(m_pRenderCommandEncoder.Borrow() == pCommandEncoder, "Invalid command encoder");

  m_pRenderCommandEncoder->EndEncode();
}

ezGALComputeCommandEncoder* ezGALPassVulkan::BeginComputePlatform(const char* szName)
{
  vk::CommandBuffer& commandBuffer = static_cast<ezGALDeviceVulkan&>(m_Device).GetPrimaryCommandBuffer();

  m_pComputeCommandEncoder->BeginEncode(commandBuffer);

  return m_pComputeCommandEncoder.Borrow();
}

void ezGALPassVulkan::EndComputePlatform(ezGALComputeCommandEncoder* pCommandEncoder)
{
  EZ_ASSERT_DEV(m_pComputeCommandEncoder.Borrow() == pCommandEncoder, "Invalid command encoder");

  m_pComputeCommandEncoder->EndEncode();
}
