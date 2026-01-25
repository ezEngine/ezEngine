#include <OpenXRPlugin/OpenXRPluginPCH.h>

#include <OpenXRPlugin/Graphics/OpenXRGraphicsBindingVulkan.h>

#ifdef BUILDSYSTEM_ENABLE_VULKAN_SUPPORT

#  include <Foundation/Logging/Log.h>
#  include <OpenXRPlugin/OpenXRDeclarations.h>
#  include <OpenXRPlugin/OpenXRSingleton.h>
#  include <RendererFoundation/Device/Device.h>
#  include <RendererVulkan/Device/DeviceVulkan.h>

EZ_IMPLEMENT_SINGLETON(ezOpenXRGraphicsBindingVulkan);

namespace
{
  vk::Result AddExtIfSupported(ezStringView sExtensionName, const ezDynamicArray<vk::ExtensionProperties>& availableExtensions, ezDynamicArray<ezString>& ref_extensions)
  {
    auto it = std::find_if(begin(availableExtensions), end(availableExtensions), [&](const vk::ExtensionProperties& prop)
      { return sExtensionName == prop.extensionName.data(); });
    if (it != end(availableExtensions))
    {
      for (const char* existingExt : ref_extensions)
      {
        if (ezStringUtils::IsEqual(existingExt, it->extensionName))
        {
          return vk::Result::eSuccess;
        }
      }
      ref_extensions.PushBack(sExtensionName);
      return vk::Result::eSuccess;
    }
    ezLog::Error("OpenXR: The required Vulkan extension '{}' is not supported.", sExtensionName);
    return vk::Result::eErrorExtensionNotPresent;
  };
} // namespace

#  if VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1
namespace vk
{
  namespace detail
  {
    DispatchLoaderDynamic defaultDispatchLoaderDynamic;
  }
} // namespace vk

PFN_vkVoidFunction vkGetInstanceProcAddr(VkInstance instance, const char* pName)
{
  PFN_vkVoidFunction pFunc = vk::detail::defaultDispatchLoaderDynamic.vkGetInstanceProcAddr(instance, pName);
  return pFunc;
}
#  endif


ezOpenXRGraphicsBindingVulkan::ezOpenXRGraphicsBindingVulkan(ezOpenXR* pOpenXR)
  : m_SingletonRegistrar(this)
  , m_pOpenXR(pOpenXR)
{
}

ezOpenXRGraphicsBindingVulkan::~ezOpenXRGraphicsBindingVulkan()
{
  Deinitialize();
}

XrResult ezOpenXRGraphicsBindingVulkan::SelectExtension(ezDynamicArray<const char*>& extensions, const ezDynamicArray<XrExtensionProperties>& extensionProperties)
{
  // Hardcoded preference: set to false to prefer XR_KHR_vulkan_enable (v1) over v2
  // TODO: Make this configurable via settings
  constexpr bool bPreferV2 = false;

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

  if (!bFoundV1 && !bFoundV2)
  {
    ezLog::Error("OpenXR: Neither {} nor {} extension found", XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME, XR_KHR_VULKAN_ENABLE_EXTENSION_NAME);
    return XR_ERROR_EXTENSION_NOT_PRESENT;
  }

  // Try preferred extension first
  if (bPreferV2 && bFoundV2 || !bFoundV1)
  {
    extensions.PushBack(XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME);
    m_bUsingVulkanEnable2 = true;
    ezLog::Info("OpenXR: Enabled {} extension", XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME);
    return XR_SUCCESS;
  }

  extensions.PushBack(XR_KHR_VULKAN_ENABLE_EXTENSION_NAME);
  m_bUsingVulkanEnable2 = false;
  ezLog::Info("OpenXR: Enabled {} extension", XR_KHR_VULKAN_ENABLE_EXTENSION_NAME);
  return XR_SUCCESS;
}

void ezOpenXRGraphicsBindingVulkan::LoadFunctionPointers(XrInstance instance)
{
  if (m_bUsingVulkanEnable2)
  {
    // Load all function pointers for XR_KHR_vulkan_enable2
    xrGetInstanceProcAddr(instance, "xrCreateVulkanInstanceKHR", reinterpret_cast<PFN_xrVoidFunction*>(&m_pfnCreateVulkanInstanceKHR));
    xrGetInstanceProcAddr(instance, "xrCreateVulkanDeviceKHR", reinterpret_cast<PFN_xrVoidFunction*>(&m_pfnCreateVulkanDeviceKHR));
    xrGetInstanceProcAddr(instance, "xrGetVulkanGraphicsDevice2KHR", reinterpret_cast<PFN_xrVoidFunction*>(&m_pfnGetVulkanGraphicsDevice2KHR));
    xrGetInstanceProcAddr(instance, "xrGetVulkanGraphicsRequirements2KHR", reinterpret_cast<PFN_xrVoidFunction*>(&m_pfnGetVulkanGraphicsRequirements2KHR));
  }
  else
  {
    // Load all function pointers for XR_KHR_vulkan_enable (v1)
    xrGetInstanceProcAddr(instance, "xrGetVulkanInstanceExtensionsKHR", reinterpret_cast<PFN_xrVoidFunction*>(&m_pfnGetVulkanInstanceExtensionsKHR));
    xrGetInstanceProcAddr(instance, "xrGetVulkanDeviceExtensionsKHR", reinterpret_cast<PFN_xrVoidFunction*>(&m_pfnGetVulkanDeviceExtensionsKHR));
    xrGetInstanceProcAddr(instance, "xrGetVulkanGraphicsDeviceKHR", reinterpret_cast<PFN_xrVoidFunction*>(&m_pfnGetVulkanGraphicsDeviceKHR));
    xrGetInstanceProcAddr(instance, "xrGetVulkanGraphicsRequirementsKHR", reinterpret_cast<PFN_xrVoidFunction*>(&m_pfnGetVulkanGraphicsRequirementsKHR));
  }
}

XrResult ezOpenXRGraphicsBindingVulkan::Initialize(XrInstance instance, XrSystemId systemId, ezGALDevice* pDevice)
{
  ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);
  // Fill in the Vulkan graphics binding
  m_GraphicsBinding.type = XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR;
  m_GraphicsBinding.next = nullptr;
  m_GraphicsBinding.instance = pVulkanDevice->GetVulkanInstance();
  m_GraphicsBinding.physicalDevice = pVulkanDevice->GetVulkanPhysicalDevice();
  m_GraphicsBinding.device = pVulkanDevice->GetVulkanDevice();
  m_GraphicsBinding.queueFamilyIndex = pVulkanDevice->GetGraphicsQueue().m_uiQueueFamily;
  m_GraphicsBinding.queueIndex = pVulkanDevice->GetGraphicsQueue().m_uiQueueIndex;
  return XR_SUCCESS;
}

void ezOpenXRGraphicsBindingVulkan::Deinitialize()
{
  m_GraphicsBinding.instance = VK_NULL_HANDLE;
  m_GraphicsBinding.physicalDevice = VK_NULL_HANDLE;
  m_GraphicsBinding.device = VK_NULL_HANDLE;
  m_GraphicsBinding.queueFamilyIndex = 0;
  m_GraphicsBinding.queueIndex = 0;

  // Clear cached Vulkan handles (not owned, just references)
  m_VulkanInstance = nullptr;
  m_VulkanPhysicalDevice = nullptr;
  m_VulkanDevice = nullptr;

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

void ezOpenXRGraphicsBindingVulkan::ExtendInstanceExtensions(const ezDynamicArray<vk::ExtensionProperties>& availableExtensions, ezDynamicArray<ezString>& ref_extensions)
{
  if (m_bUsingVulkanEnable2)
  {
    // vulkan_enable2 doesn't require manually adding extensions
    return;
  }

  // For vulkan_enable (v1), query required instance extensions
  uint32_t extensionNamesSize = 0;
  XrResult result = m_pfnGetVulkanInstanceExtensionsKHR(m_pOpenXR->GetInstance(), m_pOpenXR->GetSystemId(), 0, &extensionNamesSize, nullptr);
  if (result != XR_SUCCESS || extensionNamesSize == 0)
  {
    ezLog::Warning("OpenXR: xrGetVulkanInstanceExtensionsKHR returned no extensions");
    return;
  }

  ezDynamicArray<char> extensionNames;
  extensionNames.SetCountUninitialized(extensionNamesSize);
  result = m_pfnGetVulkanInstanceExtensionsKHR(m_pOpenXR->GetInstance(), m_pOpenXR->GetSystemId(), extensionNamesSize, &extensionNamesSize, extensionNames.GetData());
  if (result != XR_SUCCESS)
  {
    ezLog::Error("OpenXR: xrGetVulkanInstanceExtensionsKHR failed: {}", (int)result);
    return;
  }

  // Parse space-separated extension names
  ezStringBuilder extensionString(ezStringView(extensionNames.GetData(), extensionNamesSize));
  ezHybridArray<ezStringView, 8> extensionList;
  extensionString.Split(false, extensionList, " ");

  for (const ezStringView& ext : extensionList)
  {
    AddExtIfSupported(ext, availableExtensions, ref_extensions);
  }
}

void ezOpenXRGraphicsBindingVulkan::ExtendDeviceExtensions(const ezDynamicArray<vk::ExtensionProperties>& availableExtensions, ezDynamicArray<ezString>& ref_extensions)
{
  if (m_bUsingVulkanEnable2)
  {
    // vulkan_enable2 doesn't require manually adding extensions
    return;
  }

  // For vulkan_enable (v1), query required device extensions
  if (!m_pfnGetVulkanDeviceExtensionsKHR)
  {
    ezLog::Error("OpenXR: xrGetVulkanDeviceExtensionsKHR function pointer not loaded");
    return;
  }

  uint32_t extensionNamesSize = 0;
  XrResult result = m_pfnGetVulkanDeviceExtensionsKHR(m_pOpenXR->GetInstance(), m_pOpenXR->GetSystemId(), 0, &extensionNamesSize, nullptr);
  if (result != XR_SUCCESS || extensionNamesSize == 0)
  {
    ezLog::Warning("OpenXR: xrGetVulkanDeviceExtensionsKHR returned no extensions");
    return;
  }

  ezDynamicArray<char> extensionNames;
  extensionNames.SetCountUninitialized(extensionNamesSize);
  result = m_pfnGetVulkanDeviceExtensionsKHR(m_pOpenXR->GetInstance(), m_pOpenXR->GetSystemId(), extensionNamesSize, &extensionNamesSize, extensionNames.GetData());
  if (result != XR_SUCCESS)
  {
    ezLog::Error("OpenXR: xrGetVulkanDeviceExtensionsKHR failed: {}", (int)result);
    return;
  }

  // Parse space-separated extension names
  ezStringBuilder extensionString(ezStringView(extensionNames.GetData(), extensionNamesSize));
  ezHybridArray<ezStringView, 8> extensionList;
  extensionString.Split(false, extensionList, " ");

  for (const ezStringView& ext : extensionList)
  {
    AddExtIfSupported(ext, availableExtensions, ref_extensions);
  }
}

vk::Instance ezOpenXRGraphicsBindingVulkan::CreateInstance(const vk::InstanceCreateInfo& createInfo)
{
  if (!m_bUsingVulkanEnable2)
  {
    if (!m_pfnGetVulkanGraphicsRequirementsKHR)
    {
      ezLog::Error("OpenXR: xrGetVulkanGraphicsRequirementsKHR function pointer not loaded");
      return nullptr;
    }

    XrGraphicsRequirementsVulkanKHR graphicsRequirements{XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR};
    graphicsRequirements.next = nullptr;
    XrResult result = m_pfnGetVulkanGraphicsRequirementsKHR(m_pOpenXR->GetInstance(), m_pOpenXR->GetSystemId(), &graphicsRequirements);
    if (result != XR_SUCCESS)
    {
      ezLog::Error("OpenXR: xrGetVulkanGraphicsRequirements2KHR failed: {}", (int)result);
      return nullptr;
    }
    return {};
  }
  if (m_pOpenXR->GetInstance() == XR_NULL_HANDLE || m_pOpenXR->GetSystemId() == XR_NULL_SYSTEM_ID)
  {
    ezLog::Error("OpenXR: XR context not set before Vulkan instance creation");
    return nullptr;
  }

#  if VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1
  vk::detail::defaultDispatchLoaderDynamic.init();
#  endif
  // Use XR_KHR_vulkan_enable2
  if (!m_pfnCreateVulkanInstanceKHR)
  {
    ezLog::Error("OpenXR: xrCreateVulkanInstanceKHR function pointer not loaded");
    return nullptr;
  }

  if (!m_pfnGetVulkanGraphicsRequirements2KHR)
  {
    ezLog::Error("OpenXR: xrGetVulkanGraphicsRequirements2KHR function pointer not loaded");
    return nullptr;
  }

  XrGraphicsRequirementsVulkan2KHR graphicsRequirements{XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN2_KHR};
  graphicsRequirements.next = nullptr;
  XrResult result = m_pfnGetVulkanGraphicsRequirements2KHR(m_pOpenXR->GetInstance(), m_pOpenXR->GetSystemId(), &graphicsRequirements);
  if (result != XR_SUCCESS)
  {
    ezLog::Error("OpenXR: xrGetVulkanGraphicsRequirements2KHR failed: {}", (int)result);
    return nullptr;
  }

  ezLog::Info("OpenXR Vulkan requirements: minApiVersion={}.{}.{}, maxApiVersion={}.{}.{}",
    VK_API_VERSION_MAJOR(graphicsRequirements.minApiVersionSupported),
    VK_API_VERSION_MINOR(graphicsRequirements.minApiVersionSupported),
    VK_API_VERSION_PATCH(graphicsRequirements.minApiVersionSupported),
    VK_API_VERSION_MAJOR(graphicsRequirements.maxApiVersionSupported),
    VK_API_VERSION_MINOR(graphicsRequirements.maxApiVersionSupported),
    VK_API_VERSION_PATCH(graphicsRequirements.maxApiVersionSupported));

  // Create Vulkan instance through OpenXR
  XrVulkanInstanceCreateInfoKHR xrCreateInfo{XR_TYPE_VULKAN_INSTANCE_CREATE_INFO_KHR};
  xrCreateInfo.next = nullptr;
  xrCreateInfo.systemId = m_pOpenXR->GetSystemId();
  xrCreateInfo.createFlags = 0;
  xrCreateInfo.pfnGetInstanceProcAddr = vkGetInstanceProcAddr;
  xrCreateInfo.vulkanCreateInfo = reinterpret_cast<const VkInstanceCreateInfo*>(&createInfo);
  xrCreateInfo.vulkanAllocator = nullptr;

  VkInstance vkInstance = VK_NULL_HANDLE;
  VkResult vkResult = VK_SUCCESS;

  XrResult xrResult = m_pfnCreateVulkanInstanceKHR(m_pOpenXR->GetInstance(), &xrCreateInfo, &vkInstance, &vkResult);
  if (xrResult != XR_SUCCESS)
  {
    ezLog::Error("OpenXR: xrCreateVulkanInstanceKHR failed with XrResult: {}", (int)xrResult);
    return nullptr;
  }

  if (vkResult != VK_SUCCESS)
  {
    ezLog::Error("OpenXR: xrCreateVulkanInstanceKHR returned VkResult: {}", (int)vkResult);
    return nullptr;
  }

  m_VulkanInstance = vk::Instance(vkInstance);
#  if VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1
  vk::detail::defaultDispatchLoaderDynamic.init(m_VulkanInstance);
#  endif
  ezLog::Info("OpenXR: Created Vulkan instance via xrCreateVulkanInstanceKHR (vulkan_enable2)");
  return m_VulkanInstance;
}

vk::PhysicalDevice ezOpenXRGraphicsBindingVulkan::GetPhysicalDevice(vk::Instance instance)
{
  if (m_bUsingVulkanEnable2)
  {
    // Use XR_KHR_vulkan_enable2
    if (!m_pfnGetVulkanGraphicsDevice2KHR)
    {
      ezLog::Error("OpenXR: xrGetVulkanGraphicsDevice2KHR function pointer not loaded");
      return nullptr;
    }

    if (!m_VulkanInstance)
    {
      ezLog::Error("OpenXR: Vulkan instance not created before getting physical device");
      return nullptr;
    }

    XrVulkanGraphicsDeviceGetInfoKHR getInfo{XR_TYPE_VULKAN_GRAPHICS_DEVICE_GET_INFO_KHR};
    getInfo.next = nullptr;
    getInfo.systemId = m_pOpenXR->GetSystemId();
    getInfo.vulkanInstance = static_cast<VkInstance>(instance);

    VkPhysicalDevice vkPhysicalDevice = VK_NULL_HANDLE;
    XrResult xrResult = m_pfnGetVulkanGraphicsDevice2KHR(m_pOpenXR->GetInstance(), &getInfo, &vkPhysicalDevice);
    if (xrResult != XR_SUCCESS)
    {
      ezLog::Error("OpenXR: xrGetVulkanGraphicsDevice2KHR failed: {}", (int)xrResult);
      return nullptr;
    }

    m_VulkanPhysicalDevice = vk::PhysicalDevice(vkPhysicalDevice);
    return m_VulkanPhysicalDevice;
  }
  else
  {
    // Use XR_KHR_vulkan_enable (v1)
    if (!m_pfnGetVulkanGraphicsDeviceKHR)
    {
      ezLog::Error("OpenXR: xrGetVulkanGraphicsDeviceKHR function pointer not loaded");
      return nullptr;
    }

#  if VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1
    vk::detail::defaultDispatchLoaderDynamic.init();
    vk::detail::defaultDispatchLoaderDynamic.init(m_VulkanInstance);
#  endif

    VkPhysicalDevice vkPhysicalDevice = VK_NULL_HANDLE;
    XrResult xrResult = m_pfnGetVulkanGraphicsDeviceKHR(m_pOpenXR->GetInstance(), m_pOpenXR->GetSystemId(), static_cast<VkInstance>(instance), &vkPhysicalDevice);
    if (xrResult != XR_SUCCESS)
    {
      ezLog::Error("OpenXR: xrGetVulkanGraphicsDeviceKHR failed: {}", (int)xrResult);
      return nullptr;
    }

    vk::PhysicalDevice physicalDevice = vk::PhysicalDevice(vkPhysicalDevice);
    return physicalDevice;
  }
}

vk::Device ezOpenXRGraphicsBindingVulkan::CreateDevice(const vk::DeviceCreateInfo& createInfo)
{
  if (!m_bUsingVulkanEnable2)
    return {};

  // Use XR_KHR_vulkan_enable2
  if (!m_pfnCreateVulkanDeviceKHR)
  {
    ezLog::Error("OpenXR: xrCreateVulkanDeviceKHR function pointer not loaded");
    return nullptr;
  }

  XrVulkanDeviceCreateInfoKHR xrCreateInfo{XR_TYPE_VULKAN_DEVICE_CREATE_INFO_KHR};
  xrCreateInfo.next = nullptr;
  xrCreateInfo.systemId = m_pOpenXR->GetSystemId();
  xrCreateInfo.createFlags = 0;
  xrCreateInfo.pfnGetInstanceProcAddr = vkGetInstanceProcAddr;
  xrCreateInfo.vulkanPhysicalDevice = static_cast<VkPhysicalDevice>(m_VulkanPhysicalDevice);
  xrCreateInfo.vulkanCreateInfo = reinterpret_cast<const VkDeviceCreateInfo*>(&createInfo);
  xrCreateInfo.vulkanAllocator = nullptr;

  VkDevice vkDevice = VK_NULL_HANDLE;
  VkResult vkResult = VK_SUCCESS;

  XrResult xrResult = m_pfnCreateVulkanDeviceKHR(m_pOpenXR->GetInstance(), &xrCreateInfo, &vkDevice, &vkResult);
  if (xrResult != XR_SUCCESS)
  {
    ezLog::Error("OpenXR: xrCreateVulkanDeviceKHR failed with XrResult: {}", (int)xrResult);
    return nullptr;
  }

  if (vkResult != VK_SUCCESS)
  {
    ezLog::Error("OpenXR: xrCreateVulkanDeviceKHR returned VkResult: {}", (int)vkResult);
    return nullptr;
  }

  m_VulkanDevice = vk::Device(vkDevice);
#  if VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1
  vk::detail::defaultDispatchLoaderDynamic.init(m_VulkanDevice);
#  endif
  ezLog::Info("OpenXR: Created Vulkan device via xrCreateVulkanDeviceKHR (vulkan_enable2)");
  return m_VulkanDevice;
}

#endif
