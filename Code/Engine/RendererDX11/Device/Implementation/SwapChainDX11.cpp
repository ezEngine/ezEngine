#include <RendererDX11/RendererDX11PCH.h>

#include <Core/System/Window.h>
#include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Device/SwapChainDX11.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>

#include <Foundation/Basics/Platform/Win/HResultUtils.h>
#include <d3d11.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
#  include <Foundation/Basics/Platform/uwp/UWPUtils.h>
#  include <dxgi1_3.h>
#endif

void ezGALSwapChainDX11::AcquireNextRenderTarget(ezGALDevice* pDevice)
{
  EZ_IGNORE_UNUSED(pDevice);
}

void ezGALSwapChainDX11::PresentRenderTarget(ezGALDevice* pDevice)
{
  ezGALDeviceDX11* pDXDevice = static_cast<ezGALDeviceDX11*>(pDevice);

  // If there is a "actual backbuffer" (see it's documentation for detailed explanation), copy to it.
  if (!this->m_hActualBackBufferTexture.IsInvalidated())
  {
    pDXDevice->GetCommandEncoder()->CopyTexture(this->m_hActualBackBufferTexture, this->m_hBackBufferTexture);
  }

  HRESULT result = m_pDXSwapChain->Present(m_CurrentPresentMode == ezGALPresentMode::VSync ? 1 : 0, 0);
  if (FAILED(result))
  {
    ezLog::Error("Swap chain PresentImage failed with {0}", (ezUInt32)result);
    return;
  }

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
  // Since the swap chain can't be in discard mode, we do the discarding ourselves.
  ID3D11DeviceContext1* deviceContext1 = nullptr;
  if (FAILED(pDXDevice->GetDXImmediateContext()->QueryInterface(&deviceContext1)))
  {
    ezLog::Error("Failed to query ID3D11DeviceContext1.");
    return;
  }

  auto backBuffer = m_hBackBufferTexture;
  if (!backBuffer.IsInvalidated())
  {
    const ezGALRenderTargetViewDX11* renderTargetView = static_cast<const ezGALRenderTargetViewDX11*>(pDXDevice->GetRenderTargetView(pDXDevice->GetDefaultRenderTargetView(backBuffer)));
    if (renderTargetView)
    {
      deviceContext1->DiscardView(renderTargetView->GetRenderTargetView());
    }
  }
#endif
}

ezResult ezGALSwapChainDX11::UpdateSwapChain(ezGALDevice* pDevice, ezEnum<ezGALPresentMode> newPresentMode)
{
  ezGALDeviceDX11* pDXDevice = static_cast<ezGALDeviceDX11*>(pDevice);
  m_CurrentPresentMode = newPresentMode;
  DestroyBackBufferInternal(pDXDevice);

  // Need to flush dead objects or ResizeBuffers will fail as the backbuffer is still referenced.
  pDXDevice->FlushDeadObjects();

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
  HRESULT result = m_pDXSwapChain->ResizeBuffers(2, // Only double buffering supported.
    m_WindowDesc.m_pWindow->GetClientAreaSize().width,
    m_WindowDesc.m_pWindow->GetClientAreaSize().height,
    DXGI_FORMAT_R8G8B8A8_UNORM,
    0);
#else
  HRESULT result = m_pDXSwapChain->ResizeBuffers(m_WindowDesc.m_bDoubleBuffered ? 2 : 1,
    m_WindowDesc.m_pWindow->GetClientAreaSize().width,
    m_WindowDesc.m_pWindow->GetClientAreaSize().height,
    pDXDevice->GetFormatLookupTable().GetFormatInfo(m_WindowDesc.m_BackBufferFormat).m_eRenderTarget,
    DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
#endif
  if (FAILED(result))
  {
    ezLog::Error("UpdateSwapChain: ResizeBuffers call failed: {}", ezArgErrorCode(result));
    return EZ_FAILURE;
  }

  return CreateBackBufferInternal(pDXDevice);
}

ezGALSwapChainDX11::ezGALSwapChainDX11(const ezGALWindowSwapChainCreationDescription& Description)
  : ezGALWindowSwapChain(Description)

{
}

ezGALSwapChainDX11::~ezGALSwapChainDX11() = default;


ezResult ezGALSwapChainDX11::InitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceDX11* pDXDevice = static_cast<ezGALDeviceDX11*>(pDevice);
  m_CurrentPresentMode = m_WindowDesc.m_InitialPresentMode;
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
  DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {0};

  // Only double buffering supported.
  if (!m_WindowDesc.m_bDoubleBuffered)
    ezLog::Warning("Swap chain must be double buffered for UWP. Ignoring setting.");
  SwapChainDesc.BufferCount = 2;

  // Only allowed mode for UWP are the more efficient FLIP_SEQUENTIAL and the even better FLIP_DISCARD
  SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

  // SwapChainDesc.Format = pDXDevice->GetFormatLookupTable().GetFormatInfo(m_WindowDesc.m_BackBufferFormat).m_eRenderTarget;
  //
  // Can use only (DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R10G10B10A2_UNORM)
  // with DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL Note that this is also excluding SRGB formats. However, we can still set the rendertarget to
  // SRGB, see: https://software.intel.com/en-us/blogs/2013/06/03/full-screen-direct3d-games-using-borderless-windowed-mode
  //
  SwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  if (m_WindowDesc.m_BackBufferFormat != ezGALResourceFormat::RGBAUByteNormalized && m_WindowDesc.m_BackBufferFormat != ezGALResourceFormat::RGBAUByteNormalizedsRGB)
  {
    ezLog::Warning("Back buffer format for UWP can only be RGBAUByteNormalized or RGBAUByteNormalizedsRGB. Ignoring setting.");
  }

  SwapChainDesc.Width = m_WindowDesc.m_pWindow->GetClientAreaSize().width;
  SwapChainDesc.Height = m_WindowDesc.m_pWindow->GetClientAreaSize().height;
  SwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
  SwapChainDesc.Scaling = DXGI_SCALING_NONE;
  SwapChainDesc.Flags = 0;

  // Can't use multi sampling in UWP
  if (m_WindowDesc.m_SampleCount != 1)
    ezLog::Warning("Swap chain with multi sampling is not supported in UWP. Use an intermediate MSAA buffer instead. Ignoring setting.");
  SwapChainDesc.SampleDesc.Count = 1;
  SwapChainDesc.SampleDesc.Quality = 0;

#else

  DXGI_SWAP_CHAIN_DESC SwapChainDesc;
  SwapChainDesc.BufferCount = m_WindowDesc.m_bDoubleBuffered ? 2 : 1;
  SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; /// \todo The mode switch needs to be handled (ResizeBuffers + communication with engine)
  SwapChainDesc.SampleDesc.Count = m_WindowDesc.m_SampleCount;
  SwapChainDesc.SampleDesc.Quality = 0;                         /// \todo Get from MSAA value of the m_WindowDesc
  SwapChainDesc.OutputWindow = ezMinWindows::ToNative(m_WindowDesc.m_pWindow->GetNativeWindowHandle());
  SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;          // The FLIP models are more efficient but only supported in Win8+. See
                                                                // https://msdn.microsoft.com/en-us/library/windows/desktop/bb173077(v=vs.85).aspx#DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL
  SwapChainDesc.Windowed = m_WindowDesc.m_pWindow->IsFullscreenWindow(true) ? FALSE : TRUE;

  /// \todo Get from enumeration of available modes
  SwapChainDesc.BufferDesc.Format = pDXDevice->GetFormatLookupTable().GetFormatInfo(m_WindowDesc.m_BackBufferFormat).m_eRenderTarget;
  SwapChainDesc.BufferDesc.Width = m_WindowDesc.m_pWindow->GetClientAreaSize().width;
  SwapChainDesc.BufferDesc.Height = m_WindowDesc.m_pWindow->GetClientAreaSize().height;
  SwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
  SwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
  SwapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
  SwapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
#endif

  SwapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT;

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
  {
    ComPtr<IDXGIFactory1> dxgiFactory = pDXDevice->GetDXGIFactory();
    ComPtr<IDXGIFactory3> dxgiFactory3;
    EZ_HRESULT_TO_FAILURE_LOG(dxgiFactory.As(&dxgiFactory3));

    ComPtr<IDXGISwapChain1> swapChain1;
    ComPtr<IDXGISwapChain> swapChain;
    HRESULT result = dxgiFactory3->CreateSwapChainForCoreWindow(pDXDevice->GetDXDevice(), m_WindowDesc.m_pWindow->GetNativeWindowHandle(), &SwapChainDesc, nullptr, &swapChain1);
    if (FAILED(result))
    {
      if (result == E_ACCESSDENIED)
      {
        ezLog::Error("Failed to create swapchain: {0}. This may happen when the old swap chain is still in use. "
                     "Make sure all resources referencing the swap chain were destroyed, keeping in mind the 'Deferred Destruction' that "
                     "applies with FLIP swapchains. "
                     "https://msdn.microsoft.com/en-us/library/windows/desktop/ff476425(v=vs.85).aspx#Defer_Issues_with_Flip",
          result);
      }
      else
      {
        ezLog::Error("Failed to create swapchain: {0}", ezHRESULTtoString(result));
      }

      return EZ_FAILURE;
    }
    EZ_HRESULT_TO_FAILURE_LOG(swapChain1.As(&swapChain));
    m_pDXSwapChain = swapChain.Detach();
  }
#else
  if (FAILED(pDXDevice->GetDXGIFactory()->CreateSwapChain(pDXDevice->GetDXDevice(), &SwapChainDesc, &m_pDXSwapChain)))
  {
    return EZ_FAILURE;
  }
  else
#endif
  // We have created a surface on a window, the window must not be destroyed while the surface is still alive.
  m_WindowDesc.m_pWindow->AddReference();

  m_bCanMakeDirectScreenshots = (SwapChainDesc.SwapEffect != DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL);
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
  m_bCanMakeDirectScreenshots = m_bCanMakeDirectScreenshots && (SwapChainDesc.SwapEffect != DXGI_SWAP_EFFECT_FLIP_DISCARD);
#endif
  return CreateBackBufferInternal(pDXDevice);
}

ezResult ezGALSwapChainDX11::CreateBackBufferInternal(ezGALDeviceDX11* pDXDevice)
{
  // Get texture of the swap chain
  ID3D11Texture2D* pNativeBackBufferTexture = nullptr;
  HRESULT result = m_pDXSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pNativeBackBufferTexture));
  if (FAILED(result))
  {
    ezLog::Error("Couldn't access backbuffer texture of swapchain: {0}", ezHRESULTtoString(result));
    EZ_GAL_DX11_RELEASE(m_pDXSwapChain);

    return EZ_FAILURE;
  }

  ezGALTextureCreationDescription TexDesc;
  TexDesc.m_uiWidth = m_WindowDesc.m_pWindow->GetClientAreaSize().width;
  TexDesc.m_uiHeight = m_WindowDesc.m_pWindow->GetClientAreaSize().height;
  TexDesc.m_SampleCount = m_WindowDesc.m_SampleCount;
  TexDesc.m_pExisitingNativeObject = pNativeBackBufferTexture;
  TexDesc.m_bAllowShaderResourceView = false;
  TexDesc.m_bCreateRenderTarget = true;
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
  // See format handling in swap chain desc creation above.
  if (ezGALResourceFormat::IsSrgb(m_WindowDesc.m_BackBufferFormat))
    TexDesc.m_Format = ezGALResourceFormat::RGBAUByteNormalizedsRGB;
  else
    TexDesc.m_Format = ezGALResourceFormat::RGBAUByteNormalized;
#else
  TexDesc.m_Format = m_WindowDesc.m_BackBufferFormat;
#endif

  TexDesc.m_ResourceAccess.m_bImmutable = true;

  // And create the ez texture object wrapping the backbuffer texture
  m_hBackBufferTexture = pDXDevice->CreateTexture(TexDesc);
  EZ_ASSERT_RELEASE(!m_hBackBufferTexture.IsInvalidated(), "Couldn't create native backbuffer texture object!");

  // Create extra texture to be used as "practical backbuffer" if we can't do the screenshots the user wants.
  if (!m_bCanMakeDirectScreenshots && m_WindowDesc.m_bAllowScreenshots)
  {
    TexDesc.m_pExisitingNativeObject = nullptr;

    m_hActualBackBufferTexture = m_hBackBufferTexture;
    m_hBackBufferTexture = pDXDevice->CreateTexture(TexDesc);
    EZ_ASSERT_RELEASE(!m_hBackBufferTexture.IsInvalidated(), "Couldn't create non-native backbuffer texture object!");
  }

  m_RenderTargets.m_hRTs[0] = m_hBackBufferTexture;
  m_CurrentSize = ezSizeU32(TexDesc.m_uiWidth, TexDesc.m_uiHeight);
  return EZ_SUCCESS;
}

void ezGALSwapChainDX11::DestroyBackBufferInternal(ezGALDeviceDX11* pDXDevice)
{
  pDXDevice->DestroyTexture(m_hBackBufferTexture);
  m_hBackBufferTexture.Invalidate();

  if (!m_hActualBackBufferTexture.IsInvalidated())
  {
    pDXDevice->DestroyTexture(m_hActualBackBufferTexture);
    m_hActualBackBufferTexture.Invalidate();
  }

  m_RenderTargets.m_hRTs[0].Invalidate();
}

ezResult ezGALSwapChainDX11::DeInitPlatform(ezGALDevice* pDevice)
{
  DestroyBackBufferInternal(static_cast<ezGALDeviceDX11*>(pDevice));

  if (m_pDXSwapChain)
  {
#if EZ_DISABLED(EZ_PLATFORM_WINDOWS_UWP)
    // Full screen swap chains must be switched to windowed mode before destruction.
    // See: https://msdn.microsoft.com/en-us/library/windows/desktop/bb205075(v=vs.85).aspx#Destroying
    m_pDXSwapChain->SetFullscreenState(FALSE, NULL);
#endif

    EZ_GAL_DX11_RELEASE(m_pDXSwapChain);

    m_WindowDesc.m_pWindow->RemoveReference();
  }
  return EZ_SUCCESS;
}


