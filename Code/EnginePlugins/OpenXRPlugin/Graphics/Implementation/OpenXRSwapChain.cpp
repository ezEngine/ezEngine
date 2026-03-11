#include <OpenXRPlugin/OpenXRPluginPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <OpenXRPlugin/Graphics/OpenXRGraphicsBinding.h>
#include <OpenXRPlugin/Graphics/OpenXRSwapChain.h>
#include <OpenXRPlugin/OpenXRDeclarations.h>
#include <OpenXRPlugin/OpenXRSingleton.h>
#include <RendererFoundation/Device/Device.h>


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
  PresentRenderTarget();
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

XrResult ezGALOpenXRSwapChain::InitSwapChain(ezGALMSAASampleCount::Enum msaaCount)
{
  auto pOpenXR = static_cast<ezOpenXR*>(m_pXrInterface);
  ezOpenXRGraphicsBinding* pGraphicsBinding = pOpenXR->GetGraphicsBinding();

  if (!pGraphicsBinding)
  {
    ezLog::Error("No graphics binding available for swapchain creation");
    return XR_ERROR_INITIALIZATION_FAILED;
  }

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
  // Select a swapchain format using the graphics binding.
  m_PrimaryConfigView = views[0];
  XR_SUCCEED_OR_CLEANUP_LOG(pGraphicsBinding->SelectSwapchainFormats(m_pSession, pOpenXR->GetDepthComposition(), m_ColorSwapchain.format, m_DepthSwapchain.format), DeinitSwapChain);

  // Create the swapchain.
  XrSwapchainCreateInfo swapchainCreateInfo{XR_TYPE_SWAPCHAIN_CREATE_INFO};
  swapchainCreateInfo.arraySize = 2;
  swapchainCreateInfo.format = m_ColorSwapchain.format;
  swapchainCreateInfo.width = m_PrimaryConfigView.recommendedImageRectWidth;
  swapchainCreateInfo.height = m_PrimaryConfigView.recommendedImageRectHeight;
  swapchainCreateInfo.mipCount = 1;
  swapchainCreateInfo.faceCount = 1;
  swapchainCreateInfo.sampleCount = (int)msaaCount;
  swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_TRANSFER_SRC_BIT | XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;

  m_CurrentSize = {swapchainCreateInfo.width, swapchainCreateInfo.height};

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  // Create color swapchain
  {
    XR_SUCCEED_OR_CLEANUP_LOG(xrCreateSwapchain(m_pSession, &swapchainCreateInfo, &m_ColorSwapchain.handle), DeinitSwapChain);
    XR_SUCCEED_OR_CLEANUP_LOG(xrEnumerateSwapchainImages(m_ColorSwapchain.handle, 0, &m_ColorSwapchain.imageCount, nullptr), DeinitSwapChain);

    // Use graphics binding to create the swapchain images
    XrResult result = pGraphicsBinding->CreateSwapchainImages(m_ColorSwapchain.handle, m_ColorSwapchain.format, m_ColorSwapchain.imageCount, m_CurrentSize, msaaCount, false, pDevice, m_ColorRTs);

    if (result != XR_SUCCESS)
    {
      DeinitSwapChain();
      return result;
    }

    m_hColorRT = m_ColorRTs[0];
  }

  // Create depth swapchain if depth composition is enabled
  if (pOpenXR->GetDepthComposition())
  {
    swapchainCreateInfo.format = m_DepthSwapchain.format;
    swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_TRANSFER_SRC_BIT | XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;

    XR_SUCCEED_OR_CLEANUP_LOG(xrCreateSwapchain(m_pSession, &swapchainCreateInfo, &m_DepthSwapchain.handle), DeinitSwapChain);
    XR_SUCCEED_OR_CLEANUP_LOG(xrEnumerateSwapchainImages(m_DepthSwapchain.handle, 0, &m_DepthSwapchain.imageCount, nullptr), DeinitSwapChain);

    // Use graphics binding to create the swapchain images
    XrResult result = pGraphicsBinding->CreateSwapchainImages(m_DepthSwapchain.handle, m_DepthSwapchain.format, m_DepthSwapchain.imageCount, m_CurrentSize, msaaCount, true, pDevice, m_DepthRTs);

    if (result != XR_SUCCESS)
    {
      DeinitSwapChain();
      return result;
    }

    m_hDepthRT = m_DepthRTs[0];
  }
  else
  {
    // Create depth buffer in case the API does not support it
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

  // Destroy color render targets
  for (ezGALTextureHandle rt : m_ColorRTs)
  {
    pDevice->DestroyTexture(rt);
  }
  m_ColorRTs.Clear();

  // Destroy depth render targets
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

  // Destroy OpenXR swapchains
  auto DeleteSwapchain = [](Swapchain& swapchain)
  {
    if (swapchain.handle != XR_NULL_HANDLE)
    {
      xrDestroySwapchain(swapchain.handle);
      swapchain.handle = 0;
    }
    swapchain.format = 0;
    swapchain.imageCount = 0;
    swapchain.imageIndex = 0;
  };
  m_PrimaryConfigView = {XR_TYPE_VIEW_CONFIGURATION_VIEW};
  DeleteSwapchain(m_ColorSwapchain);
  DeleteSwapchain(m_DepthSwapchain);

  // Cleanup graphics binding's internal swapchain image storage
  ezOpenXRGraphicsBinding* pGraphicsBinding = pOpenXR->GetGraphicsBinding();
  if (pGraphicsBinding)
  {
    pGraphicsBinding->CleanupSwapchainImages();
  }
}
