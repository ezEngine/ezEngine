#pragma once

#include <Foundation/Basics.h>
#include <OpenXRPlugin/Basics.h>
#include <OpenXRPlugin/OpenXRIncludes.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

class ezGALDevice;

/// \brief Abstract interface for graphics API binding in OpenXR.
///
/// This interface abstracts away the graphics API specific code required for OpenXR integration.
class EZ_OPENXRPLUGIN_DLL ezOpenXRGraphicsBinding
{
public:
  virtual ~ezOpenXRGraphicsBinding() = default;

  /// \brief Returns the name of the graphics API (e.g., "D3D11", "Vulkan")
  virtual const char* GetName() const = 0;

  /// \brief Adds the required graphics API extension to the list of extensions to enable.
  virtual XrResult SelectExtension(ezDynamicArray<const char*>& extensions, const ezDynamicArray<XrExtensionProperties>& extensionProperties) = 0;

  /// \brief Loads the graphics API specific OpenXR function pointers.
  virtual void LoadFunctionPointers(XrInstance instance) = 0;

  /// \brief Initializes the graphics binding using the current GAL device.
  virtual XrResult Initialize(XrInstance instance, XrSystemId systemId, ezGALDevice* pDevice) = 0;

  /// \brief Deinitializes the graphics binding.
  virtual void Deinitialize() = 0;

  /// \brief Returns the graphics binding structure to be passed to xrCreateSession.
  virtual const void* GetGraphicsBinding() const = 0;

  /// \brief Selects appropriate swapchain formats for color and depth buffers.
  virtual XrResult SelectSwapchainFormats(XrSession session, bool bDepthComposition, int64_t& out_colorFormat, int64_t& out_depthFormat) = 0;

  /// \brief Creates texture handles from swapchain images.
  virtual XrResult CreateSwapchainImages(XrSwapchain swapchainHandle, int64_t format, ezUInt32 imageCount, ezSizeU32 size, ezGALMSAASampleCount::Enum msaaCount, bool bIsDepth, ezGALDevice* pDevice, ezDynamicArray<ezGALTextureHandle>& out_textures) = 0;

  /// \brief Cleans up any internal swapchain image storage.
  virtual void CleanupSwapchainImages() = 0;

  /// \brief Creates the appropriate graphics binding for the current platform and GAL device.
  static ezUniquePtr<ezOpenXRGraphicsBinding> Create(class ezOpenXR* pOpenXR, ezGALDevice* pDevice);
};
