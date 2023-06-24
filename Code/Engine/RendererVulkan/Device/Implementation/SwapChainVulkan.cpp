#include <RendererVulkan/RendererVulkanPCH.h>

#include <Core/System/Window.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/System/PlatformFeatures.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Device/SwapChainVulkan.h>
#include <RendererVulkan/Pools/SemaphorePoolVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>
#include <RendererVulkan/Utils/PipelineBarrierVulkan.h>

#if EZ_ENABLED(EZ_SUPPORTS_GLFW)
#  include <GLFW/glfw3.h>
#endif

#if EZ_ENABLED(EZ_PLATFORM_LINUX)
#  include <xcb/xcb.h>
#endif

namespace
{
  ezResult GetAlternativeFormat(vk::Format& format, vk::ComponentMapping& componentMapping)
  {
    switch (format)
    {
      case vk::Format::eR8G8B8A8Srgb:
        format = vk::Format::eB8G8R8A8Srgb;
        componentMapping = vk::ComponentMapping{vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eA};
        return EZ_SUCCESS;
      case vk::Format::eR8G8B8A8Unorm:
        format = vk::Format::eB8G8R8A8Unorm;
        componentMapping = vk::ComponentMapping{vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eA};
        return EZ_SUCCESS;
      default:
        return EZ_FAILURE;
    }
  }

  ezGALResourceFormat::Enum GetResourceFormat(vk::Format& format)
  {
    switch (format)
    {
      case vk::Format::eR8G8B8A8Srgb:
        return ezGALResourceFormat::RGBAUByteNormalizedsRGB;
      case vk::Format::eR8G8B8A8Unorm:
        return ezGALResourceFormat::RGBAUByteNormalized;
      case vk::Format::eB8G8R8A8Srgb:
        return ezGALResourceFormat::BGRAUByteNormalizedsRGB;
      case vk::Format::eB8G8R8A8Unorm:
        return ezGALResourceFormat::BGRAUByteNormalized;
      default:
        return ezGALResourceFormat::ENUM_COUNT;
    }
  }
} // namespace

void ezGALSwapChainVulkan::AcquireNextRenderTarget(ezGALDevice* pDevice)
{
  EZ_PROFILE_SCOPE("AcquireNextRenderTarget");

  EZ_ASSERT_DEV(!m_currentPipelineImageAvailableSemaphore, "Pipeline semaphores leaked");
  m_currentPipelineImageAvailableSemaphore = ezSemaphorePoolVulkan::RequestSemaphore();

  if (m_swapChainImageInUseFences[m_uiCurrentSwapChainImage])
  {
    // #TODO_VULKAN waiting for fence does not seem to be necessary, is it already done by acquireNextImageKHR?
    //  m_pVulkanDevice->GetVulkanDevice().waitForFences(1, &m_swapChainImageInUseFences[m_uiCurrentSwapChainImage], true, 1000000000ui64);
    m_swapChainImageInUseFences[m_uiCurrentSwapChainImage] = nullptr;
  }

  vk::Result result = m_pVulkanDevice->GetVulkanDevice().acquireNextImageKHR(m_vulkanSwapChain, std::numeric_limits<uint64_t>::max(), m_currentPipelineImageAvailableSemaphore, nullptr, &m_uiCurrentSwapChainImage);
  if (result == vk::Result::eErrorOutOfDateKHR)
  {
    ezLog::Warning("Swap-chain is incompatible with target window and will be recreated.");
    CreateSwapChainInternal().AssertSuccess("Failed to recreate swapchain");
    vk::Result result = m_pVulkanDevice->GetVulkanDevice().acquireNextImageKHR(m_vulkanSwapChain, std::numeric_limits<uint64_t>::max(), m_currentPipelineImageAvailableSemaphore, nullptr, &m_uiCurrentSwapChainImage);
    if (result == vk::Result::eErrorOutOfDateKHR)
    {
      ezLog::Error("Failed to acquire image from recreated swapchain.");
    }
  }
  else if (result == vk::Result::eSuboptimalKHR)
  {
    ezLog::Warning("Swap-chain does not match the target window size and should be recreated.");
  }

#ifdef VK_LOG_LAYOUT_CHANGES
  ezLog::Warning("AcquireNextRenderTarget {}", ezArgP(static_cast<void*>(m_swapChainImages[m_uiCurrentSwapChainImage])));
#endif

  m_RenderTargets.m_hRTs[0] = m_swapChainTextures[m_uiCurrentSwapChainImage];
}

void ezGALSwapChainVulkan::PresentRenderTarget(ezGALDevice* pDevice)
{
  EZ_PROFILE_SCOPE("PresentRenderTarget");

  auto pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);
  {
    auto view = ezGALDevice::GetDefaultDevice()->GetDefaultRenderTargetView(m_RenderTargets.m_hRTs[0]);
    const ezGALTextureVulkan* pTexture = static_cast<const ezGALTextureVulkan*>(pVulkanDevice->GetTexture(m_swapChainTextures[m_uiCurrentSwapChainImage]));
    // Move image into ePresentSrcKHR layout.
    pVulkanDevice->GetCurrentPipelineBarrier().EnsureImageLayout(pTexture, pTexture->GetFullRange(), vk::ImageLayout::ePresentSrcKHR, vk::PipelineStageFlagBits::eBottomOfPipe, {});
  }

  // Submit command buffer
  vk::Semaphore currentPipelineRenderFinishedSemaphore = ezSemaphorePoolVulkan::RequestSemaphore();
  vk::Fence renderFence = pVulkanDevice->Submit(m_currentPipelineImageAvailableSemaphore, vk::PipelineStageFlagBits::eColorAttachmentOutput, currentPipelineRenderFinishedSemaphore);
  pVulkanDevice->ReclaimLater(m_currentPipelineImageAvailableSemaphore);

  {
    // Present image
    vk::PresentInfoKHR presentInfo;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &currentPipelineRenderFinishedSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_vulkanSwapChain;
    presentInfo.pImageIndices = &m_uiCurrentSwapChainImage;

    m_pVulkanDevice->GetGraphicsQueue().m_queue.presentKHR(&presentInfo);

#ifdef VK_LOG_LAYOUT_CHANGES
    ezLog::Warning("PresentInfoKHR {}", ezArgP(m_swapChainImages[m_uiCurrentSwapChainImage]));
#endif

    m_swapChainImageInUseFences[m_uiCurrentSwapChainImage] = renderFence;
  }

  pVulkanDevice->ReclaimLater(currentPipelineRenderFinishedSemaphore);
}

ezResult ezGALSwapChainVulkan::UpdateSwapChain(ezGALDevice* pDevice, ezEnum<ezGALPresentMode> newPresentMode)
{
  EZ_ASSERT_DEBUG(!m_currentPipelineImageAvailableSemaphore, "UpdateSwapChain must not be called between AcquireNextRenderTarget and PresentRenderTarget.");
  m_currentPresentMode = newPresentMode;
  return CreateSwapChainInternal();
}

ezGALSwapChainVulkan::ezGALSwapChainVulkan(const ezGALWindowSwapChainCreationDescription& Description)
  : ezGALWindowSwapChain(Description)
  , m_vulkanSwapChain(nullptr)
{
}

ezGALSwapChainVulkan::~ezGALSwapChainVulkan() {}

ezResult ezGALSwapChainVulkan::InitPlatform(ezGALDevice* pDevice)
{
  m_pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);
  m_currentPresentMode = m_WindowDesc.m_InitialPresentMode;

#if defined(VK_USE_PLATFORM_WIN32_KHR)
  vk::Win32SurfaceCreateInfoKHR surfaceCreateInfo = {};
  surfaceCreateInfo.hinstance = GetModuleHandle(nullptr);
  surfaceCreateInfo.hwnd = (HWND)m_WindowDesc.m_pWindow->GetNativeWindowHandle();

  m_vulkanSurface = m_pVulkanDevice->GetVulkanInstance().createWin32SurfaceKHR(surfaceCreateInfo);
#elif EZ_ENABLED(EZ_PLATFORM_LINUX)
  ezWindowHandle windowHandle = m_WindowDesc.m_pWindow->GetNativeWindowHandle();
  switch (windowHandle.type)
  {
    case ezWindowHandle::Type::Invalid:
      ezLog::Error("Invalid native window handle for window \"{}\"", m_WindowDesc.m_pWindow);
      return EZ_FAILURE;
    case ezWindowHandle::Type::GLFW:
    {
      VkSurfaceKHR glfwSurface = VK_NULL_HANDLE;
      VK_SUCCEED_OR_RETURN_EZ_FAILURE(glfwCreateWindowSurface(m_pVulkanDevice->GetVulkanInstance(), windowHandle.glfwWindow, nullptr, &glfwSurface));
      m_vulkanSurface = glfwSurface;
    }
    break;
    case ezWindowHandle::Type::XCB:
    {
      vk::XcbSurfaceCreateInfoKHR surfaceCreateInfo = {};
      EZ_ASSERT_DEV(windowHandle.xcbWindow.m_pConnection != nullptr && windowHandle.xcbWindow.m_Window != 0, "Invalid xcb handle");
      surfaceCreateInfo.connection = windowHandle.xcbWindow.m_pConnection;
      surfaceCreateInfo.window = windowHandle.xcbWindow.m_Window;

      m_vulkanSurface = m_pVulkanDevice->GetVulkanInstance().createXcbSurfaceKHR(surfaceCreateInfo);
    }
    break;
  }
#elif EZ_ENABLED(EZ_SUPPORTS_GLFW)
  VkSurfaceKHR glfwSurface = VK_NULL_HANDLE;
  VK_SUCCEED_OR_RETURN_EZ_FAILURE(glfwCreateWindowSurface(m_pVulkanDevice->GetVulkanInstance(), m_WindowDesc.m_pWindow->GetNativeWindowHandle(), nullptr, &glfwSurface));
  m_vulkanSurface = glfwSurface;
#else
#  error Platform not supported
#endif

  if (!m_vulkanSurface)
  {
    ezLog::Error("Failed to create Vulkan surface for window \"{}\"", m_WindowDesc.m_pWindow);
    return EZ_FAILURE;
  }

  // We have created a surface on a window, the window must not be destroyed while the surface is still alive.
  m_WindowDesc.m_pWindow->AddReference();
  vk::Bool32 surfaceSupported = false;

  VK_SUCCEED_OR_RETURN_EZ_FAILURE(m_pVulkanDevice->GetVulkanPhysicalDevice().getSurfaceSupportKHR(m_pVulkanDevice->GetGraphicsQueue().m_uiQueueFamily, m_vulkanSurface, &surfaceSupported));

  if (!surfaceSupported)
  {
    ezLog::Error("Vulkan device does not support surfaces");
    m_pVulkanDevice->DeleteLater(m_vulkanSurface);
    return EZ_FAILURE;
  }

  return CreateSwapChainInternal();
}

ezResult ezGALSwapChainVulkan::CreateSwapChainInternal()
{
  uint32_t uiPresentModes = 0;
  VK_SUCCEED_OR_RETURN_EZ_FAILURE(m_pVulkanDevice->GetVulkanPhysicalDevice().getSurfacePresentModesKHR(m_vulkanSurface, &uiPresentModes, nullptr));
  ezHybridArray<vk::PresentModeKHR, 4> presentModes;
  presentModes.SetCount(uiPresentModes);
  VK_SUCCEED_OR_RETURN_EZ_FAILURE(m_pVulkanDevice->GetVulkanPhysicalDevice().getSurfacePresentModesKHR(m_vulkanSurface, &uiPresentModes, presentModes.GetData()));

  const vk::SurfaceCapabilitiesKHR surfaceCapabilities = m_pVulkanDevice->GetVulkanPhysicalDevice().getSurfaceCapabilitiesKHR(m_vulkanSurface);

  uint32_t uiNumSurfaceFormats = 0;
  VK_SUCCEED_OR_RETURN_EZ_FAILURE(m_pVulkanDevice->GetVulkanPhysicalDevice().getSurfaceFormatsKHR(m_vulkanSurface, &uiNumSurfaceFormats, nullptr));
  std::vector<vk::SurfaceFormatKHR> supportedFormats;
  supportedFormats.resize(uiNumSurfaceFormats);
  VK_SUCCEED_OR_RETURN_EZ_FAILURE(m_pVulkanDevice->GetVulkanPhysicalDevice().getSurfaceFormatsKHR(m_vulkanSurface, &uiNumSurfaceFormats, supportedFormats.data()));

  vk::Format desiredFormat = m_pVulkanDevice->GetFormatLookupTable().GetFormatInfo(m_WindowDesc.m_BackBufferFormat).m_format;
  vk::ColorSpaceKHR desiredColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
  vk::ComponentMapping backBufferComponentMapping;

  bool formatFound = false;
  for (vk::SurfaceFormatKHR& supportedFormat : supportedFormats)
  {
    if (supportedFormat.format == desiredFormat && supportedFormat.colorSpace == desiredColorSpace)
    {
      formatFound = true;
      break;
    }
  }

  if (!formatFound && GetAlternativeFormat(desiredFormat, backBufferComponentMapping).Succeeded())
  {
    for (vk::SurfaceFormatKHR& supportedFormat : supportedFormats)
    {
      if (supportedFormat.format == desiredFormat && supportedFormat.colorSpace == desiredColorSpace)
      {
        formatFound = true;
        break;
      }
    }
  }

  if (!formatFound)
  {
    ezStringBuilder backBufferFormatNice = "<unknown>";
    ezReflectionUtils::EnumerationToString(ezGetStaticRTTI<ezGALResourceFormat>(), m_WindowDesc.m_BackBufferFormat, backBufferFormatNice);
    ezLog::Error("The requested back buffer format {} mapping to the vulkan format {} is not supported on this system.", backBufferFormatNice, vk::to_string(desiredFormat).c_str());
    ezLog::Info("Available formats are:");
    for (vk::SurfaceFormatKHR& supportedFormat : supportedFormats)
    {
      ezLog::Info("  format: {}  color space: {}", vk::to_string(supportedFormat.format).c_str(), vk::to_string(supportedFormat.colorSpace).c_str());
    }
    return EZ_FAILURE;
  }

  // Does the device support RGBA textures or only BRGA?
  // Do we need to store somewhere if the texture is swizzeled?

  vk::SwapchainCreateInfoKHR swapChainCreateInfo = {};
  swapChainCreateInfo.clipped = VK_FALSE;
  swapChainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
  swapChainCreateInfo.imageArrayLayers = 1;
  swapChainCreateInfo.imageColorSpace = desiredColorSpace;
  swapChainCreateInfo.imageExtent.width = m_WindowDesc.m_pWindow->GetClientAreaSize().width;
  swapChainCreateInfo.imageExtent.height = m_WindowDesc.m_pWindow->GetClientAreaSize().height;
  swapChainCreateInfo.imageExtent.width = ezMath::Clamp(swapChainCreateInfo.imageExtent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
  swapChainCreateInfo.imageExtent.height = ezMath::Clamp(swapChainCreateInfo.imageExtent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
  swapChainCreateInfo.imageFormat = desiredFormat;

  swapChainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst; // We need eTransferDst to be able to resolve msaa textures into the backbuffer.
  if (m_WindowDesc.m_bAllowScreenshots)
    swapChainCreateInfo.imageUsage |= vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled;

  // #TODO_VULKAN Using only 2 images in the swapchain may trigger the following validation error when resizing the window. To prevent this we use 3 images instead. Technically m_bDoubleBuffered now means triple buffering - a problem for another time and creating a swapchain with only 1 texture is impossible anyways on most platforms.
  // https://vulkan.lunarg.com/doc/view/1.3.239.0/windows/1.3-extensions/vkspec.html#VUID-vkAcquireNextImageKHR-surface-07783
  swapChainCreateInfo.minImageCount = ezMath::Max(m_WindowDesc.m_bDoubleBuffered ? 3u : 2u, surfaceCapabilities.minImageCount);
  if (surfaceCapabilities.maxImageCount != 0)
    swapChainCreateInfo.minImageCount = ezMath::Min(swapChainCreateInfo.minImageCount, surfaceCapabilities.maxImageCount);

  swapChainCreateInfo.presentMode = ezConversionUtilsVulkan::GetPresentMode(m_currentPresentMode, presentModes);
  swapChainCreateInfo.preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
  swapChainCreateInfo.surface = m_vulkanSurface;

  swapChainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
  swapChainCreateInfo.pQueueFamilyIndices = nullptr;
  swapChainCreateInfo.queueFamilyIndexCount = 0;

  // We must pass in the old swap chain or NVidia will crash.
  swapChainCreateInfo.oldSwapchain = m_vulkanSwapChain;
  DestroySwapChainInternal(m_pVulkanDevice);

  m_vulkanSwapChain = m_pVulkanDevice->GetVulkanDevice().createSwapchainKHR(swapChainCreateInfo);

  if (!m_vulkanSwapChain)
  {
    ezLog::Error("Failed to create Vulkan swap chain!");
    return EZ_FAILURE;
  }

  ezUInt32 uiSwapChainImages = 0;
  VK_SUCCEED_OR_RETURN_EZ_FAILURE(m_pVulkanDevice->GetVulkanDevice().getSwapchainImagesKHR(m_vulkanSwapChain, &uiSwapChainImages, nullptr));
  m_swapChainImages.SetCount(uiSwapChainImages);
  VK_SUCCEED_OR_RETURN_EZ_FAILURE(m_pVulkanDevice->GetVulkanDevice().getSwapchainImagesKHR(m_vulkanSwapChain, &uiSwapChainImages, m_swapChainImages.GetData()));

  EZ_ASSERT_DEV(uiSwapChainImages < 4, "If we have more than 3 swap chain images we can't hold ontp fences owned by ezDeviceVulkan::PerFrameData anymore as that reclaims all fences once it reuses the frame data (which is 4 right now). Thus, we can't safely pass in the fence in ezGALSwapChainVulkan::PresentRenderTarget as it will be reclaimed before we use it.");
  m_swapChainImageInUseFences.SetCount(uiSwapChainImages);

  for (ezUInt32 i = 0; i < uiSwapChainImages; i++)
  {
    m_pVulkanDevice->SetDebugName("SwapChainImage", m_swapChainImages[i]);

    ezGALTextureCreationDescription TexDesc;
    TexDesc.m_Format = GetResourceFormat(desiredFormat);
    TexDesc.m_uiWidth = swapChainCreateInfo.imageExtent.width;
    TexDesc.m_uiHeight = swapChainCreateInfo.imageExtent.height;
    TexDesc.m_SampleCount = m_WindowDesc.m_SampleCount;
    TexDesc.m_pExisitingNativeObject = m_swapChainImages[i];
    TexDesc.m_bAllowShaderResourceView = false;
    TexDesc.m_bCreateRenderTarget = true;
    TexDesc.m_ResourceAccess.m_bImmutable = true;
    TexDesc.m_ResourceAccess.m_bReadBack = m_WindowDesc.m_bAllowScreenshots;
    m_swapChainTextures.PushBack(m_pVulkanDevice->CreateTextureInternal(TexDesc, ezArrayPtr<ezGALSystemMemoryDescription>(), desiredFormat));
  }
  m_CurrentSize = ezSizeU32(swapChainCreateInfo.imageExtent.width, swapChainCreateInfo.imageExtent.height);
  m_RenderTargets.m_hRTs[0] = m_swapChainTextures[0];
  return EZ_SUCCESS;
}

void ezGALSwapChainVulkan::DestroySwapChainInternal(ezGALDeviceVulkan* pVulkanDevice)
{
  ezUInt32 uiSwapChainImages = m_swapChainTextures.GetCount();
  for (ezUInt32 i = 0; i < uiSwapChainImages; i++)
  {
    pVulkanDevice->DestroyTexture(m_swapChainTextures[i]);
  }
  m_swapChainTextures.Clear();

  if (m_vulkanSwapChain)
  {
    pVulkanDevice->DeleteLater(m_vulkanSwapChain);
  }
}

ezResult ezGALSwapChainVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);
  DestroySwapChainInternal(pVulkanDevice);
  if (m_vulkanSurface)
  {
    pVulkanDevice->DeleteLater(m_vulkanSurface, (void*)m_WindowDesc.m_pWindow);
  }
  return EZ_SUCCESS;
}

EZ_STATICLINK_FILE(RendererVulkan, RendererVulkan_Device_Implementation_SwapChainVulkan);
