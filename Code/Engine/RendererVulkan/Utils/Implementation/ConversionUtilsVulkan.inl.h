#include <RendererFoundation/Resources/ResourceFormats.h>

namespace
{
  bool IsArrayViewInternal(const ezGALTextureCreationDescription& texDesc, const ezGALResourceViewCreationDescription& viewDesc)
  {
    return texDesc.m_uiArraySize > 1 || viewDesc.m_uiFirstArraySlice > 0;
  }
  bool IsArrayViewInternal(const ezGALTextureCreationDescription& texDesc, const ezGALUnorderedAccessViewCreationDescription& viewDesc)
  {
    return texDesc.m_uiArraySize > 1 || viewDesc.m_uiFirstArraySlice > 0;
  }
} // namespace

EZ_ALWAYS_INLINE vk::SampleCountFlagBits ezConversionUtilsVulkan::GetSamples(ezEnum<ezGALMSAASampleCount> samples)
{
  switch (samples)
  {
    case ezGALMSAASampleCount::None:
      return vk::SampleCountFlagBits::e1;
    case ezGALMSAASampleCount::TwoSamples:
      return vk::SampleCountFlagBits::e2;
    case ezGALMSAASampleCount::FourSamples:
      return vk::SampleCountFlagBits::e4;
    case ezGALMSAASampleCount::EightSamples:
      return vk::SampleCountFlagBits::e8;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      return vk::SampleCountFlagBits::e1;
  }
}

EZ_ALWAYS_INLINE vk::PresentModeKHR ezConversionUtilsVulkan::GetPresentMode(ezEnum<ezGALPresentMode> presentMode, const ezDynamicArray<vk::PresentModeKHR>& supportedModes)
{
  switch (presentMode)
  {
    case ezGALPresentMode::Immediate:
    {
      if (supportedModes.Contains(vk::PresentModeKHR::eImmediate))
        return vk::PresentModeKHR::eImmediate;
      else if (supportedModes.Contains(vk::PresentModeKHR::eMailbox))
        return vk::PresentModeKHR::eMailbox;
      else
        return vk::PresentModeKHR::eFifo;
    }
    case ezGALPresentMode::VSync:
      return vk::PresentModeKHR::eFifo; // FIFO must be supported according to the standard.
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      return vk::PresentModeKHR::eFifo;
  }
}

EZ_ALWAYS_INLINE vk::ImageSubresourceRange ezConversionUtilsVulkan::GetSubresourceRange(const ezGALTextureCreationDescription& texDesc, const ezGALRenderTargetViewCreationDescription& viewDesc)
{
  vk::ImageSubresourceRange range;
  ezGALResourceFormat::Enum viewFormat = viewDesc.m_OverrideViewFormat == ezGALResourceFormat::Invalid ? texDesc.m_Format : viewDesc.m_OverrideViewFormat;
  range.aspectMask = ezGALResourceFormat::IsDepthFormat(viewFormat) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
  range.setBaseMipLevel(viewDesc.m_uiMipLevel).setLevelCount(1).setBaseArrayLayer(viewDesc.m_uiFirstSlice).setLayerCount(viewDesc.m_uiSliceCount);
  return range;
}

EZ_ALWAYS_INLINE vk::ImageSubresourceRange ezConversionUtilsVulkan::GetSubresourceRange(const ezGALTextureCreationDescription& texDesc, const ezGALResourceViewCreationDescription& viewDesc)
{
  vk::ImageSubresourceRange range;

  const bool bIsArrayView = IsArrayViewInternal(texDesc, viewDesc);

  ezGALResourceFormat::Enum viewFormat = viewDesc.m_OverrideViewFormat == ezGALResourceFormat::Invalid ? texDesc.m_Format : viewDesc.m_OverrideViewFormat;
  range.aspectMask = ezGALResourceFormat::IsDepthFormat(viewFormat) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
  if (viewFormat == ezGALResourceFormat::D24S8)
  {
    range.aspectMask |= vk::ImageAspectFlagBits::eStencil;
  }
  range.baseMipLevel = viewDesc.m_uiMostDetailedMipLevel;
  range.levelCount = ezMath::Min(viewDesc.m_uiMipLevelsToUse, texDesc.m_uiMipLevelCount - range.baseMipLevel);

  switch (texDesc.m_Type)
  {
    case ezGALTextureType::Texture2D:
    case ezGALTextureType::Texture2DProxy:
      range.layerCount = viewDesc.m_uiArraySize;
      range.baseArrayLayer = viewDesc.m_uiFirstArraySlice;
      break;
    case ezGALTextureType::TextureCube:
      range.baseArrayLayer = viewDesc.m_uiFirstArraySlice;
      range.layerCount = viewDesc.m_uiArraySize * 6;
      break;
    case ezGALTextureType::Texture3D:
      range.layerCount = 1;
      break;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }
  return range;
}


EZ_ALWAYS_INLINE vk::ImageSubresourceRange ezConversionUtilsVulkan::GetSubresourceRange(const ezGALTextureCreationDescription& texDesc, const ezGALUnorderedAccessViewCreationDescription& viewDesc)
{
  vk::ImageSubresourceRange range;

  const bool bIsArrayView = IsArrayViewInternal(texDesc, viewDesc);

  ezGALResourceFormat::Enum viewFormat = viewDesc.m_OverrideViewFormat == ezGALResourceFormat::Invalid ? texDesc.m_Format : viewDesc.m_OverrideViewFormat;
  range.aspectMask = ezGALResourceFormat::IsDepthFormat(viewFormat) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
  if (viewFormat == ezGALResourceFormat::D24S8)
  {
    range.aspectMask |= vk::ImageAspectFlagBits::eStencil;
  }

  range.baseMipLevel = viewDesc.m_uiMipLevelToUse;
  range.levelCount = 1;
  range.layerCount = viewDesc.m_uiArraySize;

  switch (texDesc.m_Type)
  {
    case ezGALTextureType::Texture2D:
    case ezGALTextureType::Texture2DProxy:
      range.baseArrayLayer = viewDesc.m_uiFirstArraySlice;
      break;
    case ezGALTextureType::TextureCube:
      range.baseArrayLayer = viewDesc.m_uiFirstArraySlice;
      break;
    case ezGALTextureType::Texture3D:
      if (bIsArrayView)
      {
        EZ_ASSERT_NOT_IMPLEMENTED;
      }
      else
      {
        range.baseArrayLayer = viewDesc.m_uiFirstArraySlice;
      }
      break;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }
  return range;
}

EZ_ALWAYS_INLINE vk::ImageViewType ezConversionUtilsVulkan::GetImageViewType(ezEnum<ezGALTextureType> texType, bool bIsArrayView)
{
  switch (texType)
  {
    case ezGALTextureType::Texture2D:
    case ezGALTextureType::Texture2DProxy:
      if (!bIsArrayView)
      {
        return vk::ImageViewType::e2D;
      }
      else
      {
        return vk::ImageViewType::e2DArray;
      }
    case ezGALTextureType::TextureCube:
      if (!bIsArrayView)
      {
        return vk::ImageViewType::eCube;
      }
      else
      {
        return vk::ImageViewType::eCubeArray;
      }
    case ezGALTextureType::Texture3D:
      return vk::ImageViewType::e3D;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      return vk::ImageViewType::e1D;
  }
}

EZ_ALWAYS_INLINE bool ezConversionUtilsVulkan::IsDepthFormat(vk::Format format)
{
  switch (format)
  {
    case vk::Format::eD16Unorm:
    case vk::Format::eD32Sfloat:
    case vk::Format::eD16UnormS8Uint:
    case vk::Format::eD24UnormS8Uint:
    case vk::Format::eD32SfloatS8Uint:
      return true;
    default:
      return false;
  }
}

EZ_ALWAYS_INLINE bool ezConversionUtilsVulkan::IsStencilFormat(vk::Format format)
{
  switch (format)
  {
    case vk::Format::eS8Uint:
    case vk::Format::eD16UnormS8Uint:
    case vk::Format::eD24UnormS8Uint:
    case vk::Format::eD32SfloatS8Uint:
      return true;
    default:
      return false;
  }
}

EZ_ALWAYS_INLINE vk::PrimitiveTopology ezConversionUtilsVulkan::GetPrimitiveTopology(ezEnum<ezGALPrimitiveTopology> topology)
{
  switch (topology)
  {
    case ezGALPrimitiveTopology::Points:
      return vk::PrimitiveTopology::ePointList;
    case ezGALPrimitiveTopology::Lines:
      return vk::PrimitiveTopology::eLineList;
    case ezGALPrimitiveTopology::Triangles:
      return vk::PrimitiveTopology::eTriangleList;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      return vk::PrimitiveTopology::ePointList;
  }
}

EZ_ALWAYS_INLINE vk::ShaderStageFlagBits ezConversionUtilsVulkan::GetShaderStage(ezGALShaderStage::Enum stage)
{
  switch (stage)
  {
    case ezGALShaderStage::VertexShader:
      return vk::ShaderStageFlagBits::eVertex;
    case ezGALShaderStage::HullShader:
      return vk::ShaderStageFlagBits::eTessellationControl;
    case ezGALShaderStage::DomainShader:
      return vk::ShaderStageFlagBits::eTessellationEvaluation;
    case ezGALShaderStage::GeometryShader:
      return vk::ShaderStageFlagBits::eGeometry;
    case ezGALShaderStage::PixelShader:
      return vk::ShaderStageFlagBits::eFragment;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      [[fallthrough]];
    case ezGALShaderStage::ComputeShader:
      return vk::ShaderStageFlagBits::eCompute;
  }
}

EZ_ALWAYS_INLINE vk::PipelineStageFlags ezConversionUtilsVulkan::GetPipelineStage(ezGALShaderStage::Enum stage)
{
  switch (stage)
  {
    case ezGALShaderStage::VertexShader:
      return vk::PipelineStageFlagBits::eVertexShader;
    case ezGALShaderStage::HullShader:
      return vk::PipelineStageFlagBits::eTessellationControlShader;
    case ezGALShaderStage::DomainShader:
      return vk::PipelineStageFlagBits::eTessellationEvaluationShader;
    case ezGALShaderStage::GeometryShader:
      return vk::PipelineStageFlagBits::eGeometryShader;
    case ezGALShaderStage::PixelShader:
      return vk::PipelineStageFlagBits::eFragmentShader;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      [[fallthrough]];
    case ezGALShaderStage::ComputeShader:
      return vk::PipelineStageFlagBits::eComputeShader;
  }
}

EZ_ALWAYS_INLINE vk::PipelineStageFlags ezConversionUtilsVulkan::GetPipelineStage(vk::ShaderStageFlags flags)
{
  vk::PipelineStageFlags res;
  if (flags & vk::ShaderStageFlagBits::eVertex)
    res |= vk::PipelineStageFlagBits::eVertexShader;
  if (flags & vk::ShaderStageFlagBits::eTessellationControl)
    res |= vk::PipelineStageFlagBits::eTessellationControlShader;
  if (flags & vk::ShaderStageFlagBits::eTessellationEvaluation)
    res |= vk::PipelineStageFlagBits::eTessellationEvaluationShader;
  if (flags & vk::ShaderStageFlagBits::eGeometry)
    res |= vk::PipelineStageFlagBits::eGeometryShader;
  if (flags & vk::ShaderStageFlagBits::eFragment)
    res |= vk::PipelineStageFlagBits::eFragmentShader;
  if (flags & vk::ShaderStageFlagBits::eCompute)
    res |= vk::PipelineStageFlagBits::eComputeShader;

  return res;
}
