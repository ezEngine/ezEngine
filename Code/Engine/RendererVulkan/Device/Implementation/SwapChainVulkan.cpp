#include <RendererVulkan/RendererVulkanPCH.h>

#include <Core/System/Window.h>
#include <Foundation/Profiling/Profiling.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Device/SwapChainVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Utils/CommandBufferUtilsVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

void ezGALSwapChainVulkan::AcquireNextImage(vk::Semaphore imageAvailable) const
{
  EZ_PROFILE_SCOPE("AcquireNextImage");

  if (m_swapChainImageInUseFences[m_uiCurrentSwapChainImage])
  {
    //#TODO_VULKAN waiting for fence does not seem to be necessary, is it already done by acquireNextImageKHR?
    //m_pVulkanDevice->GetVulkanDevice().waitForFences(1, &m_swapChainImageInUseFences[m_uiCurrentSwapChainImage], true, 1000000000ui64);
    m_swapChainImageInUseFences[m_uiCurrentSwapChainImage] = nullptr;
  }

  vk::Result result = m_pVulkanDevice->GetVulkanDevice().acquireNextImageKHR(m_vulkanSwapChain, std::numeric_limits<uint64_t>::max(), imageAvailable, nullptr, &m_uiCurrentSwapChainImage);
  if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
  {
    ezLog::Warning("Swap-chain does not match the target window size and should be recreated.");
  }

  m_pVulkanDevice->ReplaceExisitingNativeObject(m_hBackBufferTexture, (void*)m_swapChainImages[m_uiCurrentSwapChainImage]).AssertSuccess();
}

void ezGALSwapChainVulkan::PresentMemoryBarrier(vk::CommandBuffer commandBuffer) const
{
  ezImageMemoryBarrierVulkan memmoryBarrier;
  {
    memmoryBarrier.old_layout = vk::ImageLayout::eColorAttachmentOptimal;
    memmoryBarrier.new_layout = vk::ImageLayout::ePresentSrcKHR;
    memmoryBarrier.src_access_mask = vk::AccessFlagBits::eColorAttachmentWrite;
    memmoryBarrier.src_stage_mask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    memmoryBarrier.dst_stage_mask = vk::PipelineStageFlagBits::eBottomOfPipe;
  }
  auto view = ezGALDevice::GetDefaultDevice()->GetDefaultRenderTargetView(m_hBackBufferTexture);
  ezCommandBufferUtilsVulkan::CmdImageMemoryBarrier(commandBuffer, view, memmoryBarrier);
}

void ezGALSwapChainVulkan::PresentNextImage(vk::Semaphore renderFinished, vk::Fence renderFinishedFence) const
{
  vk::PresentInfoKHR presentInfo;

  vk::Semaphore signalSemaphores[] = {renderFinished};

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  vk::SwapchainKHR swapChains[] = {m_vulkanSwapChain};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;

  presentInfo.pImageIndices = &m_uiCurrentSwapChainImage;

  m_pVulkanDevice->GetVulkanQueue().presentKHR(&presentInfo);

  m_swapChainImageInUseFences[m_uiCurrentSwapChainImage] = renderFinishedFence;
}

ezGALSwapChainVulkan::ezGALSwapChainVulkan(const ezGALSwapChainCreationDescription& Description)
  : ezGALSwapChain(Description)
  , m_vulkanSwapChain(nullptr)
{
}

ezGALSwapChainVulkan::~ezGALSwapChainVulkan() {}

EZ_DEFINE_AS_POD_TYPE(vk::PresentModeKHR);

ezResult ezGALSwapChainVulkan::InitPlatform(ezGALDevice* pDevice)
{
  m_pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);


  vk::Win32SurfaceCreateInfoKHR surfaceCreateInfo = {};
  surfaceCreateInfo.hinstance = GetModuleHandle(nullptr);
  surfaceCreateInfo.hwnd = (HWND)m_Description.m_pWindow->GetNativeWindowHandle();

  m_vulkanSurface = m_pVulkanDevice->GetVulkanInstance().createWin32SurfaceKHR(surfaceCreateInfo);

  if (!m_vulkanSurface)
  {
    ezLog::Error("Failed to create vulkan surface for window \"{}\"", m_Description.m_pWindow);
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
  swapChainCreateInfo.imageExtent.width = m_Description.m_pWindow->GetClientAreaSize().width;
  swapChainCreateInfo.imageExtent.height = m_Description.m_pWindow->GetClientAreaSize().height;
  swapChainCreateInfo.imageFormat = m_pVulkanDevice->GetFormatLookupTable().GetFormatInfo(m_Description.m_BackBufferFormat).m_eRenderTarget;
  swapChainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
  swapChainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
  swapChainCreateInfo.minImageCount = m_Description.m_bDoubleBuffered ? 2 : 1;
  swapChainCreateInfo.presentMode = ezConversionUtilsVulkan::GetPresentMode(m_Description.m_PresentMode);
  swapChainCreateInfo.preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
  swapChainCreateInfo.surface = m_vulkanSurface;

  ezArrayPtr<const ezUInt32> queueFamilyIndices = m_pVulkanDevice->GetQueueFamilyIndices();
  swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices.GetPtr();
  swapChainCreateInfo.queueFamilyIndexCount = queueFamilyIndices.GetCount();

  m_vulkanSwapChain = m_pVulkanDevice->GetVulkanDevice().createSwapchainKHR(swapChainCreateInfo);

  // TODO screenshot functionality
  if (!m_vulkanSwapChain)
  {
    ezLog::Error("Failed to create vulkan swap chain!");
    return EZ_FAILURE;
  }

  ezUInt32 uiSwapChainImages = 0;
  VK_SUCCEED_OR_RETURN_EZ_FAILURE(m_pVulkanDevice->GetVulkanDevice().getSwapchainImagesKHR(m_vulkanSwapChain, &uiSwapChainImages, nullptr));
  m_swapChainImages.SetCount(uiSwapChainImages);
  VK_SUCCEED_OR_RETURN_EZ_FAILURE(m_pVulkanDevice->GetVulkanDevice().getSwapchainImagesKHR(m_vulkanSwapChain, &uiSwapChainImages, m_swapChainImages.GetData()));

  EZ_ASSERT_DEV(uiSwapChainImages < 4, "If we have more than 3 swap chain images we can't hold ontp fences owned by ezDeviceVulkan::PerFrameData anymore as that reclaims all fences once it reuses the frame data (which is 4 right now). Thus, we can't safely pass in the fence in ezGALSwapChainVulkan::PresentNextImage as it will be reclaimed before we use it.");
  m_swapChainImageInUseFences.SetCount(uiSwapChainImages);

  for (ezUInt32 i = 0; i < uiSwapChainImages; i++)
  {
    //{
    //  bool canMakeDirectScreenshots = (SwapChainDesc.SwapEffect != DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL);

    //  TexDesc.m_ResourceAccess.m_bReadBack = m_Description.m_bAllowScreenshots && canMakeDirectScreenshots;

    //  // And create the ez texture object wrapping the backbuffer texture
    //  m_hBackBufferTexture = pDXDevice->CreateTexture(TexDesc);
    //  EZ_ASSERT_RELEASE(!m_hBackBufferTexture.IsInvalidated(), "Couldn't create native backbuffer texture object!");

    //  // Create extra texture to be used as "practical backbuffer" if we can't do the screenshots the user wants.
    //  if (!canMakeDirectScreenshots && m_Description.m_bAllowScreenshots)
    //  {
    //    TexDesc.m_pExisitingNativeObject = nullptr;
    //    TexDesc.m_ResourceAccess.m_bReadBack = true;

    //    m_hActualBackBufferTexture = m_hBackBufferTexture;
    //    m_hBackBufferTexture = pDXDevice->CreateTexture(TexDesc);
    //    EZ_ASSERT_RELEASE(!m_hBackBufferTexture.IsInvalidated(), "Couldn't create non-native backbuffer texture object!");
    //  }
    //}
  }

  ezGALTextureCreationDescription TexDesc;
  TexDesc.m_Format = m_Description.m_BackBufferFormat;
  TexDesc.m_uiWidth = m_Description.m_pWindow->GetClientAreaSize().width;
  TexDesc.m_uiHeight = m_Description.m_pWindow->GetClientAreaSize().height;
  TexDesc.m_SampleCount = m_Description.m_SampleCount;
  TexDesc.m_pExisitingNativeObject = m_swapChainImages[0];
  TexDesc.m_bAllowShaderResourceView = false;
  TexDesc.m_bCreateRenderTarget = true;
  TexDesc.m_ResourceAccess.m_bImmutable = true;
  m_hBackBufferTexture = m_pVulkanDevice->CreateTexture(TexDesc);

  return EZ_SUCCESS;
}

ezResult ezGALSwapChainVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  pDevice->DestroyTexture(m_hBackBufferTexture);
  m_hBackBufferTexture.Invalidate();

  if (!m_hActualBackBufferTexture.IsInvalidated())
  {
    pDevice->DestroyTexture(m_hActualBackBufferTexture);
    m_hActualBackBufferTexture.Invalidate();
  }

  ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);
  if (m_vulkanSwapChain)
  {
    pVulkanDevice->DeleteLater(m_vulkanSwapChain);
  }
  if (m_vulkanSurface)
  {

    pVulkanDevice->DeleteLater(m_vulkanSurface);
  }
  return ezGALSwapChain::DeInitPlatform(pDevice);
}

EZ_STATICLINK_FILE(RendererVulkan, RendererVulkan_Device_Implementation_SwapChainVulkan);
