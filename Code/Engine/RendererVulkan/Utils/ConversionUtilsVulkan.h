#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

#include <vulkan/vulkan.hpp>

/// \brief Helper functions to convert and extract Vulkan objects from EZ objects.
class EZ_RENDERERVULKAN_DLL ezConversionUtilsVulkan
{
public:
  static vk::SampleCountFlagBits GetSamples(ezEnum<ezGALMSAASampleCount> samples);
  static vk::PresentModeKHR GetPresentMode(ezEnum<ezGALPresentMode> presentMode);
  static vk::ImageSubresourceRange GetSubresourceRange(const ezGALTextureCreationDescription& texDesc, const ezGALRenderTargetViewCreationDescription& desc);
  static vk::ImageSubresourceRange GetSubresourceRange(const ezGALTextureCreationDescription& texDesc, const ezGALResourceViewCreationDescription& viewDesc);
  static bool IsDepthFormat(vk::Format format);
  static bool IsStencilFormat(vk::Format format);
};

#include <RendererVulkan/Utils/Implementation/ConversionUtilsVulkan.inl.h>