#pragma once

#include <OpenXRPlugin/Graphics/OpenXRGraphicsBinding.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)

EZ_DEFINE_AS_POD_TYPE(XrSwapchainImageD3D11KHR);

/// \brief D3D11 implementation of the OpenXR graphics binding.
class EZ_OPENXRPLUGIN_DLL ezOpenXRGraphicsBindingD3D11 final : public ezOpenXRGraphicsBinding
{
public:
  ezOpenXRGraphicsBindingD3D11();
  ~ezOpenXRGraphicsBindingD3D11();

  virtual const char* GetName() const override { return "D3D11"; }
  virtual XrResult SelectExtension(ezDynamicArray<const char*>& extensions, const ezDynamicArray<XrExtensionProperties>& extensionProperties) override;
  virtual void LoadFunctionPointers(XrInstance instance) override;
  virtual XrResult Initialize(XrInstance instance, XrSystemId systemId, ezGALDevice* pDevice) override;
  virtual void Deinitialize() override;
  virtual const void* GetGraphicsBinding() const override;
  virtual XrResult SelectSwapchainFormats(XrSession session, bool bDepthComposition, int64_t& out_colorFormat, int64_t& out_depthFormat) override;
  virtual XrResult CreateSwapchainImages(XrSwapchain swapchainHandle, int64_t format, ezUInt32 imageCount, ezSizeU32 size, ezGALMSAASampleCount::Enum msaaCount, bool bIsDepth, ezGALDevice* pDevice, ezDynamicArray<ezGALTextureHandle>& out_textures) override;
  virtual void CleanupSwapchainImages() override;

private:
  ezGALResourceFormat::Enum ConvertTextureFormat(int64_t format) const;

  XrGraphicsBindingD3D11KHR m_GraphicsBinding{XR_TYPE_GRAPHICS_BINDING_D3D11_KHR};
  PFN_xrGetD3D11GraphicsRequirementsKHR m_pfnGetD3D11GraphicsRequirementsKHR = nullptr;

  // Swapchain images storage
  ezHybridArray<XrSwapchainImageD3D11KHR, 3> m_ColorSwapchainImages;
  ezHybridArray<XrSwapchainImageD3D11KHR, 3> m_DepthSwapchainImages;
};

#endif // EZ_ENABLED(EZ_PLATFORM_WINDOWS)
