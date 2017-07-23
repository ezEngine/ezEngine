#include <PCH.h>
#include <WindowsMixedReality/Graphics/MixedRealityDX11Device.h>
#include <WindowsMixedReality/Graphics/MixedRealitySwapChainDX11.h>
#include <WindowsMixedReality/HolographicSpace.h>

#include <RendererDX11/Context/ContextDX11.h>
#include <RendererDX11/Resources/RenderTargetViewDX11.h>

#include <RendererFoundation/Context/Context.h>
#include <RendererFoundation/Resources/Texture.h>

#include <d3d11.h>
#include <d3d11_1.h>
#include <dxgi1_4.h>
#include <windows.graphics.holographic.h>
#pragma warning (push)
#pragma warning (disable: 4467) // warning C4467: usage of ATL attributes is deprecated
#include <windows.graphics.directx.direct3d11.interop.h>
#pragma warning (pop)

ezGALMixedRealityDeviceDX11::ezGALMixedRealityDeviceDX11(const ezGALDeviceCreationDescription& Description)
  : ezGALDeviceDX11(Description)
  , m_bPresentedCurrentFrame(false)
{
}

ezGALMixedRealityDeviceDX11::~ezGALMixedRealityDeviceDX11()
{
}

ezResult ezGALMixedRealityDeviceDX11::InitPlatform()
{
  EZ_LOG_BLOCK("ezGALMixedRealityDeviceDX11::InitPlatform");

  ComPtr<ABI::Windows::Graphics::Holographic::IHolographicSpace> pHolographicSpace = ezWindowsHolographicSpace::GetSingleton()->GetInternalHolographicSpace();
  if (!pHolographicSpace)
  {
    ezLog::Error("Can't create holographic DX11 device since there is no holographic space.");
    return EZ_FAILURE;
  }

  // Find out which DXGI factory should be used for DX11 device creation.
  IDXGIAdapter3* pDXGIAdapter = nullptr;
  {
    ABI::Windows::Graphics::Holographic::HolographicAdapterId adapterID;
    EZ_HRESULT_TO_FAILURE_LOG(pHolographicSpace->get_PrimaryAdapterId(&adapterID));
    LUID id;
    id.HighPart = adapterID.HighPart;
    id.LowPart = adapterID.LowPart;

    // When a primary adapter ID is given to the app, the app should find the corresponding DXGI adapter and use it to create Direct3D devices
    // and device contexts. Otherwise, there is no restriction on the DXGI adapter the app can use.
    if (adapterID.HighPart != 0 && adapterID.LowPart != 0)
    {
      UINT createFlags = 0;
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
      createFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
      // Create the DXGI factory 4.
      ComPtr<IDXGIFactory1> pDxgiFactory;
      EZ_HRESULT_TO_FAILURE_LOG(CreateDXGIFactory2(createFlags, IID_PPV_ARGS(pDxgiFactory.GetAddressOf())));
      ComPtr<IDXGIFactory4> pDxgiFactory4;
      EZ_HRESULT_TO_FAILURE_LOG(pDxgiFactory.As(&pDxgiFactory4));

      // Retrieve the adapter specified by the holographic space.
      Microsoft::WRL::ComPtr<IDXGIAdapter3> m_pDxgiAdapter;
      EZ_HRESULT_TO_FAILURE_LOG(pDxgiFactory4->EnumAdapterByLuid(id, IID_PPV_ARGS(m_pDxgiAdapter.GetAddressOf())));
    }
  }

  // Enable BGRA support.
  // Without this, it won't be possible to set the device to a holographic space if one is around.
  //
  // Note that it is not entirely clear why this error comes up. The error message is just:
  // "Error: Call 'm_pHolographicSpace->SetDirect3D11Device(m_pDX11InteropDevice.Get())' failed with: The parameter is incorrect."
  // The documentation only mentions that D3D11_CREATE_DEVICE_BGRA_SUPPORT is necessary for Direct2D interop (which we're not relying on as of writing).
  DWORD dwFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

  // Proceed with normal device startup.
  if (ezGALDeviceDX11::InitPlatform(dwFlags, pDXGIAdapter).Failed())
    return EZ_FAILURE;


  // Create interop device.
  {
    // Retrieve WinRT DX11 device pointer.
    ComPtr<ABI::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice> pDX11DeviceWinRT;
    {
      ComPtr<ID3D11Device> pDX11DeviceWinAPI = GetDXDevice();

      // Acquire the DXGI interface for the Direct3D device.
      ComPtr<IDXGIDevice3> pDxgiDevice;
      EZ_HRESULT_TO_FAILURE_LOG(pDX11DeviceWinAPI.As(&pDxgiDevice));

      // Wrap the native device using a WinRT interop object.
      EZ_HRESULT_TO_FAILURE_LOG(CreateDirect3D11DeviceFromDXGIDevice(pDxgiDevice.Get(), &m_pDX11InteropDevice));
    }
  }

  // Inform holographic space.
  EZ_HRESULT_TO_FAILURE_LOG(pHolographicSpace->SetDirect3D11Device(m_pDX11InteropDevice.Get()));

  ezLog::Success("Successfully created DX11 device and set to holographic space!");

  return EZ_SUCCESS;
}

ezResult ezGALMixedRealityDeviceDX11::ShutdownPlatform()
{
  auto pHolographicSpace = ezWindowsHolographicSpace::GetSingleton()->GetInternalHolographicSpace();
  if (!pHolographicSpace)
  {
    ezLog::Error("Can't destroy holographic DX11 device since there is no holographic space.");
    return EZ_FAILURE;
  }

  pHolographicSpace->SetDirect3D11Device(nullptr);
  m_pDX11InteropDevice.Reset();

  return ShutdownPlatform();
}

ezGALSwapChain* ezGALMixedRealityDeviceDX11::CreateSwapChainPlatform(const ezGALSwapChainCreationDescription& Description)
{
  if (Description.m_pWindow != &ezGALMixedRealitySwapChainDX11::s_mockWindow)
  {
    ezLog::Error("It is not possible to manually create swap chains when using the holographic DX11 device. Swap chains are automatically created by holographic cameras.");
    return nullptr;
  }

  ezGALMixedRealitySwapChainDX11* pSwapChain = EZ_NEW(&m_Allocator, ezGALMixedRealitySwapChainDX11, Description);

  if (!pSwapChain->InitPlatform(this).Succeeded())
  {
    EZ_DELETE(&m_Allocator, pSwapChain);
    return nullptr;
  }

  return pSwapChain;
}

void ezGALMixedRealityDeviceDX11::DestroySwapChainPlatform(ezGALSwapChain* pSwapChain)
{
  ezGALMixedRealitySwapChainDX11* pSwapChainHoloDX11 = static_cast<ezGALMixedRealitySwapChainDX11*>(pSwapChain);
  pSwapChainHoloDX11->DeInitPlatform(this);
  EZ_DELETE(&m_Allocator, pSwapChainHoloDX11);
}

void ezGALMixedRealityDeviceDX11::PresentPlatform(ezGALSwapChain* pSwapChain)
{
  EZ_ASSERT_DEV(m_pCurrentHolographicFrame, "There is no holographic frame.");

  // More than one present needs to be ignored since everything or nothing is presented.
  if (m_bPresentedCurrentFrame)
  {
    static bool warned = false;
    if (!warned)
    {
      ezLog::Warning("ezGALMixedRealityDeviceDX11 needs to present all swap chains at once. Only first Present call has an effect, all others are ignored.");
      warned = true;
    }
    return;
  }

  //// Test
  //ezGALContext* pContext = GetPrimaryContext();
  //auto renderTarget = GetDefaultRenderTargetView(pSwapChain->GetBackBufferTexture());
  //ezGALRenderTagetSetup targetSetup;
  //targetSetup.SetRenderTarget(0, renderTarget);
  //pContext->SetRenderTargetSetup(targetSetup);
  //pContext->Clear(ezColorLinearUB(100, 149, 237, 255));


  // Presents frame and blocks until done.
  ABI::Windows::Graphics::Holographic::HolographicFramePresentResult presentResult;
  HRESULT result = m_pCurrentHolographicFrame->PresentUsingCurrentPrediction(&presentResult);
  if (FAILED(result))
  {
    ezLog::Error("Failed to present holographic frame: {1}", ezHRESULTtoString(result));
    return;
  }

  // Discard the contents of the render target.
  // This is a valid operation only when the existing contents will be entirely overwritten. If dirty or scroll rects are used, this call should be removed.
  {
    ID3D11DeviceContext* deviceContext = static_cast<ezGALContextDX11*>(GetPrimaryContext())->GetDXContext();
    ID3D11DeviceContext1* deviceContext1 = nullptr;
    if (FAILED(deviceContext->QueryInterface(&deviceContext1)))
    {
      ezLog::Error("Failed to query ID3D11DeviceContext1.");
      return;
    }

    auto backBuffer = pSwapChain->GetBackBufferTexture();
    if (!backBuffer.IsInvalidated())
    {
      const ezGALRenderTargetViewDX11* renderTargetView = static_cast<const ezGALRenderTargetViewDX11*>(GetRenderTargetView(GetDefaultRenderTargetView(backBuffer)));
      if (renderTargetView)
      {
        deviceContext1->DiscardView(renderTargetView->GetRenderTargetView());
      }
    }
  }

  // Apparently this is using a DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL in the background, so we need to force rebinding the render target to avoid this error:
  //
  // D3D11 WARNING: ID3D11DeviceContext::Draw: The Pixel Shader expects a Render Target View bound to slot 0, but the Render Target View was unbound during a call to Present.
  // A successful Present call for DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL SwapChains unbinds backbuffer 0 from all GPU writeable bind points.
  // [ EXECUTION WARNING #3146082: DEVICE_DRAW_RENDERTARGETVIEW_NOT_SET_DUE_TO_FLIP_PRESENT]
  //
  GetPrimaryContext()->SetRenderTargetSetup(ezGALRenderTagetSetup());


  // Device lost can occur!
  if (presentResult == ABI::Windows::Graphics::Holographic::HolographicFramePresentResult_DeviceRemoved)
  {
    // TODO: DEVICE LOST.
  }

  m_bPresentedCurrentFrame = true;
}

void ezGALMixedRealityDeviceDX11::BeginFramePlatform()
{
  EZ_ASSERT_DEV(!m_pCurrentHolographicFrame, "There is already a running holographic frame.");

  // TODO: Creating the holographic frame here might mean that all the updates before were done with more outdated camera poses than necessary.
  m_pCurrentHolographicFrame = ezWindowsHolographicSpace::GetSingleton()->StartNewHolographicFrame();

  m_bPresentedCurrentFrame = false;
}

void ezGALMixedRealityDeviceDX11::EndFramePlatform()
{
  EZ_ASSERT_DEV(m_pCurrentHolographicFrame, "There is no holographic frame.");
  m_pCurrentHolographicFrame.Reset();
}

