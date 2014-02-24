
#include <RendererDX11/PCH.h>
#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Device/SwapChainDX11.h>
#include <RendererDX11/Shader/ShaderDX11.h>
#include <RendererDX11/Resources/BufferDX11.h>
#include <RendererDX11/Resources/TextureDX11.h>
#include <RendererDX11/Resources/RenderTargetConfigDX11.h>
#include <RendererDX11/Resources/RenderTargetViewDX11.h>
#include <RendererDX11/Shader/VertexDeclarationDX11.h>
#include <RendererDX11/State/StateDX11.h>
#include <RendererDX11/Resources/ResourceViewDX11.h>
#include <RendererDX11/Context/ContextDX11.h>

#include <Foundation/Logging/Log.h>
#include <System/Window/Window.h>
#include <Foundation/Math/Color.h>

#include <d3d11.h>

#if EZ_ENABLED(EZ_COMPILER_MSVC) && _MSC_VER < 1700
#warning "You need to setup the DirectX SDK paths to compile the renderer DX11 plugin currently!"
#endif

ezGALDeviceDX11::ezGALDeviceDX11(const ezGALDeviceCreationDescription& Description)
  : ezGALDevice(Description),
    m_pDevice(NULL),
    m_pDXGIFactory(NULL),
    m_pDXGIAdapter(NULL),
    m_pDXGIDevice(NULL)
{
}

ezGALDeviceDX11::~ezGALDeviceDX11()
{
}


// Init & shutdown functions

ezResult ezGALDeviceDX11::InitPlatform()
{
  EZ_LOG_BLOCK("ezGALDeviceDX11::InitPlatform()");

  DWORD dwFlags = 0;
  
  if(m_Description.m_bDebugDevice)
    dwFlags |= D3D11_CREATE_DEVICE_DEBUG;

  D3D_FEATURE_LEVEL FeatureLevel = D3D_FEATURE_LEVEL_11_0; // TODO

  ID3D11DeviceContext* pImmediateContext = NULL;

  if(FAILED(D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, dwFlags, &FeatureLevel, 1, D3D11_SDK_VERSION, &m_pDevice, NULL, &pImmediateContext)))
  {
    ezLog::Error("D3D11CreateDevice() failed!");
    return EZ_FAILURE;
  }

  // Create primary context object
  m_pPrimaryContext = EZ_DEFAULT_NEW(ezGALContextDX11)(this, pImmediateContext);
  EZ_ASSERT(m_pPrimaryContext != NULL, "Couldn't create primary context!");

  if(FAILED(m_pDevice->QueryInterface(__uuidof(IDXGIDevice1), (void **)&m_pDXGIDevice)))
  {
    ezLog::Error("Couldn't get the DXGIDevice1 interface of the D3D11 device - this may happen when running on Windows Vista without SP2 installed!");
    return EZ_FAILURE;
  }
      
  if(FAILED(m_pDXGIDevice->GetParent(__uuidof(IDXGIAdapter), (void **)&m_pDXGIAdapter)))
  {
    return EZ_FAILURE;
  }

  if(FAILED(m_pDXGIAdapter->GetParent(__uuidof(IDXGIFactory1), (void **)&m_pDXGIFactory)))
  {
    return EZ_FAILURE;
  }

  // Fill lookup table
  FillFormatLookupTable();

  // TODO: Get features of the device (depending on feature level, CheckFormat* functions etc.)

  return EZ_SUCCESS;
}

ezResult ezGALDeviceDX11::ShutdownPlatform()
{
  EZ_DEFAULT_DELETE(m_pPrimaryContext);

  EZ_GAL_DX11_RELEASE(m_pDevice);
  EZ_GAL_DX11_RELEASE(m_pDXGIFactory);
  EZ_GAL_DX11_RELEASE(m_pDXGIAdapter);
  EZ_GAL_DX11_RELEASE(m_pDXGIDevice);

  return EZ_SUCCESS;
}


// State creation functions

ezGALBlendState* ezGALDeviceDX11::CreateBlendStatePlatform(const ezGALBlendStateCreationDescription& Description)
{
  return NULL;
}

void ezGALDeviceDX11::DestroyBlendStatePlatform(ezGALBlendState* pBlendState)
{
}

ezGALDepthStencilState* ezGALDeviceDX11::CreateDepthStencilStatePlatform(const ezGALDepthStencilStateCreationDescription& Description)
{
  return NULL;
}

void ezGALDeviceDX11::DestroyDepthStencilStatePlatform(ezGALDepthStencilState* pDepthStencilState)
{
}

ezGALRasterizerState* ezGALDeviceDX11::CreateRasterizerStatePlatform(const ezGALRasterizerStateCreationDescription& Description)
{
  ezGALRasterizerStateDX11* pDX11RasterizerState = EZ_DEFAULT_NEW(ezGALRasterizerStateDX11)(Description);

  if(pDX11RasterizerState->InitPlatform(this))
  {
    return pDX11RasterizerState;
  }
  else
  {
    EZ_DEFAULT_DELETE(pDX11RasterizerState);
    return NULL;
  }
}

void ezGALDeviceDX11::DestroyRasterizerStatePlatform(ezGALRasterizerState* pRasterizerState)
{
  ezGALRasterizerStateDX11* pDX11RasterizerState = static_cast<ezGALRasterizerStateDX11*>(pRasterizerState);
  pDX11RasterizerState->DeInitPlatform(this);
  EZ_DEFAULT_DELETE(pDX11RasterizerState);
}

ezGALSamplerState* ezGALDeviceDX11::CreateSamplerStatePlatform(const ezGALSamplerStateCreationDescription& Description)
{
  ezGALSamplerStateDX11* pDX11SamplerState = EZ_DEFAULT_NEW(ezGALSamplerStateDX11)(Description);

  if (pDX11SamplerState->InitPlatform(this))
  {
    return pDX11SamplerState;
  }
  else
  {
    EZ_DEFAULT_DELETE(pDX11SamplerState);
    return NULL;
  }
}

void ezGALDeviceDX11::DestroySamplerStatePlatform(ezGALSamplerState* pSamplerState)
{
  ezGALSamplerStateDX11* pDX11SamplerState = static_cast<ezGALSamplerStateDX11*>(pSamplerState);
  pDX11SamplerState->DeInitPlatform(this);
  EZ_DEFAULT_DELETE(pDX11SamplerState);
}


// Resource creation functions

ezGALShader* ezGALDeviceDX11::CreateShaderPlatform(const ezGALShaderCreationDescription& Description)
{
  ezGALShaderDX11* pShader = EZ_DEFAULT_NEW(ezGALShaderDX11)(Description);

  if(!pShader->InitPlatform(this).IsSuccess())
  {
    EZ_DEFAULT_DELETE(pShader);
    return NULL;
  }

  return pShader;
}

void ezGALDeviceDX11::DestroyShaderPlatform(ezGALShader* pShader)
{
  ezGALShaderDX11* pDX11Shader = static_cast<ezGALShaderDX11*>(pShader);
  pDX11Shader->DeInitPlatform(this);
  EZ_DEFAULT_DELETE(pDX11Shader);
}

ezGALBuffer* ezGALDeviceDX11::CreateBufferPlatform(const ezGALBufferCreationDescription& Description, const void* pInitialData)
{
  ezGALBufferDX11* pBuffer = EZ_DEFAULT_NEW(ezGALBufferDX11)(Description);

  if(!pBuffer->InitPlatform(this, pInitialData).IsSuccess())
  {
    EZ_DEFAULT_DELETE(pBuffer);
    return NULL;
  }

  return pBuffer;
}

void ezGALDeviceDX11::DestroyBufferPlatform(ezGALBuffer* pBuffer)
{
  ezGALBufferDX11* pDX11Buffer = static_cast<ezGALBufferDX11*>(pBuffer);
  pDX11Buffer->DeInitPlatform(this);
  EZ_DEFAULT_DELETE(pDX11Buffer);
}

ezGALTexture* ezGALDeviceDX11::CreateTexturePlatform(const ezGALTextureCreationDescription& Description, const ezArrayPtr<ezGALSystemMemoryDescription>* pInitialData)
{
  ezGALTextureDX11* pTexture = EZ_DEFAULT_NEW(ezGALTextureDX11)(Description);

  if(!pTexture->InitPlatform(this, pInitialData).IsSuccess())
  {
    EZ_DEFAULT_DELETE(pTexture);
    return NULL;
  }

  return pTexture;
}

void ezGALDeviceDX11::DestroyTexturePlatform(ezGALTexture* pTexture)
{
  ezGALTextureDX11* pDX11Texture = static_cast<ezGALTextureDX11*>(pTexture);
  pDX11Texture->DeInitPlatform(this);
  EZ_DEFAULT_DELETE(pDX11Texture);
}

ezGALResourceView* ezGALDeviceDX11::CreateResourceViewPlatform(const ezGALResourceViewCreationDescription& Description)
{
  ezGALResourceViewDX11* pResourceView = EZ_DEFAULT_NEW(ezGALResourceViewDX11)(Description);

  if(!pResourceView->InitPlatform(this).IsSuccess())
  {
    EZ_DEFAULT_DELETE(pResourceView);
    return NULL;
  }

  return pResourceView;
}

void ezGALDeviceDX11::DestroyResourceViewPlatform(ezGALResourceView* pResourceView)
{
  ezGALResourceViewDX11* pDX11ResourceView = static_cast<ezGALResourceViewDX11*>(pResourceView);
  pDX11ResourceView->DeInitPlatform(this);
  EZ_DEFAULT_DELETE(pDX11ResourceView);
}

ezGALRenderTargetView* ezGALDeviceDX11::CreateRenderTargetViewPlatform(const ezGALRenderTargetViewCreationDescription& Description)
{
  ezGALRenderTargetViewDX11* pRTView = EZ_DEFAULT_NEW(ezGALRenderTargetViewDX11)(Description);

  if(!pRTView->InitPlatform(this).IsSuccess())
  {
    EZ_DEFAULT_DELETE(pRTView);
    return NULL;
  }

  return pRTView;
}

void ezGALDeviceDX11::DestroyRenderTargetViewPlatform(ezGALRenderTargetView* pRenderTargetView)
{
  ezGALRenderTargetViewDX11* pDX11RenderTargetView = static_cast<ezGALRenderTargetViewDX11*>(pRenderTargetView);
  pDX11RenderTargetView->DeInitPlatform(this);
  EZ_DEFAULT_DELETE(pDX11RenderTargetView);
}


// Other rendering creation functions

// TODO: Move the real code creating things to the implementation files (all?)
ezGALSwapChain* ezGALDeviceDX11::CreateSwapChainPlatform(const ezGALSwapChainCreationDescription& Description)
{
  DXGI_SWAP_CHAIN_DESC SwapChainDesc;
  SwapChainDesc.BufferCount = Description.m_bDoubleBuffered ? 2 : 1;
  SwapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT;
  SwapChainDesc.OutputWindow = Description.m_pWindow->GetNativeWindowHandle();
  SwapChainDesc.SampleDesc.Count = Description.m_SampleCount; SwapChainDesc.SampleDesc.Quality = 0; // TODO: Get from MSAA value of the description
  SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
  SwapChainDesc.Windowed = Description.m_bFullscreen ? FALSE : TRUE;
  SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; // TODO: The mode switch needs to be handled (ResizeBuffers + communication with engine)

  // TODO: Get from enumeration of available modes
  // TODO: (Find via format table)
  SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
  SwapChainDesc.BufferDesc.Width = Description.m_pWindow->GetCreationDescription().m_ClientAreaSize.width;
  SwapChainDesc.BufferDesc.Height = Description.m_pWindow->GetCreationDescription().m_ClientAreaSize.height;
  SwapChainDesc.BufferDesc.RefreshRate.Numerator = 60; SwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
  SwapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
  SwapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;

  IDXGISwapChain* pSwapChain = NULL;

  if(FAILED(m_pDXGIFactory->CreateSwapChain(m_pDevice, &SwapChainDesc, &pSwapChain)))
  {
    return NULL;
  }
  else
  {
    // Get texture of the swap chain
    ID3D11Texture2D* pNativeBackBufferTexture = NULL;
    if(FAILED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pNativeBackBufferTexture))))
    {
      ezLog::Error("Couldn't access backbuffer texture of swapchain!");
      EZ_GAL_DX11_RELEASE(pSwapChain);

      return NULL;
    }

    ezGALTextureCreationDescription TexDesc;
    TexDesc.m_uiWidth = SwapChainDesc.BufferDesc.Width;
    TexDesc.m_uiHeight = SwapChainDesc.BufferDesc.Height;
    TexDesc.m_Format = Description.m_BackBufferFormat;
    TexDesc.m_SampleCount = Description.m_SampleCount;
    TexDesc.m_pExisitingNativeObject = pNativeBackBufferTexture;
    TexDesc.m_bCreateRenderTarget = true;

    if (Description.m_bAllowScreenshots)
      TexDesc.m_ResourceAccess.m_bReadBack = true;

    // And create the ez texture object wrapping the backbuffer texture
    ezGALTextureHandle hBackBufferTexture = CreateTexture(TexDesc, NULL);
    EZ_ASSERT(!hBackBufferTexture.IsInvalidated(), "Couldn't create backbuffer texture object!");


    // Create rendertarget view
    ezGALRenderTargetViewCreationDescription RTViewDesc;
    RTViewDesc.m_bReadOnly = true;
    RTViewDesc.m_hTexture = hBackBufferTexture;
    RTViewDesc.m_RenderTargetType = ezGALRenderTargetType::Color;
    RTViewDesc.m_uiFirstSlice = 0;
    RTViewDesc.m_uiMipSlice = 0;
    RTViewDesc.m_uiSliceCount = 1;

    ezGALRenderTargetViewHandle hBackBufferRenderTargetView = CreateRenderTargetView(RTViewDesc);
    EZ_ASSERT(!hBackBufferRenderTargetView.IsInvalidated(), "Couldn't create backbuffer rendertarget view!");

    ezGALSwapChain* pRetValue = new ezGALSwapChainDX11(Description, hBackBufferTexture, hBackBufferRenderTargetView, pSwapChain);
    return pRetValue;
  }
}

void ezGALDeviceDX11::DestroySwapChainPlatform(ezGALSwapChain* pSwapChain)
{
  delete static_cast<ezGALSwapChainDX11*>(pSwapChain);
}

ezGALFence* ezGALDeviceDX11::CreateFencePlatform()
{
  return NULL;
}

void ezGALDeviceDX11::DestroyFencePlatform(ezGALFence* pFence)
{
}

ezGALQuery* ezGALDeviceDX11::CreateQueryPlatform(const ezGALQueryCreationDescription& Description)
{
  return NULL;
}

void ezGALDeviceDX11::DestroyQueryPlatform(ezGALQuery* pQuery)
{
}

ezGALRenderTargetConfig* ezGALDeviceDX11::CreateRenderTargetConfigPlatform(const ezGALRenderTargetConfigCreationDescription& Description)
{
  ezGALRenderTargetConfigDX11* pRenderTargetConfig = EZ_DEFAULT_NEW(ezGALRenderTargetConfigDX11)(Description);

  if(pRenderTargetConfig->InitPlatform(this).IsSuccess())
  {
    return pRenderTargetConfig;
  }
  else
  {
    EZ_DEFAULT_DELETE(pRenderTargetConfig);
    return NULL;
  }
}

void ezGALDeviceDX11::DestroyRenderTargetConfigPlatform(ezGALRenderTargetConfig* pRenderTargetConfig)
{
  delete static_cast<ezGALRenderTargetConfigDX11*>(pRenderTargetConfig);
}

ezGALVertexDeclaration* ezGALDeviceDX11::CreateVertexDeclarationPlatform(const ezGALVertexDeclarationCreationDescription& Description)
{
  ezGALVertexDeclarationDX11* pVertexDeclaration = EZ_DEFAULT_NEW(ezGALVertexDeclarationDX11)(Description);

  if(pVertexDeclaration->InitPlatform(this).IsSuccess())
  {
    return pVertexDeclaration;
  }
  else
  {
    EZ_DEFAULT_DELETE(pVertexDeclaration);
    return NULL;
  }
}

void ezGALDeviceDX11::DestroyVertexDeclarationPlatform(ezGALVertexDeclaration* pVertexDeclaration)
{

  ezGALVertexDeclarationDX11* pVertexDeclarationDX11 = static_cast<ezGALVertexDeclarationDX11*>(pVertexDeclaration);
  pVertexDeclarationDX11->DeInitPlatform(this);

  EZ_DEFAULT_DELETE(pVertexDeclarationDX11);
}


void ezGALDeviceDX11::GetQueryDataPlatform(ezGALQuery* pQuery, ezUInt64* puiRendererdPixels)
{
  *puiRendererdPixels = 42;
}





// Swap chain functions

void ezGALDeviceDX11::PresentPlatform(ezGALSwapChain* pSwapChain)
{
  IDXGISwapChain* pDXGISwapChain = static_cast<ezGALSwapChainDX11*>(pSwapChain)->GetDXSwapChain();

  pDXGISwapChain->Present(0, 0); // TODO: VSync control?
}

// Misc functions

void ezGALDeviceDX11::BeginFramePlatform()
{
}

void ezGALDeviceDX11::EndFramePlatform()
{
}

void ezGALDeviceDX11::FlushPlatform()
{
  GetPrimaryContext<ezGALContextDX11>()->GetDXContext()->Flush();
}

void ezGALDeviceDX11::FinishPlatform()
{
}

void ezGALDeviceDX11::SetPrimarySwapChainPlatform(ezGALSwapChain* pSwapChain)
{
  // Make window association
  m_pDXGIFactory->MakeWindowAssociation(pSwapChain->GetDescription().m_pWindow->GetNativeWindowHandle(), 0);
}



void ezGALDeviceDX11::FillFormatLookupTable()
{

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RGBFloat,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32B32_TYPELESS).RT(DXGI_FORMAT_R32G32B32_FLOAT).VA(DXGI_FORMAT_R32G32B32_FLOAT));

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RGFloat,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32_TYPELESS).RT(DXGI_FORMAT_R32G32_FLOAT).VA(DXGI_FORMAT_R32G32_FLOAT));

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RFloat,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R32_TYPELESS).RT(DXGI_FORMAT_R32_FLOAT).VA(DXGI_FORMAT_R32_FLOAT).RV(DXGI_FORMAT_R32_FLOAT));


  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RGBAHalf,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16B16A16_TYPELESS).RT(DXGI_FORMAT_R16G16B16A16_FLOAT).VA(DXGI_FORMAT_R16G16B16A16_FLOAT).RV(DXGI_FORMAT_R16G16B16A16_FLOAT));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGBAUByteNormalizedsRGB, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8B8A8_TYPELESS).RT(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB).RV(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB));
  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGBAUByteNormalized, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8B8A8_TYPELESS).RT(DXGI_FORMAT_R8G8B8A8_UNORM).RV(DXGI_FORMAT_R8G8B8A8_UNORM));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::D24S8, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R24G8_TYPELESS).DS(DXGI_FORMAT_D24_UNORM_S8_UINT).D(DXGI_FORMAT_R24_UNORM_X8_TYPELESS).S(DXGI_FORMAT_X24_TYPELESS_G8_UINT));

}