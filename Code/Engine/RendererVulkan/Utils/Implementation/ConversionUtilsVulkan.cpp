#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

void ezConversionUtilsVulkan::ConvertResourceState(ezBitflags<ezGALResourceState> state, vk::PipelineStageFlags& out_stages, vk::AccessFlags& out_access)
{
  out_stages = {};
  out_access = {};

  // All shader stages that can access resources. Unsupported stages must be masked
  // out by the caller using ezGALDeviceVulkan::GetSupportedStages().
  constexpr vk::PipelineStageFlags allShaderStages =
    vk::PipelineStageFlagBits::eVertexShader |
    vk::PipelineStageFlagBits::eTessellationControlShader |
    vk::PipelineStageFlagBits::eTessellationEvaluationShader |
    vk::PipelineStageFlagBits::eGeometryShader |
    vk::PipelineStageFlagBits::eFragmentShader |
    vk::PipelineStageFlagBits::eComputeShader;

  for (auto flag : state)
  {
    switch (flag)
    {
      case ezGALResourceState::ShaderResource:
        out_stages |= allShaderStages;
        out_access |= vk::AccessFlagBits::eShaderRead;
        break;

      case ezGALResourceState::ConstantBuffer:
        out_stages |= allShaderStages;
        out_access |= vk::AccessFlagBits::eUniformRead;
        break;

      case ezGALResourceState::VertexBuffer:
        out_stages |= vk::PipelineStageFlagBits::eVertexInput;
        out_access |= vk::AccessFlagBits::eVertexAttributeRead;
        break;

      case ezGALResourceState::IndexBuffer:
        out_stages |= vk::PipelineStageFlagBits::eVertexInput;
        out_access |= vk::AccessFlagBits::eIndexRead;
        break;

      case ezGALResourceState::DrawIndirect:
        out_stages |= vk::PipelineStageFlagBits::eDrawIndirect;
        out_access |= vk::AccessFlagBits::eIndirectCommandRead;
        break;

      case ezGALResourceState::DepthStencilRead:
        out_stages |= vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;
        out_access |= vk::AccessFlagBits::eDepthStencilAttachmentRead;
        break;

      case ezGALResourceState::CopySource:
        out_stages |= vk::PipelineStageFlagBits::eTransfer;
        out_access |= vk::AccessFlagBits::eTransferRead;
        break;

      case ezGALResourceState::ResolveSource:
        out_stages |= vk::PipelineStageFlagBits::eTransfer;
        out_access |= vk::AccessFlagBits::eTransferRead;
        break;

      case ezGALResourceState::UnorderedAccess:
        out_stages |= allShaderStages;
        out_access |= vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite;
        break;

      case ezGALResourceState::RenderTarget:
        out_stages |= vk::PipelineStageFlagBits::eColorAttachmentOutput;
        out_access |= vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
        break;

      case ezGALResourceState::DepthStencilWrite:
        out_stages |= vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;
        out_access |= vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        break;

      case ezGALResourceState::CopyDestination:
        out_stages |= vk::PipelineStageFlagBits::eTransfer;
        out_access |= vk::AccessFlagBits::eTransferWrite;
        break;

      case ezGALResourceState::ResolveDestination:
        out_stages |= vk::PipelineStageFlagBits::eTransfer;
        out_access |= vk::AccessFlagBits::eTransferWrite;
        break;

      case ezGALResourceState::Present:
        // No stage/access needed — presentation is handled outside the render pipeline.
        break;

      case ezGALResourceState::CpuRead:
        out_stages |= vk::PipelineStageFlagBits::eHost;
        out_access |= vk::AccessFlagBits::eHostRead;
        break;

      case ezGALResourceState::CpuWrite:
        out_stages |= vk::PipelineStageFlagBits::eHost;
        out_access |= vk::AccessFlagBits::eHostWrite;
        break;

      default:
        EZ_ASSERT_NOT_IMPLEMENTED;
        break;
    }
  }

  // If no stages were determined, default to top/bottom of pipe.
  if (!out_stages)
    out_stages = vk::PipelineStageFlagBits::eTopOfPipe;
}

void ezConversionUtilsVulkan::ConvertResourceState(ezBitflags<ezGALResourceState> state, vk::PipelineStageFlags2& out_stages, vk::AccessFlags2& out_access)
{
  out_stages = {};
  out_access = {};

  constexpr vk::PipelineStageFlags2 allShaderStages =
    vk::PipelineStageFlagBits2::eVertexShader |
    vk::PipelineStageFlagBits2::eTessellationControlShader |
    vk::PipelineStageFlagBits2::eTessellationEvaluationShader |
    vk::PipelineStageFlagBits2::eGeometryShader |
    vk::PipelineStageFlagBits2::eFragmentShader |
    vk::PipelineStageFlagBits2::eComputeShader;

  for (auto flag : state)
  {
    switch (flag)
    {
      case ezGALResourceState::ShaderResource:
        out_stages |= allShaderStages;
        out_access |= vk::AccessFlagBits2::eShaderRead;
        break;

      case ezGALResourceState::ConstantBuffer:
        out_stages |= allShaderStages;
        out_access |= vk::AccessFlagBits2::eUniformRead;
        break;

      case ezGALResourceState::VertexBuffer:
        out_stages |= vk::PipelineStageFlagBits2::eVertexAttributeInput;
        out_access |= vk::AccessFlagBits2::eVertexAttributeRead;
        break;

      case ezGALResourceState::IndexBuffer:
        out_stages |= vk::PipelineStageFlagBits2::eIndexInput;
        out_access |= vk::AccessFlagBits2::eIndexRead;
        break;

      case ezGALResourceState::DrawIndirect:
        out_stages |= vk::PipelineStageFlagBits2::eDrawIndirect;
        out_access |= vk::AccessFlagBits2::eIndirectCommandRead;
        break;

      case ezGALResourceState::DepthStencilRead:
        out_stages |= vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests;
        out_access |= vk::AccessFlagBits2::eDepthStencilAttachmentRead;
        break;

      case ezGALResourceState::CopySource:
        out_stages |= vk::PipelineStageFlagBits2::eCopy;
        out_access |= vk::AccessFlagBits2::eTransferRead;
        break;

      case ezGALResourceState::ResolveSource:
        out_stages |= vk::PipelineStageFlagBits2::eResolve;
        out_access |= vk::AccessFlagBits2::eTransferRead;
        break;

      case ezGALResourceState::UnorderedAccess:
        out_stages |= allShaderStages;
        out_access |= vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite;
        break;

      case ezGALResourceState::RenderTarget:
        out_stages |= vk::PipelineStageFlagBits2::eColorAttachmentOutput;
        out_access |= vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite;
        break;

      case ezGALResourceState::DepthStencilWrite:
        out_stages |= vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests;
        out_access |= vk::AccessFlagBits2::eDepthStencilAttachmentRead | vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
        break;

      case ezGALResourceState::CopyDestination:
        out_stages |= vk::PipelineStageFlagBits2::eCopy;
        out_access |= vk::AccessFlagBits2::eTransferWrite;
        break;

      case ezGALResourceState::ResolveDestination:
        out_stages |= vk::PipelineStageFlagBits2::eResolve;
        out_access |= vk::AccessFlagBits2::eTransferWrite;
        break;

      case ezGALResourceState::Present:
        break;

      case ezGALResourceState::CpuRead:
        out_stages |= vk::PipelineStageFlagBits2::eHost;
        out_access |= vk::AccessFlagBits2::eHostRead;
        break;

      case ezGALResourceState::CpuWrite:
        out_stages |= vk::PipelineStageFlagBits2::eHost;
        out_access |= vk::AccessFlagBits2::eHostWrite;
        break;

      default:
        EZ_ASSERT_NOT_IMPLEMENTED;
        break;
    }
  }

  if (!out_stages)
    out_stages = vk::PipelineStageFlagBits2::eNone;
}

vk::ImageLayout ezConversionUtilsVulkan::GetTextureLayout(ezBitflags<ezGALResourceState> state)
{
  // Write states require a specific layout.
  if (state.IsSet(ezGALResourceState::RenderTarget))
    return vk::ImageLayout::eColorAttachmentOptimal;

  if (state.IsSet(ezGALResourceState::DepthStencilWrite))
    return vk::ImageLayout::eDepthStencilAttachmentOptimal;

  if (state.IsSet(ezGALResourceState::UnorderedAccess))
    return vk::ImageLayout::eGeneral;

  if (state.IsSet(ezGALResourceState::CopyDestination))
    return vk::ImageLayout::eTransferDstOptimal;

  if (state.IsSet(ezGALResourceState::ResolveDestination))
    return vk::ImageLayout::eTransferDstOptimal;

  // Read-only states.
  // If multiple read flags are set, different layouts would conflict — fall back to eGeneral.
  const ezBitflags<ezGALResourceState> readStates = state & ezGALResourceState::AllReadStates;
  if (readStates.GetValue() & (readStates.GetValue() - 1))
    return vk::ImageLayout::eGeneral;

  if (state.IsSet(ezGALResourceState::DepthStencilRead))
    return vk::ImageLayout::eDepthStencilReadOnlyOptimal;

  if (state.IsSet(ezGALResourceState::ShaderResource))
    return vk::ImageLayout::eShaderReadOnlyOptimal;

  if (state.IsSet(ezGALResourceState::CopySource))
    return vk::ImageLayout::eTransferSrcOptimal;

  if (state.IsSet(ezGALResourceState::ResolveSource))
    return vk::ImageLayout::eTransferSrcOptimal;

  // Special states.
  if (state.IsSet(ezGALResourceState::Present))
    return vk::ImageLayout::ePresentSrcKHR;

  // Unknown or no state.
  return vk::ImageLayout::eUndefined;
}
