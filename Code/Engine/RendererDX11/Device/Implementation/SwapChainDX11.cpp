
#include <RendererDX11/PCH.h>
#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Device/SwapChainDX11.h>
#include <Foundation/Logging/Log.h>
#include <System/Window/Window.h>

#include <d3d11.h>

ezGALSwapChainDX11::ezGALSwapChainDX11(const ezGALSwapChainCreationDescription& Description)
  : ezGALSwapChain(Description)
{
}

ezGALSwapChainDX11::~ezGALSwapChainDX11()
{
}


ezResult ezGALSwapChainDX11::InitPlatform(ezGALDevice* pDevice)
{
  DXGI_SWAP_CHAIN_DESC SwapChainDesc;
  SwapChainDesc.BufferCount = m_Description.m_bDoubleBuffered ? 2 : 1;
  SwapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT;
  SwapChainDesc.OutputWindow = m_Description.m_pWindow->GetNativeWindowHandle();
  SwapChainDesc.SampleDesc.Count = m_Description.m_SampleCount; SwapChainDesc.SampleDesc.Quality = 0; /// \todo Get from MSAA value of the m_Description
  SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
  SwapChainDesc.Windowed = m_Description.m_bFullscreen ? FALSE : TRUE;
  SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; /// \todo The mode switch needs to be handled (ResizeBuffers + communication with engine)

  /// \todo Get from enumeration of available modes
  /// \todo (Find via format table)
  SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
  SwapChainDesc.BufferDesc.Width = m_Description.m_pWindow->GetClientAreaSize().width;
  SwapChainDesc.BufferDesc.Height = m_Description.m_pWindow->GetClientAreaSize().height;
  SwapChainDesc.BufferDesc.RefreshRate.Numerator = 60; SwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
  SwapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
  SwapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;


  ezGALDeviceDX11* pDXDevice = static_cast<ezGALDeviceDX11*>(pDevice);

  if (FAILED(pDXDevice->GetDXGIFactory()->CreateSwapChain(pDXDevice->GetDXDevice(), &SwapChainDesc, &m_pDXSwapChain)))
  {
    return EZ_FAILURE;
  }
  else
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
    TexDesc.m_uiWidth = SwapChainDesc.BufferDesc.Width;
    TexDesc.m_uiHeight = SwapChainDesc.BufferDesc.Height;
    TexDesc.m_Format = m_Description.m_BackBufferFormat;
    TexDesc.m_SampleCount = m_Description.m_SampleCount;
    TexDesc.m_pExisitingNativeObject = pNativeBackBufferTexture;
    TexDesc.m_bCreateRenderTarget = true;

    if (m_Description.m_bAllowScreenshots)
      TexDesc.m_ResourceAccess.m_bReadBack = true;

    // And create the ez texture object wrapping the backbuffer texture
    m_hBackBufferTexture = pDXDevice->CreateTexture(TexDesc, nullptr);
    EZ_ASSERT_RELEASE(!m_hBackBufferTexture.IsInvalidated(), "Couldn't create backbuffer texture object!");


    // Create rendertarget view
    ezGALRenderTargetViewCreationDescription RTViewDesc;
    RTViewDesc.m_bReadOnly = true;
    RTViewDesc.m_hTexture = m_hBackBufferTexture;
    RTViewDesc.m_RenderTargetType = ezGALRenderTargetType::Color;
    RTViewDesc.m_uiFirstSlice = 0;
    RTViewDesc.m_uiMipSlice = 0;
    RTViewDesc.m_uiSliceCount = 1;
    m_hBackBufferRTV = pDXDevice->CreateRenderTargetView(RTViewDesc);
    EZ_ASSERT_RELEASE(!m_hBackBufferRTV.IsInvalidated(), "Couldn't create backbuffer rendertarget view!");

    // Optionally create depth buffer texture.
    if (m_Description.m_bCreateDepthStencilBuffer)
    {
      ezGALTextureCreationDescription DepthStencilTexDesc;
      DepthStencilTexDesc.m_uiWidth = SwapChainDesc.BufferDesc.Width;
      DepthStencilTexDesc.m_uiHeight = SwapChainDesc.BufferDesc.Height;
      DepthStencilTexDesc.m_Format = m_Description.m_DepthStencilBufferFormat;
      DepthStencilTexDesc.m_SampleCount = m_Description.m_SampleCount;
      DepthStencilTexDesc.m_bCreateRenderTarget = true;
      m_hDepthStencilBufferTexture = pDXDevice->CreateTexture(DepthStencilTexDesc, nullptr);
      EZ_ASSERT_RELEASE(!m_hDepthStencilBufferTexture.IsInvalidated(), "Couldn't create depth-stencil texture object!");

      ezGALRenderTargetViewCreationDescription DepthStencilRTViewDesc;
      DepthStencilRTViewDesc.m_bReadOnly = false;
      DepthStencilRTViewDesc.m_hTexture = m_hDepthStencilBufferTexture;
      DepthStencilRTViewDesc.m_RenderTargetType = ezGALRenderTargetType::DepthStencil;
      m_hBackBufferDSV = pDXDevice->CreateRenderTargetView(DepthStencilRTViewDesc);
      EZ_ASSERT_RELEASE(!m_hBackBufferDSV.IsInvalidated(), "Couldn't create depth-stencil rendertarget view!");
    }

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

