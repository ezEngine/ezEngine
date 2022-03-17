#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererFoundation/CommandEncoder/CommandEncoderState.h>
#include <RendererFoundation/CommandEncoder/ComputeCommandEncoder.h>
#include <RendererFoundation/CommandEncoder/RenderCommandEncoder.h>
#include <RendererVulkan/CommandEncoder/CommandEncoderImplVulkan.h>
#include <RendererVulkan/Device/PassVulkan.h>

ezGALPassVulkan::ezGALPassVulkan(ezGALDevice& device)
  : ezGALPass(device)
{
  m_pCommandEncoderState = EZ_DEFAULT_NEW(ezGALCommandEncoderRenderState);
  m_pCommandEncoderImpl = EZ_DEFAULT_NEW(ezGALCommandEncoderImplVulkan, static_cast<ezGALDeviceVulkan&>(device));

  m_pRenderCommandEncoder = EZ_DEFAULT_NEW(ezGALRenderCommandEncoder, device, *m_pCommandEncoderState, *m_pCommandEncoderImpl, *m_pCommandEncoderImpl);
  m_pComputeCommandEncoder = EZ_DEFAULT_NEW(ezGALComputeCommandEncoder, device, *m_pCommandEncoderState, *m_pCommandEncoderImpl, *m_pCommandEncoderImpl);
}

ezGALPassVulkan::~ezGALPassVulkan() = default;

ezGALRenderCommandEncoder* ezGALPassVulkan::BeginRenderingPlatform(const ezGALRenderingSetup& renderingSetup, const char* szName)
{
  vk::CommandBuffer& commandBuffer = static_cast<ezGALDeviceVulkan&>(m_Device).GetCurrentCommandBuffer();

  m_pCommandEncoderImpl->BeginRendering(commandBuffer, renderingSetup);

  return m_pRenderCommandEncoder.Borrow();
}

void ezGALPassVulkan::EndRenderingPlatform(ezGALRenderCommandEncoder* pCommandEncoder)
{
  EZ_ASSERT_DEV(m_pRenderCommandEncoder.Borrow() == pCommandEncoder, "Invalid command encoder");

  m_pCommandEncoderImpl->EndRendering();
}

ezGALComputeCommandEncoder* ezGALPassVulkan::BeginComputePlatform(const char* szName)
{
  vk::CommandBuffer& commandBuffer = static_cast<ezGALDeviceVulkan&>(m_Device).GetCurrentCommandBuffer();

  m_pCommandEncoderImpl->BeginCompute(commandBuffer);

  return m_pComputeCommandEncoder.Borrow();
}

void ezGALPassVulkan::EndComputePlatform(ezGALComputeCommandEncoder* pCommandEncoder)
{
  EZ_ASSERT_DEV(m_pComputeCommandEncoder.Borrow() == pCommandEncoder, "Invalid command encoder");

  m_pCommandEncoderImpl->EndCompute();
}
