#include <OpenXRPlugin/OpenXRPluginPCH.h>

#include <OpenXRPlugin/Graphics/OpenXRGraphicsBindingVulkan.h>

#if EZ_OPENXR_HAS_VULKAN_RENDERER

#include <Foundation/Logging/Log.h>
#include <OpenXRPlugin/OpenXRDeclarations.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererVulkan/Device/DeviceVulkan.h>

ezOpenXRGraphicsBindingVulkan::ezOpenXRGraphicsBindingVulkan() = default;

ezOpenXRGraphicsBindingVulkan::~ezOpenXRGraphicsBindingVulkan()
{
  Deinitialize();
}

XrResult ezOpenXRGraphicsBindingVulkan::SelectExtension(ezDynamicArray<const char*>& extensions, const ezDynamicArray<XrExtensionProperties>& extensionProperties)
{
  // Check for vulkan_enable (v1) first - preferred because it allows using existing Vulkan instance/device
  bool bFoundV1 = false;
  bool bFoundV2 = false;
  for (const XrExtensionProperties& prop : extensionProperties)
  {
    if (ezStringUtils::IsEqual(prop.extensionName, XR_KHR_VULKAN_ENABLE_EXTENSION_NAME))
    {
      bFoundV1 = true;
    }
    else if (ezStringUtils::IsEqual(prop.extensionName, XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME))
    {
      bFoundV2 = true;
    }
  }

  if (bFoundV1)
  {
    extensions.PushBack(XR_KHR_VULKAN_ENABLE_EXTENSION_NAME);
    m_bUsingVulkanEnable = true;
    ezLog::Info("OpenXR: Enabled {} extension", XR_KHR_VULKAN_ENABLE_EXTENSION_NAME);
    return XR_SUCCESS;
  }

  // Fall back to vulkan_enable2 (v2)
  if (bFoundV2)
  {
    extensions.PushBack(XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME);
    m_bUsingVulkanEnable2 = true;
    ezLog::Info("OpenXR: Enabled {} extension", XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME);
    return XR_SUCCESS;
  }

  ezLog::Error("OpenXR: No Vulkan extension available (tried {} and {})", XR_KHR_VULKAN_ENABLE_EXTENSION_NAME, XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME);
  return XR_ERROR_EXTENSION_NOT_PRESENT;
}

void ezOpenXRGraphicsBindingVulkan::LoadFunctionPointers(XrInstance instance)
{
  if (m_bUsingVulkanEnable)
  {
    xrGetInstanceProcAddr(instance, "xrGetVulkanGraphicsRequirementsKHR", reinterpret_cast<PFN_xrVoidFunction*>(&m_pfnGetVulkanGraphicsRequirementsKHR));
    xrGetInstanceProcAddr(instance, "xrGetVulkanInstanceExtensionsKHR", reinterpret_cast<PFN_xrVoidFunction*>(&m_pfnGetVulkanInstanceExtensionsKHR));
    xrGetInstanceProcAddr(instance, "xrGetVulkanDeviceExtensionsKHR", reinterpret_cast<PFN_xrVoidFunction*>(&m_pfnGetVulkanDeviceExtensionsKHR));
    xrGetInstanceProcAddr(instance, "xrGetVulkanGraphicsDeviceKHR", reinterpret_cast<PFN_xrVoidFunction*>(&m_pfnGetVulkanGraphicsDeviceKHR));
  }

  if (m_bUsingVulkanEnable2)
  {
    xrGetInstanceProcAddr(instance, "xrGetVulkanGraphicsRequirements2KHR", reinterpret_cast<PFN_xrVoidFunction*>(&m_pfnGetVulkanGraphicsRequirements2KHR));
  }
}

XrResult ezOpenXRGraphicsBindingVulkan::Initialize(XrInstance instance, XrSystemId systemId, ezGALDevice* pDevice)
{
  ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);

  // Log required Vulkan instance extensions
  if (m_pfnGetVulkanInstanceExtensionsKHR)
  {
    uint32_t extensionsLength = 0;
    XrResult res = m_pfnGetVulkanInstanceExtensionsKHR(instance, systemId, 0, &extensionsLength, nullptr);
    if (res == XR_SUCCESS && extensionsLength > 0)
    {
      ezDynamicArray<char> extensionsBuffer;
      extensionsBuffer.SetCountUninitialized(extensionsLength);
      res = m_pfnGetVulkanInstanceExtensionsKHR(instance, systemId, extensionsLength, &extensionsLength, extensionsBuffer.GetData());
      if (res == XR_SUCCESS)
      {
        ezLog::Info("OpenXR required Vulkan instance extensions: {}", extensionsBuffer.GetData());
      }
    }
  }

  // Log required Vulkan device extensions
  if (m_pfnGetVulkanDeviceExtensionsKHR)
  {
    uint32_t extensionsLength = 0;
    XrResult res = m_pfnGetVulkanDeviceExtensionsKHR(instance, systemId, 0, &extensionsLength, nullptr);
    if (res == XR_SUCCESS && extensionsLength > 0)
    {
      ezDynamicArray<char> extensionsBuffer;
      extensionsBuffer.SetCountUninitialized(extensionsLength);
      res = m_pfnGetVulkanDeviceExtensionsKHR(instance, systemId, extensionsLength, &extensionsLength, extensionsBuffer.GetData());
      if (res == XR_SUCCESS)
      {
        ezLog::Info("OpenXR required Vulkan device extensions: {}", extensionsBuffer.GetData());
      }
    }
  }

  // Query Vulkan graphics requirements
  if (m_bUsingVulkanEnable2 && m_pfnGetVulkanGraphicsRequirements2KHR)
  {
    XrGraphicsRequirementsVulkan2KHR graphicsRequirements{XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN2_KHR};
    graphicsRequirements.next = nullptr;
    XrResult result = m_pfnGetVulkanGraphicsRequirements2KHR(instance, systemId, &graphicsRequirements);
    if (result != XR_SUCCESS)
    {
      ezLog::Error("OpenXR: xrGetVulkanGraphicsRequirements2KHR failed: {}", (int)result);
      return result;
    }

    ezLog::Info("OpenXR Vulkan requirements: minApiVersion={}.{}.{}, maxApiVersion={}.{}.{}",
      VK_API_VERSION_MAJOR(graphicsRequirements.minApiVersionSupported),
      VK_API_VERSION_MINOR(graphicsRequirements.minApiVersionSupported),
      VK_API_VERSION_PATCH(graphicsRequirements.minApiVersionSupported),
      VK_API_VERSION_MAJOR(graphicsRequirements.maxApiVersionSupported),
      VK_API_VERSION_MINOR(graphicsRequirements.maxApiVersionSupported),
      VK_API_VERSION_PATCH(graphicsRequirements.maxApiVersionSupported));
  }
  else if (m_pfnGetVulkanGraphicsRequirementsKHR)
  {
    XrGraphicsRequirementsVulkanKHR graphicsRequirements{XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR};
    graphicsRequirements.next = nullptr;
    XrResult result = m_pfnGetVulkanGraphicsRequirementsKHR(instance, systemId, &graphicsRequirements);
    if (result != XR_SUCCESS)
    {
      ezLog::Error("OpenXR: xrGetVulkanGraphicsRequirementsKHR failed: {}", (int)result);
      return result;
    }

    ezLog::Info("OpenXR Vulkan requirements: minApiVersion={}.{}.{}, maxApiVersion={}.{}.{}",
      VK_API_VERSION_MAJOR(graphicsRequirements.minApiVersionSupported),
      VK_API_VERSION_MINOR(graphicsRequirements.minApiVersionSupported),
      VK_API_VERSION_PATCH(graphicsRequirements.minApiVersionSupported),
      VK_API_VERSION_MAJOR(graphicsRequirements.maxApiVersionSupported),
      VK_API_VERSION_MINOR(graphicsRequirements.maxApiVersionSupported),
      VK_API_VERSION_PATCH(graphicsRequirements.maxApiVersionSupported));
  }

  // Fill in the Vulkan graphics binding
  m_GraphicsBinding.type = XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR;
  m_GraphicsBinding.next = nullptr;
  m_GraphicsBinding.instance = pVulkanDevice->GetVulkanInstance();
  m_GraphicsBinding.physicalDevice = pVulkanDevice->GetVulkanPhysicalDevice();
  m_GraphicsBinding.device = pVulkanDevice->GetVulkanDevice();
  m_GraphicsBinding.queueFamilyIndex = pVulkanDevice->GetGraphicsQueue().m_uiQueueFamily;
  m_GraphicsBinding.queueIndex = pVulkanDevice->GetGraphicsQueue().m_uiQueueIndex;

  ezLog::Info("OpenXR Vulkan binding: instance={}, physicalDevice={}, device={}, queueFamily={}, queueIndex={}",
    ezArgP(m_GraphicsBinding.instance),
    ezArgP(m_GraphicsBinding.physicalDevice),
    ezArgP(m_GraphicsBinding.device),
    m_GraphicsBinding.queueFamilyIndex,
    m_GraphicsBinding.queueIndex);

  // Validate: Check if OpenXR wants a specific physical device
  if (m_pfnGetVulkanGraphicsDeviceKHR)
  {
    VkPhysicalDevice xrPhysicalDevice = VK_NULL_HANDLE;
    XrResult res = m_pfnGetVulkanGraphicsDeviceKHR(instance, systemId, m_GraphicsBinding.instance, &xrPhysicalDevice);
    if (res == XR_SUCCESS)
    {
      if (xrPhysicalDevice != m_GraphicsBinding.physicalDevice)
      {
        const auto& ezDeviceProps = pVulkanDevice->GetPhysicalDeviceProperties();
        ezLog::Error("OpenXR physical device mismatch! OpenXR wants {}, but ezEngine is using {} ('{}')",
          ezArgP(xrPhysicalDevice), ezArgP(m_GraphicsBinding.physicalDevice), ezDeviceProps.deviceName.data());
      }
      else
      {
        ezLog::Info("OpenXR physical device matches ezEngine's device");
      }
    }
    else
    {
      ezLog::Warning("Failed to query OpenXR preferred physical device: {}", (int)res);
    }
  }

  // Log queue and device info
  const auto& graphicsQueue = pVulkanDevice->GetGraphicsQueue();
  ezLog::Info("Using graphics queue: family={}, index={}", graphicsQueue.m_uiQueueFamily, graphicsQueue.m_uiQueueIndex);

  const auto& deviceProps = pVulkanDevice->GetPhysicalDeviceProperties();
  ezLog::Info("Vulkan device '{}' API version: {}.{}.{}", deviceProps.deviceName.data(), VK_API_VERSION_MAJOR(deviceProps.apiVersion), VK_API_VERSION_MINOR(deviceProps.apiVersion), VK_API_VERSION_PATCH(deviceProps.apiVersion));

  return XR_SUCCESS;
}

void ezOpenXRGraphicsBindingVulkan::Deinitialize()
{
  m_GraphicsBinding.instance = VK_NULL_HANDLE;
  m_GraphicsBinding.physicalDevice = VK_NULL_HANDLE;
  m_GraphicsBinding.device = VK_NULL_HANDLE;
  m_GraphicsBinding.queueFamilyIndex = 0;
  m_GraphicsBinding.queueIndex = 0;
  CleanupSwapchainImages();
}

const void* ezOpenXRGraphicsBindingVulkan::GetGraphicsBinding() const
{
  return &m_GraphicsBinding;
}

XrResult ezOpenXRGraphicsBindingVulkan::SelectSwapchainFormats(XrSession session, bool bDepthComposition, int64_t& out_colorFormat, int64_t& out_depthFormat)
{
  // Enumerate available formats
  uint32_t formatCount;
  XR_SUCCEED_OR_RETURN_LOG(xrEnumerateSwapchainFormats(session, 0, &formatCount, nullptr));
  ezDynamicArray<int64_t> swapchainFormats;
  swapchainFormats.SetCountUninitialized(formatCount);
  XR_SUCCEED_OR_RETURN_LOG(xrEnumerateSwapchainFormats(session, formatCount, &formatCount, swapchainFormats.GetData()));

  // Build a set for fast lookup
  ezSet<int64_t> availableFormats;
  for (int64_t format : swapchainFormats)
  {
    availableFormats.Insert(format);
  }

  // Supported color formats in preference order
  constexpr VkFormat SupportedColorFormats[] = {
    VK_FORMAT_R8G8B8A8_SRGB,
    VK_FORMAT_B8G8R8A8_SRGB,
    VK_FORMAT_R8G8B8A8_UNORM,
    VK_FORMAT_B8G8R8A8_UNORM,
  };

  // Find matching color format
  out_colorFormat = 0;
  for (VkFormat colorFormat : SupportedColorFormats)
  {
    if (availableFormats.Contains(static_cast<int64_t>(colorFormat)))
    {
      out_colorFormat = static_cast<int64_t>(colorFormat);
      break;
    }
  }

  if (out_colorFormat == 0)
  {
    ezLog::Error("OpenXR Vulkan: No supported color swapchain format found");
    return XR_ERROR_INITIALIZATION_FAILED;
  }

  // Select depth format if needed
  if (bDepthComposition)
  {
    constexpr VkFormat SupportedDepthFormats[] = {
      VK_FORMAT_D32_SFLOAT,
      VK_FORMAT_D24_UNORM_S8_UINT,
      VK_FORMAT_D16_UNORM,
      VK_FORMAT_D32_SFLOAT_S8_UINT,
    };

    out_depthFormat = 0;
    for (VkFormat depthFormat : SupportedDepthFormats)
    {
      if (availableFormats.Contains(static_cast<int64_t>(depthFormat)))
      {
        out_depthFormat = static_cast<int64_t>(depthFormat);
        break;
      }
    }

    if (out_depthFormat == 0)
    {
      ezLog::Error("OpenXR Vulkan: No supported depth swapchain format found");
      return XR_ERROR_INITIALIZATION_FAILED;
    }
  }

  return XR_SUCCESS;
}

XrResult ezOpenXRGraphicsBindingVulkan::CreateSwapchainImages(XrSwapchain swapchainHandle, int64_t format, ezUInt32 imageCount, ezSizeU32 size, ezGALMSAASampleCount::Enum msaaCount, bool bIsDepth, ezGALDevice* pDevice, ezDynamicArray<ezGALTextureHandle>& out_textures)
{
  // Select the appropriate image storage
  auto& imageStorage = bIsDepth ? m_DepthSwapchainImages : m_ColorSwapchainImages;
  imageStorage.SetCount(imageCount, {XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR});

  // Enumerate swapchain images
  XrResult result = xrEnumerateSwapchainImages(swapchainHandle, imageCount, &imageCount, reinterpret_cast<XrSwapchainImageBaseHeader*>(imageStorage.GetData()));

  if (result != XR_SUCCESS)
  {
    ezLog::Error("OpenXR Vulkan: xrEnumerateSwapchainImages failed: {}", (int)result);
    return result;
  }

  // Create texture handles for each swapchain image
  for (ezUInt32 i = 0; i < imageCount; i++)
  {
    VkImage vkImage = imageStorage[i].image;

    ezGALTextureCreationDescription textureDesc;
    textureDesc.SetAsRenderTarget(size.width, size.height, ConvertTextureFormat(format), msaaCount);
    textureDesc.m_uiArraySize = 2;
    textureDesc.m_pExisitingNativeObject = (void*)vkImage;
    textureDesc.m_Type = ezGALTextureType::Texture2DArray;
    textureDesc.m_bAllowRenderTargetView = true;
    textureDesc.m_ResourceAccess.m_bImmutable = true;

    out_textures.PushBack(pDevice->CreateTexture(textureDesc, ezArrayPtr<ezGALSystemMemoryDescription>()));
  }

  return XR_SUCCESS;
}

ezGALResourceFormat::Enum ezOpenXRGraphicsBindingVulkan::ConvertTextureFormat(int64_t format) const
{
  switch (static_cast<VkFormat>(format))
  {
    case VK_FORMAT_D32_SFLOAT:
      return ezGALResourceFormat::DFloat;
    case VK_FORMAT_D16_UNORM:
      return ezGALResourceFormat::D16;
    case VK_FORMAT_D24_UNORM_S8_UINT:
      return ezGALResourceFormat::D24S8;
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
      return ezGALResourceFormat::D24S8; // Closest match
    case VK_FORMAT_B8G8R8A8_SRGB:
      return ezGALResourceFormat::BGRAUByteNormalizedsRGB;
    case VK_FORMAT_R8G8B8A8_SRGB:
      return ezGALResourceFormat::RGBAUByteNormalizedsRGB;
    case VK_FORMAT_B8G8R8A8_UNORM:
      return ezGALResourceFormat::BGRAUByteNormalized;
    case VK_FORMAT_R8G8B8A8_UNORM:
      return ezGALResourceFormat::RGBAUByteNormalized;
    default:
      ezLog::Warning("Unknown Vulkan format {} for OpenXR texture", static_cast<ezUInt32>(format));
      return ezGALResourceFormat::RGBAUByteNormalizedsRGB;
  }
}

void ezOpenXRGraphicsBindingVulkan::CleanupSwapchainImages()
{
  m_ColorSwapchainImages.Clear();
  m_DepthSwapchainImages.Clear();
}

#endif // EZ_OPENXR_HAS_VULKAN_RENDERER
