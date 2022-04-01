#include <RendererFoundation/Resources/ResourceFormats.h>

namespace
{
  bool IsArrayViewInternal(const ezGALTextureCreationDescription& texDesc, const ezGALResourceViewCreationDescription& viewDesc)
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
  range.layerCount = viewDesc.m_uiArraySize;
  range.levelCount = viewDesc.m_uiMipLevelsToUse;

  switch (texDesc.m_Type)
  {
    case ezGALTextureType::Texture2D:
    case ezGALTextureType::Texture2DProxy:
      if (bIsArrayView)
      {
        range.baseArrayLayer = viewDesc.m_uiFirstArraySlice;
        range.baseMipLevel = viewDesc.m_uiMostDetailedMipLevel;
      }
      break;
    case ezGALTextureType::TextureCube:
      if (bIsArrayView)
      {
        range.baseArrayLayer = viewDesc.m_uiFirstArraySlice;
        range.baseMipLevel = viewDesc.m_uiMostDetailedMipLevel;
      }
      break;
    case ezGALTextureType::Texture3D:
      break;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }
  return range;
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
