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

  static vk::AttachmentLoadOp GetAttachmentLoadOp(ezEnum<ezGALRenderTargetLoadOp> op);
  static vk::AttachmentStoreOp GetAttachmentStoreOp(ezEnum<ezGALRenderTargetStoreOp> op);
  static vk::VertexInputRate GetVertexBindingRate(ezEnum<ezGALVertexBindingRate> rate);
  static vk::SampleCountFlagBits GetSamples(ezEnum<ezGALMSAASampleCount> samples);
  static vk::PresentModeKHR GetPresentMode(ezEnum<ezGALPresentMode> presentMode, const ezDynamicArray<vk::PresentModeKHR>& supportedModes);
  static vk::ImageSubresourceRange GetSubresourceRange(const ezGALTextureCreationDescription& texDesc, const ezGALRenderTargetViewCreationDescription& desc);
  static vk::ImageSubresourceRange GetSubresourceRange(const ezGALTextureCreationDescription& texDesc, const ezGALTextureResourceViewCreationDescription& viewDesc);
  static vk::ImageSubresourceRange GetSubresourceRange(const ezGALTextureCreationDescription& texDesc, const ezGALTextureUnorderedAccessViewCreationDescription& viewDesc);
  static vk::ImageSubresourceRange GetSubresourceRange(const vk::ImageSubresourceLayers& layers);
  static vk::ImageViewType GetImageViewType(ezEnum<ezGALTextureType> texType);
  static vk::ImageViewType GetImageArrayViewType(ezEnum<ezGALTextureType> texType);

  static bool IsDepthFormat(vk::Format format);
  static bool IsStencilFormat(vk::Format format);
  static vk::ImageLayout GetDefaultLayout(vk::Format format);
  static vk::PrimitiveTopology GetPrimitiveTopology(ezEnum<ezGALPrimitiveTopology> topology);
  static vk::ShaderStageFlagBits GetShaderStage(ezGALShaderStage::Enum stage);
  static vk::ShaderStageFlagBits GetShaderStages(ezBitflags<ezGALShaderStageFlags> stages);
  static vk::PipelineStageFlags GetPipelineStage(ezGALShaderStage::Enum stage);
  static vk::PipelineStageFlags GetPipelineStage(vk::ShaderStageFlags flags);
  static vk::PipelineStageFlags GetPipelineStages(ezBitflags<ezGALShaderStageFlags> stages);
  static vk::DescriptorType GetDescriptorType(ezGALShaderResourceType::Enum type);
};

#include <RendererVulkan/Utils/Implementation/ConversionUtilsVulkan.inl.h>
