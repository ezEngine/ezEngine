
#include <RendererVulkanPCH.h>

#include <RendererVulkan/CommandEncoder/ComputeCommandEncoderVulkan.h>
#include <RendererVulkan/Resources/BufferVulkan.h>

ezGALComputeCommandEncoderVulkan::ezGALComputeCommandEncoderVulkan(ezGALDevice& device)
  : SUPER(device)
{
}

ezGALComputeCommandEncoderVulkan::~ezGALComputeCommandEncoderVulkan() = default;

void ezGALComputeCommandEncoderVulkan::BeginEncode(vk::CommandBuffer& commandBuffer)
{
  m_pCommandBuffer = &commandBuffer;

  // TODO: do we need a renderpass for compute only?
}

void ezGALComputeCommandEncoderVulkan::EndEncode()
{
  m_pCommandBuffer = nullptr;
}

void ezGALComputeCommandEncoderVulkan::DispatchPlatform(ezUInt32 uiThreadGroupCountX, ezUInt32 uiThreadGroupCountY, ezUInt32 uiThreadGroupCountZ)
{
  m_pCommandBuffer->dispatch(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);
}

void ezGALComputeCommandEncoderVulkan::DispatchIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  m_pCommandBuffer->dispatchIndirect(static_cast<const ezGALBufferVulkan*>(pIndirectArgumentBuffer)->GetVkBuffer(), uiArgumentOffsetInBytes);
}
