#pragma once

#include <OpenXRPlugin/Graphics/OpenXRGraphicsBinding.h>

#if EZ_OPENXR_HAS_VULKAN_RENDERER

EZ_DEFINE_AS_POD_TYPE(XrSwapchainImageVulkanKHR);

/// \brief Vulkan implementation of the OpenXR graphics binding.
class EZ_OPENXRPLUGIN_DLL ezOpenXRGraphicsBindingVulkan final : public ezOpenXRGraphicsBinding
{
public:
  ezOpenXRGraphicsBindingVulkan();
  ~ezOpenXRGraphicsBindingVulkan();

  virtual const char* GetName() const override { return "Vulkan"; }

  virtual XrResult SelectExtension(ezDynamicArray<const char*>& extensions, const ezDynamicArray<XrExtensionProperties>& extensionProperties) override;

  virtual void LoadFunctionPointers(XrInstance instance) override;

  virtual XrResult Initialize(XrInstance instance, XrSystemId systemId, ezGALDevice* pDevice) override;

  virtual void Deinitialize() override;

  virtual const void* GetGraphicsBinding() const override;

  virtual XrResult SelectSwapchainFormats(XrSession session, bool bDepthComposition, int64_t& out_colorFormat, int64_t& out_depthFormat) override;

  virtual XrResult CreateSwapchainImages(XrSwapchain swapchainHandle, int64_t format, ezUInt32 imageCount, ezSizeU32 size, ezGALMSAASampleCount::Enum msaaCount, bool bIsDepth, ezGALDevice* pDevice, ezDynamicArray<ezGALTextureHandle>& out_textures) override;

  virtual void CleanupSwapchainImages() override;

  /// \brief Returns whether the vulkan_enable (v1) extension is being used vs vulkan_enable2.
  bool IsUsingVulkanEnable() const { return m_bUsingVulkanEnable; }

private:
  ezGALResourceFormat::Enum ConvertTextureFormat(int64_t format) const;

  XrGraphicsBindingVulkanKHR m_GraphicsBinding{XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR};

  // Extension flags
  bool m_bUsingVulkanEnable = false;
  bool m_bUsingVulkanEnable2 = false;

  // Function pointers for vulkan_enable (v1)
  PFN_xrGetVulkanGraphicsRequirementsKHR m_pfnGetVulkanGraphicsRequirementsKHR = nullptr;
  PFN_xrGetVulkanInstanceExtensionsKHR m_pfnGetVulkanInstanceExtensionsKHR = nullptr;
  PFN_xrGetVulkanDeviceExtensionsKHR m_pfnGetVulkanDeviceExtensionsKHR = nullptr;
  PFN_xrGetVulkanGraphicsDeviceKHR m_pfnGetVulkanGraphicsDeviceKHR = nullptr;

  // Function pointers for vulkan_enable2 (v2)
  PFN_xrGetVulkanGraphicsRequirements2KHR m_pfnGetVulkanGraphicsRequirements2KHR = nullptr;

  // Swapchain images storage
  ezHybridArray<XrSwapchainImageVulkanKHR, 3> m_ColorSwapchainImages;
  ezHybridArray<XrSwapchainImageVulkanKHR, 3> m_DepthSwapchainImages;
};

#endif // EZ_OPENXR_HAS_VULKAN_RENDERER
