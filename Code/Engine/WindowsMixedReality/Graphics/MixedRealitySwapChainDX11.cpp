#includde <WindowsMixedRealityPCH.h>
#include <WindowsMixedReality/Graphics/MixedRealitySwapChainDX11.h>
#include <WindowsMixedReality/Graphics/MixedRealityDX11Device.h>
#include <RendererDX11/Resources/TextureDX11.h>

#include <d3d11.h>
#include <windows.graphics.holographic.h>
#pragma warning (push)
#pragma warning (disable: 4467) // warning C4467: usage of ATL attributes is deprecated
#include <windows.graphics.directx.direct3d11.interop.h>
#pragma warning (pop)

ezGALMixedRealitySwapChainDX11::ezHoloMockWindow ezGALMixedRealitySwapChainDX11::s_mockWindow;

ezGALMixedRealitySwapChainDX11::ezGALMixedRealitySwapChainDX11(const ezGALSwapChainCreationDescription& Description)
  : ezGALSwapChain(Description)
  //, m_reprojectionPlane()
{
}

ezGALMixedRealitySwapChainDX11::~ezGALMixedRealitySwapChainDX11()
{
}

ezResult ezGALMixedRealitySwapChainDX11::EnsureBackBufferResources(ezGALDevice* pDevice, ABI::Windows::Graphics::Holographic::IHolographicCameraRenderingParameters* parameters)
{
  // Retrieve backbuffer texture.
  ComPtr<ID3D11Texture2D> pBackBufferTexture;
  {
    ComPtr<ABI::Windows::Graphics::DirectX::Direct3D11::IDirect3DSurface> pSurface;
    EZ_HRESULT_TO_FAILURE_LOG(parameters->get_Direct3D11BackBuffer(pSurface.GetAddressOf()));

    ComPtr<Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess> pDxgiInterfaceAccess;
    EZ_HRESULT_TO_FAILURE_LOG(pSurface->QueryInterface(IID_PPV_ARGS(&pDxgiInterfaceAccess)));

    ComPtr<ID3D11Texture2D> pBackBufferResource;
    EZ_HRESULT_TO_FAILURE_LOG(pDxgiInterfaceAccess->GetInterface(IID_PPV_ARGS(pBackBufferResource.GetAddressOf())));

    EZ_HRESULT_TO_FAILURE_LOG(pBackBufferResource.As(&pBackBufferTexture));
  }

  // Determine wheather we need to create a new texture.
  bool createNewBuffer = true;
  if (!m_hBackBufferTexture.IsInvalidated())
  {
    auto pTexture = static_cast<const ezGALTextureDX11*>(pDevice->GetTexture(m_hBackBufferTexture));
    if (!pTexture || pTexture->GetDXTexture() != pBackBufferTexture.Get())
      pDevice->DestroyTexture(m_hBackBufferTexture);
    else
      createNewBuffer = false;
  }

  // Create new texture if necessary.
  if (createNewBuffer)
  {
    D3D11_TEXTURE2D_DESC backBufferDesc;
    pBackBufferTexture->GetDesc(&backBufferDesc);

    ezGALTextureCreationDescription textureDesc;
    textureDesc.SetAsRenderTarget(backBufferDesc.Width, backBufferDesc.Height, 
                                  ezGALResourceFormat::BGRAUByteNormalized, // This is what was shown on hololens emulator. Don't have a way yet to convert DGI -> ezGALResourceFormat 
                                  ezGALMSAASampleCount::None);   // Might in theory enforce MSAA, ignore that in the desc for now as well.
    textureDesc.m_uiArraySize = backBufferDesc.ArraySize;   // Should be either one or two, in accordance with ezWindowsMixedRealityCamera::IsStereoscopic
    textureDesc.m_pExisitingNativeObject = pBackBufferTexture.Get();

    m_hBackBufferTexture = pDevice->CreateTexture(textureDesc);

    ezLog::Info("Holographic swap chain has now a backbuffer with size {0}x{1}.", backBufferDesc.Width, backBufferDesc.Height);
  }

  return EZ_SUCCESS;
}

ezResult ezGALMixedRealitySwapChainDX11::InitPlatform(ezGALDevice* pDevice)
{
  return EZ_SUCCESS;
}

ezResult ezGALMixedRealitySwapChainDX11::DeInitPlatform(ezGALDevice* pDevice)
{
  return EZ_SUCCESS;
}

