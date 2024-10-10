#include <OpenXRPlugin/OpenXRPluginPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <OpenXRPlugin/OpenXRDeclarations.h>
#include <OpenXRPlugin/OpenXRSingleton.h>
#include <OpenXRPlugin/OpenXRSwapChain.h>
#include <RendererFoundation/Device/Device.h>
#include <vector>

void ezGALOpenXRSwapChain::AcquireNextRenderTarget(ezGALDevice* pDevice)
{
  EZ_PROFILE_SCOPE("AcquireNextRenderTarget");
  EZ_ASSERT_DEBUG(m_bImageAcquired == false, "PresentRenderTarget was not called.");
  m_bImageAcquired = true;
  auto pOpenXR = static_cast<ezOpenXR*>(m_pXrInterface);

  auto AquireAndWait = [](Swapchain& swapchain)
  {
    XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
    XR_SUCCEED_OR_RETURN_LOG(xrAcquireSwapchainImage(swapchain.handle, &acquireInfo, &swapchain.imageIndex));

    XrSwapchainImageWaitInfo waitInfo{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
    waitInfo.timeout = XR_INFINITE_DURATION;
    XR_SUCCEED_OR_RETURN_LOG(xrWaitSwapchainImage(swapchain.handle, &waitInfo));
    return XR_SUCCESS;
  };

  {
    EZ_PROFILE_SCOPE("AquireAndWait");
    AquireAndWait(m_ColorSwapchain);
    if (pOpenXR->GetDepthComposition())
      AquireAndWait(m_DepthSwapchain);
  }

  m_hColorRT = m_ColorRTs[m_ColorSwapchain.imageIndex];
  m_RenderTargets.m_hRTs[0] = m_hColorRT;
  if (pOpenXR->GetDepthComposition())
  {
    m_hDepthRT = m_DepthRTs[m_DepthSwapchain.imageIndex];
    m_RenderTargets.m_hDSTarget = m_hDepthRT;
  }
}

void ezGALOpenXRSwapChain::PresentRenderTarget(ezGALDevice* pDevice)
{
  EZ_PROFILE_SCOPE("PresentRenderTarget");
  auto pOpenXR = static_cast<ezOpenXR*>(m_pXrInterface);
  // If we have a companion view we can't let go of the current swap chain image yet as we need to use it as the input to render to the companion window.
  // Thus, we delay PresentRenderTarget here and ezOpenXR will call PresentRenderTarget instead after the companion window was updated.
  if (pOpenXR->m_pCompanion)
  {
    pOpenXR->DelayPresent();
  }
  else
  {
    PresentRenderTarget();
  }
  m_bImageAcquired = false;
}

void ezGALOpenXRSwapChain::PresentRenderTarget() const
{
  auto pOpenXR = static_cast<ezOpenXR*>(m_pXrInterface);
  EZ_PROFILE_SCOPE("xrReleaseSwapchainImage");
  XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
  XR_LOG_ERROR(xrReleaseSwapchainImage(m_ColorSwapchain.handle, &releaseInfo));
  if (pOpenXR->GetDepthComposition())
  {
    XR_LOG_ERROR(xrReleaseSwapchainImage(m_DepthSwapchain.handle, &releaseInfo));
  }
}

ezResult ezGALOpenXRSwapChain::InitPlatform(ezGALDevice* pDevice)
{
  if (InitSwapChain(m_MsaaCount) != XrResult::XR_SUCCESS)
    return EZ_FAILURE;

  m_RenderTargets.m_hRTs[0] = m_hColorRT;
  m_RenderTargets.m_hDSTarget = m_hDepthRT;
  return EZ_SUCCESS;
}

ezResult ezGALOpenXRSwapChain::DeInitPlatform(ezGALDevice* pDevice)
{
  DeinitSwapChain();
  return EZ_SUCCESS;
}

ezGALOpenXRSwapChain::ezGALOpenXRSwapChain(ezOpenXR* pXrInterface, ezGALMSAASampleCount::Enum msaaCount)
  : ezGALXRSwapChain(pXrInterface)
{
  m_pInstance = pXrInterface->GetInstance();
  m_SystemId = pXrInterface->GetSystemId();
  m_pSession = pXrInterface->GetSession();
  m_MsaaCount = msaaCount;
}

XrResult ezGALOpenXRSwapChain::SelectSwapchainFormat(int64_t& colorFormat, int64_t& depthFormat)
{
  auto pOpenXR = static_cast<ezOpenXR*>(m_pXrInterface);

  uint32_t swapchainFormatCount;
  XR_SUCCEED_OR_CLEANUP_LOG(xrEnumerateSwapchainFormats(m_pSession, 0, &swapchainFormatCount, nullptr), voidFunction);
  std::vector<int64_t> swapchainFormats(swapchainFormatCount);
  XR_SUCCEED_OR_CLEANUP_LOG(xrEnumerateSwapchainFormats(m_pSession, (uint32_t)swapchainFormats.size(), &swapchainFormatCount, swapchainFormats.data()), voidFunction);

  // List of supported color swapchain formats, in priority order.
  constexpr DXGI_FORMAT SupportedColorSwapchainFormats[] = {
    DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
    DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
  };

  constexpr DXGI_FORMAT SupportedDepthSwapchainFormats[] = {
    DXGI_FORMAT_D32_FLOAT,
    DXGI_FORMAT_D16_UNORM,
    DXGI_FORMAT_D24_UNORM_S8_UINT,
  };

  auto swapchainFormatIt = std::find_first_of(std::begin(SupportedColorSwapchainFormats), std::end(SupportedColorSwapchainFormats), swapchainFormats.begin(), swapchainFormats.end());
  if (swapchainFormatIt == std::end(SupportedColorSwapchainFormats))
  {
    return XrResult::XR_ERROR_INITIALIZATION_FAILED;
  }
  colorFormat = *swapchainFormatIt;

  if (pOpenXR->GetDepthComposition())
  {
    auto depthSwapchainFormatIt = std::find_first_of(std::begin(SupportedDepthSwapchainFormats), std::end(SupportedDepthSwapchainFormats), swapchainFormats.begin(), swapchainFormats.end());
    if (depthSwapchainFormatIt == std::end(SupportedDepthSwapchainFormats))
    {
      return XrResult::XR_ERROR_INITIALIZATION_FAILED;
    }
    depthFormat = *depthSwapchainFormatIt;
  }
  return XrResult::XR_SUCCESS;
}

XrResult ezGALOpenXRSwapChain::CreateSwapchainImages(Swapchain& swapchain, SwapchainType type)
{
  if (type == SwapchainType::Color)
  {
    m_ColorSwapChainImagesD3D11.SetCount(swapchain.imageCount, {XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR});
    swapchain.images = reinterpret_cast<XrSwapchainImageBaseHeader*>(m_ColorSwapChainImagesD3D11.GetData());
  }
  else
  {
    m_DepthSwapChainImagesD3D11.SetCount(swapchain.imageCount, {XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR});
    swapchain.images = reinterpret_cast<XrSwapchainImageBaseHeader*>(m_DepthSwapChainImagesD3D11.GetData());
  }
  XR_SUCCEED_OR_CLEANUP_LOG(xrEnumerateSwapchainImages(swapchain.handle, swapchain.imageCount, &swapchain.imageCount, swapchain.images), voidFunction);

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  for (ezUInt32 i = 0; i < swapchain.imageCount; i++)
  {
    ID3D11Texture2D* pTex = nullptr;
    if (type == SwapchainType::Color)
    {
      pTex = m_ColorSwapChainImagesD3D11[i].texture;
    }
    else
    {
      pTex = m_DepthSwapChainImagesD3D11[i].texture;
    }

    D3D11_TEXTURE2D_DESC backBufferDesc;
    pTex->GetDesc(&backBufferDesc);

    ezGALTextureCreationDescription textureDesc;
    textureDesc.SetAsRenderTarget(backBufferDesc.Width, backBufferDesc.Height, ezOpenXR::ConvertTextureFormat(swapchain.format), ezGALMSAASampleCount::Enum(backBufferDesc.SampleDesc.Count));
    textureDesc.m_uiArraySize = backBufferDesc.ArraySize;
    textureDesc.m_pExisitingNativeObject = pTex;
    // Need to add a ref as the EZ texture will always remove one on destruction.
    pTex->AddRef();
    if (type == SwapchainType::Color)
    {
      m_ColorRTs.PushBack(pDevice->CreateTexture(textureDesc));
    }
    else
    {
      m_DepthRTs.PushBack(pDevice->CreateTexture(textureDesc));
    }
  }
  if (type == SwapchainType::Color)
    m_hColorRT = m_ColorRTs[0];
  else
    m_hDepthRT = m_DepthRTs[0];
  return XR_SUCCESS;
}

XrResult ezGALOpenXRSwapChain::InitSwapChain(ezGALMSAASampleCount::Enum msaaCount)
{
  auto pOpenXR = static_cast<ezOpenXR*>(m_pXrInterface);

  // Read graphics properties for preferred swapchain length and logging.
  XrSystemProperties systemProperties{XR_TYPE_SYSTEM_PROPERTIES};
  XR_SUCCEED_OR_CLEANUP_LOG(xrGetSystemProperties(m_pInstance, m_SystemId, &systemProperties), DeinitSwapChain);

  ezUInt32 viewCount = 0;
  XR_SUCCEED_OR_CLEANUP_LOG(xrEnumerateViewConfigurationViews(m_pInstance, m_SystemId, pOpenXR->GetViewType(), 0, &viewCount, nullptr), DeinitSwapChain);
  if (viewCount != 2)
  {
    ezLog::Error("No stereo view configuration present, can't create swap chain");
    DeinitSwapChain();
    return XR_ERROR_INITIALIZATION_FAILED;
  }
  ezHybridArray<XrViewConfigurationView, 2> views;
  views.SetCount(viewCount, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
  XR_SUCCEED_OR_CLEANUP_LOG(xrEnumerateViewConfigurationViews(m_pInstance, m_SystemId, pOpenXR->GetViewType(), viewCount, &viewCount, views.GetData()), DeinitSwapChain);

  // Create the swapchain and get the images.
  // Select a swapchain format.
  m_PrimaryConfigView = views[0];
  XR_SUCCEED_OR_CLEANUP_LOG(SelectSwapchainFormat(m_ColorSwapchain.format, m_DepthSwapchain.format), DeinitSwapChain);

  // Create the swapchain.
  XrSwapchainCreateInfo swapchainCreateInfo{XR_TYPE_SWAPCHAIN_CREATE_INFO};
  swapchainCreateInfo.arraySize = 2;
  swapchainCreateInfo.format = m_ColorSwapchain.format;
  swapchainCreateInfo.width = m_PrimaryConfigView.recommendedImageRectWidth;
  swapchainCreateInfo.height = m_PrimaryConfigView.recommendedImageRectHeight;
  swapchainCreateInfo.mipCount = 1;
  swapchainCreateInfo.faceCount = 1;
  swapchainCreateInfo.sampleCount = (int)msaaCount;
  swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;

  m_CurrentSize = {swapchainCreateInfo.width, swapchainCreateInfo.height};

  auto CreateSwapChain = [this](const XrSwapchainCreateInfo& swapchainCreateInfo, Swapchain& swapchain, SwapchainType type) -> XrResult
  {
    XR_SUCCEED_OR_CLEANUP_LOG(xrCreateSwapchain(m_pSession, &swapchainCreateInfo, &swapchain.handle), voidFunction);
    XR_SUCCEED_OR_CLEANUP_LOG(xrEnumerateSwapchainImages(swapchain.handle, 0, &swapchain.imageCount, nullptr), voidFunction);
    CreateSwapchainImages(swapchain, type);

    return XrResult::XR_SUCCESS;
  };
  XR_SUCCEED_OR_CLEANUP_LOG(CreateSwapChain(swapchainCreateInfo, m_ColorSwapchain, SwapchainType::Color), DeinitSwapChain);

  if (pOpenXR->GetDepthComposition())
  {
    swapchainCreateInfo.format = m_DepthSwapchain.format;
    swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    XR_SUCCEED_OR_CLEANUP_LOG(CreateSwapChain(swapchainCreateInfo, m_DepthSwapchain, SwapchainType::Depth), DeinitSwapChain);
  }
  else
  {
    // Create depth buffer in case the API does not support it
    ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
    ezGALTextureCreationDescription tcd;
    tcd.m_Type = ezGALTextureType::Texture2DArray;
    tcd.SetAsRenderTarget(m_CurrentSize.width, m_CurrentSize.height, ezGALResourceFormat::DFloat, msaaCount);
    tcd.m_uiArraySize = 2;
    m_hDepthRT = pDevice->CreateTexture(tcd);
  }

  return XrResult::XR_SUCCESS;
}

void ezGALOpenXRSwapChain::DeinitSwapChain()
{
  auto pOpenXR = static_cast<ezOpenXR*>(m_pXrInterface);

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  for (ezGALTextureHandle rt : m_ColorRTs)
  {
    pDevice->DestroyTexture(rt);
  }
  m_ColorRTs.Clear();
  if (pOpenXR->GetDepthComposition())
  {
    for (ezGALTextureHandle rt : m_DepthRTs)
    {
      pDevice->DestroyTexture(rt);
    }
  }
  else
  {
    pDevice->DestroyTexture(m_hDepthRT);
    m_hDepthRT.Invalidate();
  }
  m_DepthRTs.Clear();
  m_hColorRT.Invalidate();
  m_hDepthRT.Invalidate();

  auto DeleteSwapchain = [](Swapchain& swapchain)
  {
    if (swapchain.handle != XR_NULL_HANDLE)
    {
      xrDestroySwapchain(swapchain.handle);
      swapchain.handle = 0;
    }
    swapchain.format = 0;
    swapchain.imageCount = 0;
    swapchain.images = nullptr;
    swapchain.imageIndex = 0;
  };
  m_PrimaryConfigView = {XR_TYPE_VIEW_CONFIGURATION_VIEW};
  DeleteSwapchain(m_ColorSwapchain);
  DeleteSwapchain(m_DepthSwapchain);

  m_ColorSwapChainImagesD3D11.Clear();
  m_DepthSwapChainImagesD3D11.Clear();
}
