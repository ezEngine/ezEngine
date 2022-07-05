#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

#include <vulkan/vulkan.hpp>

EZ_DEFINE_AS_POD_TYPE(vk::PresentModeKHR);

/// \brief Helper functions to convert and extract Vulkan objects from EZ objects.
class EZ_RENDERERVULKAN_DLL ezConversionUtilsVulkan
{
public:
  /// \brief Helper function to hash vk enums.
  template <typename T, typename R = typename std::underlying_type<T>::type>
  static R GetUnderlyingValue(T value)
  {
    return static_cast<typename std::underlying_type<T>::type>(value);
  }

  /// \brief Helper function to hash vk flags.
  template <typename T>
  static auto GetUnderlyingFlagsValue(T value)
  {
    return static_cast<typename T::MaskType>(value);
  }

  static vk::SampleCountFlagBits GetSamples(ezEnum<ezGALMSAASampleCount> samples);
  static vk::PresentModeKHR GetPresentMode(ezEnum<ezGALPresentMode> presentMode, const ezDynamicArray<vk::PresentModeKHR>& supportedModes);
  static vk::ImageSubresourceRange GetSubresourceRange(const ezGALTextureCreationDescription& texDesc, const ezGALRenderTargetViewCreationDescription& desc);
  static vk::ImageSubresourceRange GetSubresourceRange(const ezGALTextureCreationDescription& texDesc, const ezGALResourceViewCreationDescription& viewDesc);
  static vk::ImageSubresourceRange GetSubresourceRange(const ezGALTextureCreationDescription& texDesc, const ezGALUnorderedAccessViewCreationDescription& viewDesc);
  static vk::ImageViewType GetImageViewType(ezEnum<ezGALTextureType> texType, bool bIsArray);

  static bool IsDepthFormat(vk::Format format);
  static bool IsStencilFormat(vk::Format format);
  static vk::PrimitiveTopology GetPrimitiveTopology(ezEnum<ezGALPrimitiveTopology> topology);
  static vk::ShaderStageFlagBits GetShaderStage(ezGALShaderStage::Enum stage);
  static vk::PipelineStageFlags GetPipelineStage(ezGALShaderStage::Enum stage);
  static vk::PipelineStageFlags GetPipelineStage(vk::ShaderStageFlags flags);
};

#include <RendererVulkan/Utils/Implementation/ConversionUtilsVulkan.inl.h>
