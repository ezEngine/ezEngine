#include <RendererDX11/RendererDX11PCH.h>

#include <Core/System/Window.h>
#include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Device/SwapChainDX11.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>

#include <Foundation/Platform/Win/Utils/HResultUtils.h>
#include <d3d11.h>

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
}

ezResult ezGALSwapChainDX11::UpdateSwapChain(ezGALDevice* pDevice, ezEnum<ezGALPresentMode> newPresentMode)
{
  ezGALDeviceDX11* pDXDevice = static_cast<ezGALDeviceDX11*>(pDevice);
  m_CurrentPresentMode = newPresentMode;
  DestroyBackBufferInternal(pDXDevice);

  // Need to flush dead objects or ResizeBuffers will fail as the backbuffer is still referenced.
  pDXDevice->FlushDeadObjects();

  HRESULT result = m_pDXSwapChain->ResizeBuffers(m_WindowDesc.m_bDoubleBuffered ? 2 : 1,
    m_WindowDesc.m_pWindow->GetClientAreaSize().width,
    m_WindowDesc.m_pWindow->GetClientAreaSize().height,
    pDXDevice->GetFormatLookupTable().GetFormatInfo(m_WindowDesc.m_BackBufferFormat).m_eRenderTarget,
    DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);

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

  SwapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT;

  if (FAILED(pDXDevice->GetDXGIFactory()->CreateSwapChain(pDXDevice->GetDXDevice(), &SwapChainDesc, &m_pDXSwapChain)))
  {
    return EZ_FAILURE;
  }

  // We have created a surface on a window, the window must not be destroyed while the surface is still alive.
  m_WindowDesc.m_pWindow->AddReference();

  m_bCanMakeDirectScreenshots = (SwapChainDesc.SwapEffect != DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL);
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
  TexDesc.m_bAllowRenderTargetView = true;
  TexDesc.m_Format = m_WindowDesc.m_BackBufferFormat;

  TexDesc.m_ResourceAccess.m_bImmutable = true;

  // And create the ez texture object wrapping the backbuffer texture
  m_hBackBufferTexture = pDXDevice->CreateTexture(TexDesc);
  EZ_ASSERT_RELEASE(!m_hBackBufferTexture.IsInvalidated(), "Couldn't create native backbuffer texture object!");

  // Create extra texture to be used as "practical backbuffer" if we can't do the screenshots the user wants.
  if (!m_bCanMakeDirectScreenshots)
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
    // Full screen swap chains must be switched to windowed mode before destruction.
    // See: https://msdn.microsoft.com/en-us/library/windows/desktop/bb205075(v=vs.85).aspx#Destroying
    m_pDXSwapChain->SetFullscreenState(FALSE, NULL);

    EZ_GAL_DX11_RELEASE(m_pDXSwapChain);

    m_WindowDesc.m_pWindow->RemoveReference();
  }
  return EZ_SUCCESS;
}
