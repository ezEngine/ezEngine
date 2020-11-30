#include <RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Device/SwapChainVulkan.h>
#include <Core/System/Window.h>

ezGALSwapChainVulkan::ezGALSwapChainVulkan(const ezGALSwapChainCreationDescription& Description)
  : ezGALSwapChain(Description)
  , m_vulkanSwapChain(nullptr)
{
}

ezGALSwapChainVulkan::~ezGALSwapChainVulkan() {}


ezResult ezGALSwapChainVulkan::InitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);

  vk::Win32SurfaceCreateInfoKHR surfaceCreateInfo = {};
  surfaceCreateInfo.hinstance = GetModuleHandle(nullptr);
  surfaceCreateInfo.hwnd = (HWND)m_Description.m_pWindow->GetNativeWindowHandle();

  m_vulkanSurface = pVulkanDevice->GetVulkanInstance().createWin32SurfaceKHR(surfaceCreateInfo);

  if (!m_vulkanSurface)
  {
    ezLog::Error("Failed to create vulkan surface for window \"{}\"", m_Description.m_pWindow);
    return EZ_FAILURE;
  }

  vk::SwapchainCreateInfoKHR swapChainCreateInfo = {};
  swapChainCreateInfo.clipped = VK_FALSE;
  swapChainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
  swapChainCreateInfo.imageArrayLayers = 1;
  swapChainCreateInfo.imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
  swapChainCreateInfo.imageExtent.width = m_Description.m_pWindow->GetClientAreaSize().width;
  swapChainCreateInfo.imageExtent.height = m_Description.m_pWindow->GetClientAreaSize().height;
  swapChainCreateInfo.imageFormat = pVulkanDevice->GetFormatLookupTable().GetFormatInfo(m_Description.m_BackBufferFormat).m_eRenderTarget;
  swapChainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
  swapChainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
  swapChainCreateInfo.minImageCount = m_Description.m_bDoubleBuffered ? 2 : 1;
  swapChainCreateInfo.presentMode = vk::PresentModeKHR::eMailbox;
  swapChainCreateInfo.preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
  swapChainCreateInfo.surface = m_vulkanSurface;

  ezArrayPtr<const ezUInt32> queueFamilyIndices = pVulkanDevice->GetQueueFamilyIndices();
  swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices.GetPtr();
  swapChainCreateInfo.queueFamilyIndexCount = queueFamilyIndices.GetCount();

  m_vulkanSwapChain = pVulkanDevice->GetVulkanDevice().createSwapchainKHR(swapChainCreateInfo);

  // TODO screenshot functionality
  if (!m_vulkanSwapChain)
  {
    ezLog::Error("Failed to create vulkan swap chain!");
    return EZ_FAILURE;
  }
  /*else
  {
    // Get texture of the swap chain
    ID3D11Texture2D* pNativeBackBufferTexture = nullptr;
    HRESULT result = m_pDXSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pNativeBackBufferTexture));
    if (FAILED(result))
    {
      ezLog::Error("Couldn't access backbuffer texture of swapchain: {0}", ezHRESULTtoString(result));
      EZ_GAL_Vulkan_RELEASE(m_pDXSwapChain);

      return EZ_FAILURE;
    }

    ezGALTextureCreationDescription TexDesc;
    TexDesc.m_uiWidth = m_Description.m_pWindow->GetClientAreaSize().width;
    TexDesc.m_uiHeight = m_Description.m_pWindow->GetClientAreaSize().height;
    TexDesc.m_SampleCount = m_Description.m_SampleCount;
    TexDesc.m_pExisitingNativeObject = pNativeBackBufferTexture;
    TexDesc.m_bAllowShaderResourceView = false;
    TexDesc.m_bCreateRenderTarget = true;
    TexDesc.m_ResourceAccess.m_bImmutable = true;

    bool canMakeDirectScreenshots = (SwapChainDesc.SwapEffect != DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL);

    TexDesc.m_ResourceAccess.m_bReadBack = m_Description.m_bAllowScreenshots && canMakeDirectScreenshots;

    // And create the ez texture object wrapping the backbuffer texture
    m_hBackBufferTexture = pDXDevice->CreateTexture(TexDesc);
    EZ_ASSERT_RELEASE(!m_hBackBufferTexture.IsInvalidated(), "Couldn't create native backbuffer texture object!");

    // Create extra texture to be used as "practical backbuffer" if we can't do the screenshots the user wants.
    if (!canMakeDirectScreenshots && m_Description.m_bAllowScreenshots)
    {
      TexDesc.m_pExisitingNativeObject = nullptr;
      TexDesc.m_ResourceAccess.m_bReadBack = true;

      m_hActualBackBufferTexture = m_hBackBufferTexture;
      m_hBackBufferTexture = pDXDevice->CreateTexture(TexDesc);
      EZ_ASSERT_RELEASE(!m_hBackBufferTexture.IsInvalidated(), "Couldn't create non-native backbuffer texture object!");
    }

    return EZ_SUCCESS;
  }*/

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
  pVulkanDevice->GetVulkanDevice().destroySwapchainKHR(m_vulkanSwapChain);
  m_vulkanSwapChain = nullptr;

  return ezGALSwapChain::DeInitPlatform(pDevice);
}



EZ_STATICLINK_FILE(RendererVulkan, RendererVulkan_Device_Implementation_SwapChainVulkan);
