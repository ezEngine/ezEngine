#pragma once

#include <Foundation/Configuration/Singleton.h>
#include <OpenXRPlugin/Graphics/OpenXRGraphicsBinding.h>

#ifdef BUILDSYSTEM_ENABLE_VULKAN_SUPPORT

#  include <RendererVulkan/Device/ezVulkanInitInterface.h>

EZ_DEFINE_AS_POD_TYPE(XrSwapchainImageVulkanKHR);

class ezOpenXR;

/// \brief Vulkan implementation of the OpenXR graphics binding.
class EZ_OPENXRPLUGIN_DLL ezOpenXRGraphicsBindingVulkan final : public ezOpenXRGraphicsBinding, public ezVulkanInitInterface
{
  EZ_DECLARE_SINGLETON_OF_INTERFACE(ezOpenXRGraphicsBindingVulkan, ezVulkanInitInterface);

public:
  ezOpenXRGraphicsBindingVulkan(ezOpenXR* pOpenXR);
  ~ezOpenXRGraphicsBindingVulkan();

  // ezOpenXRGraphicsBinding
  virtual const char* GetName() const override { return "Vulkan"; }
  virtual XrResult SelectExtension(ezDynamicArray<const char*>& extensions, const ezDynamicArray<XrExtensionProperties>& extensionProperties) override;
  virtual void LoadFunctionPointers(XrInstance instance) override;
  virtual XrResult Initialize(XrInstance instance, XrSystemId systemId, ezGALDevice* pDevice) override;
  virtual void Deinitialize() override;
  virtual const void* GetGraphicsBinding() const override;
  virtual XrResult SelectSwapchainFormats(XrSession session, bool bDepthComposition, int64_t& out_colorFormat, int64_t& out_depthFormat) override;
  virtual XrResult CreateSwapchainImages(XrSwapchain swapchainHandle, int64_t format, ezUInt32 imageCount, ezSizeU32 size, ezGALMSAASampleCount::Enum msaaCount, bool bIsDepth, ezGALDevice* pDevice, ezDynamicArray<ezGALTextureHandle>& out_textures) override;
  virtual void CleanupSwapchainImages() override;

  // ezVulkanInitInterface
  virtual vk::Instance CreateInstance(const vk::InstanceCreateInfo& createInfo) override;
  virtual vk::PhysicalDevice GetPhysicalDevice(vk::Instance instance) override;
  virtual vk::Device CreateDevice(const vk::DeviceCreateInfo& createInfo) override;

  /// \brief Extends the instance extensions required for OpenXR (for vulkan_enable v1)
  virtual void ExtendInstanceExtensions(const ezDynamicArray<vk::ExtensionProperties>& availableExtensions, ezDynamicArray<ezString>& ref_extensions) override;
  /// \brief Extends the device extensions required for OpenXR (for vulkan_enable v1)
  virtual void ExtendDeviceExtensions(const ezDynamicArray<vk::ExtensionProperties>& availableExtensions, ezDynamicArray<ezString>& ref_extensions) override;

private:
  ezOpenXR* m_pOpenXR = nullptr;
  ezGALResourceFormat::Enum ConvertTextureFormat(int64_t format) const;

  XrGraphicsBindingVulkanKHR m_GraphicsBinding{XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR};

  bool m_bUsingVulkanEnable2 = false; // true if using vulkan_enable2, false if using vulkan_enable

  // Function pointers for vulkan_enable2
  PFN_xrCreateVulkanInstanceKHR m_pfnCreateVulkanInstanceKHR = nullptr;
  PFN_xrCreateVulkanDeviceKHR m_pfnCreateVulkanDeviceKHR = nullptr;
  PFN_xrGetVulkanGraphicsDevice2KHR m_pfnGetVulkanGraphicsDevice2KHR = nullptr;
  PFN_xrGetVulkanGraphicsRequirements2KHR m_pfnGetVulkanGraphicsRequirements2KHR = nullptr;

  // Function pointers for vulkan_enable (v1)
  PFN_xrGetVulkanInstanceExtensionsKHR m_pfnGetVulkanInstanceExtensionsKHR = nullptr;
  PFN_xrGetVulkanDeviceExtensionsKHR m_pfnGetVulkanDeviceExtensionsKHR = nullptr;
  PFN_xrGetVulkanGraphicsDeviceKHR m_pfnGetVulkanGraphicsDeviceKHR = nullptr;
  PFN_xrGetVulkanGraphicsRequirementsKHR m_pfnGetVulkanGraphicsRequirementsKHR = nullptr;

  // Vulkan handles created by OpenXR via vulkan_enable2
  vk::Instance m_VulkanInstance;
  vk::PhysicalDevice m_VulkanPhysicalDevice;
  vk::Device m_VulkanDevice;

  // Swapchain images storage
  ezHybridArray<XrSwapchainImageVulkanKHR, 3> m_ColorSwapchainImages;
  ezHybridArray<XrSwapchainImageVulkanKHR, 3> m_DepthSwapchainImages;
};

#endif
