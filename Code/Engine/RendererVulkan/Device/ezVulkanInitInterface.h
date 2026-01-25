#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

/// \brief Interface for customizing Vulkan instance and device creation.
///
/// This interface is used by systems like OpenXR that need to control how the Vulkan instance and device are created to ensure the correct extensions and physical device are used.
class ezVulkanInitInterface
{
public:
  virtual ~ezVulkanInitInterface() = default;

  /// \brief Creates the Vulkan instance using platform-specific requirements.
  /// Returns nullptr if the instance should be created normally by the renderer.
  virtual vk::Instance CreateInstance(const vk::InstanceCreateInfo& createInfo) = 0;

  /// \brief Returns the physical device to use.
  /// Returns nullptr if the renderer should select the physical device normally.
  virtual vk::PhysicalDevice GetPhysicalDevice(vk::Instance instance) = 0;

  /// \brief Creates the Vulkan device using platform-specific requirements.
  /// Returns nullptr if the device should be created normally by the renderer.
  virtual vk::Device CreateDevice(const vk::DeviceCreateInfo& createInfo) = 0;

  /// \brief Extends the instance extensions required.
  /// Used by XR_KHR_vulkan_enable (v1) to add required instance extensions.
  virtual void ExtendInstanceExtensions(const ezDynamicArray<vk::ExtensionProperties>& availableExtensions, ezDynamicArray<ezString>& ref_extensions) {}

  /// \brief Extends the device extensions required.
  /// Used by XR_KHR_vulkan_enable (v1) to add required device extensions.
  virtual void ExtendDeviceExtensions(const ezDynamicArray<vk::ExtensionProperties>& availableExtensions, ezDynamicArray<ezString>& ref_extensions) {}
};
