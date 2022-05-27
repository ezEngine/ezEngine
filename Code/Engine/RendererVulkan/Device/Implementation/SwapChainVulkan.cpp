#include <RendererVulkan/RendererVulkanPCH.h>

#include <Core/System/Window.h>
#include <Foundation/Profiling/Profiling.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Device/SwapChainVulkan.h>
#include <RendererVulkan/Pools/SemaphorePoolVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Utils/CommandBufferUtilsVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

void ezGALSwapChainVulkan::AcquireNextRenderTarget(ezGALDevice* pDevice)
{
  EZ_PROFILE_SCOPE("AcquireNextRenderTarget");

  EZ_ASSERT_DEV(!m_currentPipelineImageAvailableSemaphore, "Pipeline semaphores leaked");
  m_currentPipelineImageAvailableSemaphore = ezSemaphorePoolVulkan::RequestSemaphore();

  if (m_swapChainImageInUseFences[m_uiCurrentSwapChainImage])
  {
    //#TODO_VULKAN waiting for fence does not seem to be necessary, is it already done by acquireNextImageKHR?
    //m_pVulkanDevice->GetVulkanDevice().waitForFences(1, &m_swapChainImageInUseFences[m_uiCurrentSwapChainImage], true, 1000000000ui64);
    m_swapChainImageInUseFences[m_uiCurrentSwapChainImage] = nullptr;
  }

  vk::Result result = m_pVulkanDevice->GetVulkanDevice().acquireNextImageKHR(m_vulkanSwapChain, std::numeric_limits<uint64_t>::max(), m_currentPipelineImageAvailableSemaphore, nullptr, &m_uiCurrentSwapChainImage);
  if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
  {
    ezLog::Warning("Swap-chain does not match the target window size and should be recreated.");
  }

  m_RenderTargets.m_hRTs[0] = m_swapChainTextures[m_uiCurrentSwapChainImage];
}

void ezGALSwapChainVulkan::PresentRenderTarget(ezGALDevice* pDevice)
{
  auto pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);
  {
    // Move image into ePresentSrcKHR layout.
    ezImageMemoryBarrierVulkan memmoryBarrier;
    {
      memmoryBarrier.old_layout = vk::ImageLayout::eColorAttachmentOptimal;
      memmoryBarrier.new_layout = vk::ImageLayout::ePresentSrcKHR;
      memmoryBarrier.src_access_mask = vk::AccessFlagBits::eColorAttachmentWrite;
      memmoryBarrier.src_stage_mask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
      memmoryBarrier.dst_stage_mask = vk::PipelineStageFlagBits::eBottomOfPipe;
    }
    auto view = ezGALDevice::GetDefaultDevice()->GetDefaultRenderTargetView(m_RenderTargets.m_hRTs[0]);
    ezCommandBufferUtilsVulkan::CmdImageMemoryBarrier(pVulkanDevice->GetCurrentCommandBuffer(), view, memmoryBarrier);
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

    m_swapChainImageInUseFences[m_uiCurrentSwapChainImage] = renderFence;
  }

  pVulkanDevice->ReclaimLater(currentPipelineRenderFinishedSemaphore);
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


  vk::Win32SurfaceCreateInfoKHR surfaceCreateInfo = {};
  surfaceCreateInfo.hinstance = GetModuleHandle(nullptr);
  surfaceCreateInfo.hwnd = (HWND)m_WindowDesc.m_pWindow->GetNativeWindowHandle();

  m_vulkanSurface = m_pVulkanDevice->GetVulkanInstance().createWin32SurfaceKHR(surfaceCreateInfo);

  if (!m_vulkanSurface)
  {
    ezLog::Error("Failed to create vulkan surface for window \"{}\"", m_WindowDesc.m_pWindow);
    return EZ_FAILURE;
  }

  uint32_t uiPresentModes = 0;
  VK_SUCCEED_OR_RETURN_EZ_FAILURE(m_pVulkanDevice->GetVulkanPhysicalDevice().getSurfacePresentModesKHR(m_vulkanSurface, &uiPresentModes, nullptr));
  ezHybridArray<vk::PresentModeKHR, 4> presentModes;
  presentModes.SetCount(uiPresentModes);
  VK_SUCCEED_OR_RETURN_EZ_FAILURE(m_pVulkanDevice->GetVulkanPhysicalDevice().getSurfacePresentModesKHR(m_vulkanSurface, &uiPresentModes, presentModes.GetData()));

  vk::SwapchainCreateInfoKHR swapChainCreateInfo = {};
  swapChainCreateInfo.clipped = VK_FALSE;
  swapChainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
  swapChainCreateInfo.imageArrayLayers = 1;
  swapChainCreateInfo.imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
  swapChainCreateInfo.imageExtent.width = m_WindowDesc.m_pWindow->GetClientAreaSize().width;
  swapChainCreateInfo.imageExtent.height = m_WindowDesc.m_pWindow->GetClientAreaSize().height;
  swapChainCreateInfo.imageFormat = m_pVulkanDevice->GetFormatLookupTable().GetFormatInfo(m_WindowDesc.m_BackBufferFormat).m_eRenderTarget;
  swapChainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
  if (m_WindowDesc.m_bAllowScreenshots)
    swapChainCreateInfo.imageUsage |= vk::ImageUsageFlagBits::eTransferSrc;

  swapChainCreateInfo.minImageCount = m_WindowDesc.m_bDoubleBuffered ? 2 : 1;
  swapChainCreateInfo.presentMode = ezConversionUtilsVulkan::GetPresentMode(m_WindowDesc.m_PresentMode, presentModes);
  swapChainCreateInfo.preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
  swapChainCreateInfo.surface = m_vulkanSurface;

  swapChainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
  swapChainCreateInfo.pQueueFamilyIndices = nullptr;
  swapChainCreateInfo.queueFamilyIndexCount = 0;

  m_vulkanSwapChain = m_pVulkanDevice->GetVulkanDevice().createSwapchainKHR(swapChainCreateInfo);

  // TODO screenshot functionality
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
    //{
    //  bool canMakeDirectScreenshots = (SwapChainDesc.SwapEffect != DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL);

    //  TexDesc.m_ResourceAccess.m_bReadBack = m_WindowDesc.m_bAllowScreenshots && canMakeDirectScreenshots;

    //  // And create the ez texture object wrapping the backbuffer texture
    //  m_hBackBufferTexture = pDXDevice->CreateTexture(TexDesc);
    //  EZ_ASSERT_RELEASE(!m_hBackBufferTexture.IsInvalidated(), "Couldn't create native backbuffer texture object!");

    //  // Create extra texture to be used as "practical backbuffer" if we can't do the screenshots the user wants.
    //  if (!canMakeDirectScreenshots && m_WindowDesc.m_bAllowScreenshots)
    //  {
    //    TexDesc.m_pExisitingNativeObject = nullptr;
    //    TexDesc.m_ResourceAccess.m_bReadBack = true;

    //    m_hActualBackBufferTexture = m_hBackBufferTexture;
    //    m_hBackBufferTexture = pDXDevice->CreateTexture(TexDesc);
    //    EZ_ASSERT_RELEASE(!m_hBackBufferTexture.IsInvalidated(), "Couldn't create non-native backbuffer texture object!");
    //  }
    //}
    m_pVulkanDevice->SetDebugName("SwapChainImage", m_swapChainImages[i]);

    ezGALTextureCreationDescription TexDesc;
    TexDesc.m_Format = m_WindowDesc.m_BackBufferFormat;
    TexDesc.m_uiWidth = m_WindowDesc.m_pWindow->GetClientAreaSize().width;
    TexDesc.m_uiHeight = m_WindowDesc.m_pWindow->GetClientAreaSize().height;
    TexDesc.m_SampleCount = m_WindowDesc.m_SampleCount;
    TexDesc.m_pExisitingNativeObject = m_swapChainImages[i];
    TexDesc.m_bAllowShaderResourceView = false;
    TexDesc.m_bCreateRenderTarget = true;
    TexDesc.m_ResourceAccess.m_bImmutable = true;
    TexDesc.m_ResourceAccess.m_bReadBack = m_WindowDesc.m_bAllowScreenshots;
    m_swapChainTextures.PushBack(m_pVulkanDevice->CreateTexture(TexDesc));
  }

  m_RenderTargets.m_hRTs[0] = m_swapChainTextures[0];
  return EZ_SUCCESS;
}

ezResult ezGALSwapChainVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  ezUInt32 uiSwapChainImages = m_swapChainTextures.GetCount();
  for (ezUInt32 i = 0; i < uiSwapChainImages; i++)
  {
    pDevice->DestroyTexture(m_swapChainTextures[i]);
  }
  m_swapChainTextures.Clear();

  ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);
  if (m_vulkanSwapChain)
  {
    pVulkanDevice->DeleteLater(m_vulkanSwapChain);
  }
  if (m_vulkanSurface)
  {

    pVulkanDevice->DeleteLater(m_vulkanSurface);
  }
  return EZ_SUCCESS;
}

EZ_STATICLINK_FILE(RendererVulkan, RendererVulkan_Device_Implementation_SwapChainVulkan);
