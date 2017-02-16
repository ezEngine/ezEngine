
#include <PCH.h>
#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Device/SwapChainDX11.h>
#include <System/Window/Window.h>

#include <d3d11.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
  #include <Foundation/Basics/Platform/uwp/UWPUtils.h>
  #include <dxgi1_3.h>
#endif

ezGALSwapChainDX11::ezGALSwapChainDX11(const ezGALSwapChainCreationDescription& Description)
  : ezGALSwapChain(Description)
  , m_pDXSwapChain(nullptr)
{
}

ezGALSwapChainDX11::~ezGALSwapChainDX11()
{
}


ezResult ezGALSwapChainDX11::InitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceDX11* pDXDevice = static_cast<ezGALDeviceDX11*>(pDevice);

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
  DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = { 0 };

  // Only double buffering supported.
  if (!m_Description.m_bDoubleBuffered)
    ezLog::Warning("Swap chain must be double buffered for UWP. Ignoring setting.");
  SwapChainDesc.BufferCount = 2;

  // Only allowed mode for UWP.
  SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; 
  
  //SwapChainDesc.Format = pDXDevice->GetFormatLookupTable().GetFormatInfo(m_Description.m_BackBufferFormat).m_eRenderTarget;
  //
  // Can use only (DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R10G10B10A2_UNORM) with DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL
  // Note that this is also excluding SRGB formats. However, we can still set the rendertarget to SRGB, see:
  // https://software.intel.com/en-us/blogs/2013/06/03/full-screen-direct3d-games-using-borderless-windowed-mode
  //
  SwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  if (m_Description.m_BackBufferFormat != ezGALResourceFormat::RGBAUByteNormalized || 
      m_Description.m_BackBufferFormat != ezGALResourceFormat::RGBAUByteNormalizedsRGB)
  {
    ezLog::Warning("Back buffer format for UWP can only be RGBAUByteNormalized or RGBAUByteNormalizedsRGB. Ignoring setting.");
  }

  SwapChainDesc.Width = m_Description.m_pWindow->GetClientAreaSize().width;
  SwapChainDesc.Height = m_Description.m_pWindow->GetClientAreaSize().height;
  SwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
  SwapChainDesc.Scaling = DXGI_SCALING_NONE;
  SwapChainDesc.Flags = 0;

  // Can't use multi sampling in UWP
  if (m_Description.m_SampleCount != 1)
    ezLog::Warning("Swap chain with multi sampling is not supported in UWP. Use an intermediate MSAA buffer instead. Ignoring setting.");
  SwapChainDesc.SampleDesc.Count = 1;
  SwapChainDesc.SampleDesc.Quality = 0;

#else

  DXGI_SWAP_CHAIN_DESC SwapChainDesc;
  SwapChainDesc.BufferCount = m_Description.m_bDoubleBuffered ? 2 : 1;
  SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; /// \todo The mode switch needs to be handled (ResizeBuffers + communication with engine)
  SwapChainDesc.SampleDesc.Count = m_Description.m_SampleCount; SwapChainDesc.SampleDesc.Quality = 0; /// \todo Get from MSAA value of the m_Description
  SwapChainDesc.OutputWindow = m_Description.m_pWindow->GetNativeWindowHandle();
  SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
  SwapChainDesc.Windowed = m_Description.m_bFullscreen ? FALSE : TRUE;

  /// \todo Get from enumeration of available modes
  SwapChainDesc.BufferDesc.Format = pDXDevice->GetFormatLookupTable().GetFormatInfo(m_Description.m_BackBufferFormat).m_eRenderTarget;
  SwapChainDesc.BufferDesc.Width = m_Description.m_pWindow->GetClientAreaSize().width;
  SwapChainDesc.BufferDesc.Height = m_Description.m_pWindow->GetClientAreaSize().height;
  SwapChainDesc.BufferDesc.RefreshRate.Numerator = 60; SwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
  SwapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
  SwapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
#endif

  SwapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT;

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
  {
    ComPtr<IDXGIFactory1> dxgiFactory = pDXDevice->GetDXGIFactory();
    ComPtr<IDXGIFactory3> dxgiFactory3;
    EZ_SUCCEED_OR_RETURN(dxgiFactory.As(&dxgiFactory3));

    ComPtr<IDXGISwapChain1> swapChain1;
    ComPtr<IDXGISwapChain> swapChain;
    EZ_SUCCEED_OR_RETURN(dxgiFactory3->CreateSwapChainForCoreWindow(pDXDevice->GetDXDevice(), m_Description.m_pWindow->GetNativeWindowHandle(), &SwapChainDesc, nullptr, &swapChain1));
    EZ_SUCCEED_OR_RETURN(swapChain1.As(&swapChain));
    m_pDXSwapChain = swapChain.Detach();

	// TODO: Support fullscreen. Code below doesn't work.
    //HRESULT result = swapChain1->SetFullscreenState(m_Description.m_bFullscreen ? FALSE : TRUE, nullptr);
    //if (FAILED(result))
    //{
    //  // see https://msdn.microsoft.com/en-us/library/windows/desktop/bb174579%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396
    //  ezLog::Error("Failed enabling/disabling fullscreen: {0}", result);
    //}
  }
#else
  if (FAILED(pDXDevice->GetDXGIFactory()->CreateSwapChain(pDXDevice->GetDXDevice(), &SwapChainDesc, &m_pDXSwapChain)))
  {
    return EZ_FAILURE;
  }
  else
#endif
  {

    // Get texture of the swap chain
    ID3D11Texture2D* pNativeBackBufferTexture = nullptr;
    if (FAILED(m_pDXSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pNativeBackBufferTexture))))
    {
      ezLog::Error("Couldn't access backbuffer texture of swapchain!");
      EZ_GAL_DX11_RELEASE(m_pDXSwapChain);

      return EZ_FAILURE;
    }

    ezGALTextureCreationDescription TexDesc;
    TexDesc.m_uiWidth = m_Description.m_pWindow->GetClientAreaSize().width;
    TexDesc.m_uiHeight = m_Description.m_pWindow->GetClientAreaSize().height;
    TexDesc.m_SampleCount = m_Description.m_SampleCount;
    TexDesc.m_pExisitingNativeObject = pNativeBackBufferTexture;
    TexDesc.m_bAllowShaderResourceView = false;
    TexDesc.m_bCreateRenderTarget = true;
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
    // See format handling in swap chain desc creation above.
    if (ezGALResourceFormat::IsSrgb(m_Description.m_BackBufferFormat))
      TexDesc.m_Format = ezGALResourceFormat::RGBAUByteNormalizedsRGB;
    else
      TexDesc.m_Format = ezGALResourceFormat::RGBAUByteNormalized;
#else
    TexDesc.m_Format = m_Description.m_BackBufferFormat;
#endif

    if (m_Description.m_bAllowScreenshots)
      TexDesc.m_ResourceAccess.m_bReadBack = true;

    // And create the ez texture object wrapping the backbuffer texture
    m_hBackBufferTexture = pDXDevice->CreateTexture(TexDesc);
    EZ_ASSERT_RELEASE(!m_hBackBufferTexture.IsInvalidated(), "Couldn't create backbuffer texture object!");

    return EZ_SUCCESS;
  }
}

ezResult ezGALSwapChainDX11::DeInitPlatform(ezGALDevice* pDevice)
{
  EZ_GAL_DX11_RELEASE(m_pDXSwapChain);

  ezGALSwapChain::DeInitPlatform(pDevice);

  return EZ_SUCCESS;
}



EZ_STATICLINK_FILE(RendererDX11, RendererDX11_Device_Implementation_SwapChainDX11);

