
#include <RendererDX11/PCH.h>
#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Device/SwapChainDX11.h>
#include <RendererDX11/Shader/ShaderDX11.h>
#include <RendererDX11/Resources/BufferDX11.h>
#include <RendererDX11/Resources/TextureDX11.h>
#include <RendererDX11/Resources/RenderTargetViewDX11.h>
#include <RendererDX11/Shader/VertexDeclarationDX11.h>
#include <RendererDX11/State/StateDX11.h>
#include <RendererDX11/Resources/ResourceViewDX11.h>
#include <RendererDX11/Context/ContextDX11.h>

#include <Foundation/Logging/Log.h>
#include <System/Window/Window.h>
#include <Foundation/Math/Color.h>

#include <d3d11.h>

ezGALDeviceDX11::ezGALDeviceDX11(const ezGALDeviceCreationDescription& Description)
  : ezGALDevice(Description),
    m_pDevice(nullptr),
    m_pDXGIFactory(nullptr),
    m_pDXGIAdapter(nullptr),
    m_pDXGIDevice(nullptr),
    m_FeatureLevel(D3D_FEATURE_LEVEL_9_1)
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

  D3D_FEATURE_LEVEL FeatureLevels[] =
  {
    D3D_FEATURE_LEVEL_11_1,
    D3D_FEATURE_LEVEL_11_0,
    D3D_FEATURE_LEVEL_10_1,
    D3D_FEATURE_LEVEL_10_0,
    D3D_FEATURE_LEVEL_9_3
  };

  ID3D11DeviceContext* pImmediateContext = nullptr;

  // Manually step through feature levels - if a Win 7 system doesn't have the 11.1 runtime installed
  // The create device call will fail even though the 11.0 (or lower) level could've been
  // intialized successfully
  int FeatureLevelIdx = 0;
  for (FeatureLevelIdx = 0; FeatureLevelIdx < EZ_ARRAY_SIZE(FeatureLevels); FeatureLevelIdx++)
  {
    if (SUCCEEDED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, dwFlags, &FeatureLevels[FeatureLevelIdx], 1, D3D11_SDK_VERSION, &m_pDevice, &m_FeatureLevel, &pImmediateContext)))
    {
      break;
    }
  }

  // Nothing could be initialized:
  if (pImmediateContext == nullptr)
  {
    ezLog::Error("Couldn't initialize D3D11 device!");
    return EZ_FAILURE;
  }
  else
  {
    const char* FeatureLevelNames[] =
    {
      "11.1",
      "11.0",
      "10.1",
      "10",
      "9.3"
    };

    EZ_CHECK_AT_COMPILETIME(EZ_ARRAY_SIZE(FeatureLevels) == EZ_ARRAY_SIZE(FeatureLevelNames));

    ezLog::Info("Initialized D3D11 device with feature level %s.", FeatureLevelNames[FeatureLevelIdx]);
  }
  

  // Create primary context object
  m_pPrimaryContext = EZ_NEW(&m_Allocator, ezGALContextDX11, this, pImmediateContext);
  EZ_ASSERT_RELEASE(m_pPrimaryContext != nullptr, "Couldn't create primary context!");

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

  /// \todo Get features of the device (depending on feature level, CheckFormat* functions etc.)

  ezProjectionDepthRange::Default = ezProjectionDepthRange::ZeroToOne;

  return EZ_SUCCESS;
}

ezResult ezGALDeviceDX11::ShutdownPlatform()
{
  EZ_DELETE(&m_Allocator, m_pPrimaryContext);

  EZ_GAL_DX11_RELEASE(m_pDevice);
  EZ_GAL_DX11_RELEASE(m_pDXGIFactory);
  EZ_GAL_DX11_RELEASE(m_pDXGIAdapter);
  EZ_GAL_DX11_RELEASE(m_pDXGIDevice);

  return EZ_SUCCESS;
}


// State creation functions

ezGALBlendState* ezGALDeviceDX11::CreateBlendStatePlatform(const ezGALBlendStateCreationDescription& Description)
{
  ezGALBlendStateDX11* pState = EZ_NEW(&m_Allocator, ezGALBlendStateDX11, Description);

  if (pState->InitPlatform(this).Succeeded())
  {
    return pState;
  }
  else
  {
    EZ_DELETE(&m_Allocator, pState);
    return nullptr;
  }
  return nullptr;
}

void ezGALDeviceDX11::DestroyBlendStatePlatform(ezGALBlendState* pBlendState)
{
  ezGALBlendStateDX11* pState = static_cast<ezGALBlendStateDX11*>(pBlendState);
  pState->DeInitPlatform(this);
  EZ_DELETE(&m_Allocator, pState);
}

ezGALDepthStencilState* ezGALDeviceDX11::CreateDepthStencilStatePlatform(const ezGALDepthStencilStateCreationDescription& Description)
{
  ezGALDepthStencilStateDX11* pDX11DepthStencilState = EZ_NEW(&m_Allocator, ezGALDepthStencilStateDX11, Description);

  if (pDX11DepthStencilState->InitPlatform(this).Succeeded())
  {
    return pDX11DepthStencilState;
  }
  else
  {
    EZ_DELETE(&m_Allocator, pDX11DepthStencilState);
    return nullptr;
  }
}

void ezGALDeviceDX11::DestroyDepthStencilStatePlatform(ezGALDepthStencilState* pDepthStencilState)
{
  ezGALDepthStencilStateDX11* pDX11DepthStencilState = static_cast<ezGALDepthStencilStateDX11*>(pDepthStencilState);
  pDX11DepthStencilState->DeInitPlatform(this);
  EZ_DELETE(&m_Allocator, pDX11DepthStencilState);
}

ezGALRasterizerState* ezGALDeviceDX11::CreateRasterizerStatePlatform(const ezGALRasterizerStateCreationDescription& Description)
{
  ezGALRasterizerStateDX11* pDX11RasterizerState = EZ_NEW(&m_Allocator, ezGALRasterizerStateDX11, Description);

  if(pDX11RasterizerState->InitPlatform(this).Succeeded())
  {
    return pDX11RasterizerState;
  }
  else
  {
    EZ_DELETE(&m_Allocator, pDX11RasterizerState);
    return nullptr;
  }
}

void ezGALDeviceDX11::DestroyRasterizerStatePlatform(ezGALRasterizerState* pRasterizerState)
{
  ezGALRasterizerStateDX11* pDX11RasterizerState = static_cast<ezGALRasterizerStateDX11*>(pRasterizerState);
  pDX11RasterizerState->DeInitPlatform(this);
  EZ_DELETE(&m_Allocator, pDX11RasterizerState);
}

ezGALSamplerState* ezGALDeviceDX11::CreateSamplerStatePlatform(const ezGALSamplerStateCreationDescription& Description)
{
  ezGALSamplerStateDX11* pDX11SamplerState = EZ_NEW(&m_Allocator, ezGALSamplerStateDX11, Description);

  if (pDX11SamplerState->InitPlatform(this).Succeeded())
  {
    return pDX11SamplerState;
  }
  else
  {
    EZ_DELETE(&m_Allocator, pDX11SamplerState);
    return nullptr;
  }
}

void ezGALDeviceDX11::DestroySamplerStatePlatform(ezGALSamplerState* pSamplerState)
{
  ezGALSamplerStateDX11* pDX11SamplerState = static_cast<ezGALSamplerStateDX11*>(pSamplerState);
  pDX11SamplerState->DeInitPlatform(this);
  EZ_DELETE(&m_Allocator, pDX11SamplerState);
}


// Resource creation functions

ezGALShader* ezGALDeviceDX11::CreateShaderPlatform(const ezGALShaderCreationDescription& Description)
{
  ezGALShaderDX11* pShader = EZ_NEW(&m_Allocator, ezGALShaderDX11, Description);

  if(!pShader->InitPlatform(this).Succeeded())
  {
    EZ_DELETE(&m_Allocator, pShader);
    return nullptr;
  }

  return pShader;
}

void ezGALDeviceDX11::DestroyShaderPlatform(ezGALShader* pShader)
{
  ezGALShaderDX11* pDX11Shader = static_cast<ezGALShaderDX11*>(pShader);
  pDX11Shader->DeInitPlatform(this);
  EZ_DELETE(&m_Allocator, pDX11Shader);
}

ezGALBuffer* ezGALDeviceDX11::CreateBufferPlatform(const ezGALBufferCreationDescription& Description, const void* pInitialData)
{
  ezGALBufferDX11* pBuffer = EZ_NEW(&m_Allocator, ezGALBufferDX11, Description);

  if(!pBuffer->InitPlatform(this, pInitialData).Succeeded())
  {
    EZ_DELETE(&m_Allocator, pBuffer);
    return nullptr;
  }

  return pBuffer;
}

void ezGALDeviceDX11::DestroyBufferPlatform(ezGALBuffer* pBuffer)
{
  ezGALBufferDX11* pDX11Buffer = static_cast<ezGALBufferDX11*>(pBuffer);
  pDX11Buffer->DeInitPlatform(this);
  EZ_DELETE(&m_Allocator, pDX11Buffer);
}

ezGALTexture* ezGALDeviceDX11::CreateTexturePlatform(const ezGALTextureCreationDescription& Description, const ezArrayPtr<ezGALSystemMemoryDescription>* pInitialData)
{
  ezGALTextureDX11* pTexture = EZ_NEW(&m_Allocator, ezGALTextureDX11, Description);

  if(!pTexture->InitPlatform(this, pInitialData).Succeeded())
  {
    EZ_DELETE(&m_Allocator, pTexture);
    return nullptr;
  }

  return pTexture;
}

void ezGALDeviceDX11::DestroyTexturePlatform(ezGALTexture* pTexture)
{
  ezGALTextureDX11* pDX11Texture = static_cast<ezGALTextureDX11*>(pTexture);
  pDX11Texture->DeInitPlatform(this);
  EZ_DELETE(&m_Allocator, pDX11Texture);
}

ezGALResourceView* ezGALDeviceDX11::CreateResourceViewPlatform(const ezGALResourceViewCreationDescription& Description)
{
  ezGALResourceViewDX11* pResourceView = EZ_NEW(&m_Allocator, ezGALResourceViewDX11, Description);

  if(!pResourceView->InitPlatform(this).Succeeded())
  {
    EZ_DELETE(&m_Allocator, pResourceView);
    return nullptr;
  }

  return pResourceView;
}

void ezGALDeviceDX11::DestroyResourceViewPlatform(ezGALResourceView* pResourceView)
{
  ezGALResourceViewDX11* pDX11ResourceView = static_cast<ezGALResourceViewDX11*>(pResourceView);
  pDX11ResourceView->DeInitPlatform(this);
  EZ_DELETE(&m_Allocator, pDX11ResourceView);
}

ezGALRenderTargetView* ezGALDeviceDX11::CreateRenderTargetViewPlatform(const ezGALRenderTargetViewCreationDescription& Description)
{
  ezGALRenderTargetViewDX11* pRTView = EZ_NEW(&m_Allocator, ezGALRenderTargetViewDX11, Description);

  if(!pRTView->InitPlatform(this).Succeeded())
  {
    EZ_DELETE(&m_Allocator, pRTView);
    return nullptr;
  }

  return pRTView;
}

void ezGALDeviceDX11::DestroyRenderTargetViewPlatform(ezGALRenderTargetView* pRenderTargetView)
{
  ezGALRenderTargetViewDX11* pDX11RenderTargetView = static_cast<ezGALRenderTargetViewDX11*>(pRenderTargetView);
  pDX11RenderTargetView->DeInitPlatform(this);
  EZ_DELETE(&m_Allocator, pDX11RenderTargetView);
}


// Other rendering creation functions

/// \todo Move the real code creating things to the implementation files (all?)
ezGALSwapChain* ezGALDeviceDX11::CreateSwapChainPlatform(const ezGALSwapChainCreationDescription& Description)
{
  ezGALSwapChainDX11* pSwapChain = EZ_NEW(&m_Allocator, ezGALSwapChainDX11, Description);

  if (!pSwapChain->InitPlatform(this).Succeeded())
  {
    EZ_DELETE(&m_Allocator, pSwapChain);
    return nullptr;
  }

  return pSwapChain;
}

void ezGALDeviceDX11::DestroySwapChainPlatform(ezGALSwapChain* pSwapChain)
{
  ezGALSwapChainDX11* pSwapChainDX11 = static_cast<ezGALSwapChainDX11*>(pSwapChain);
  pSwapChainDX11->DeInitPlatform(this);
  EZ_DELETE(&m_Allocator, pSwapChainDX11);
}

ezGALFence* ezGALDeviceDX11::CreateFencePlatform()
{
  EZ_ASSERT_NOT_IMPLEMENTED;
  return nullptr;
}

void ezGALDeviceDX11::DestroyFencePlatform(ezGALFence* pFence)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

ezGALQuery* ezGALDeviceDX11::CreateQueryPlatform(const ezGALQueryCreationDescription& Description)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
  return nullptr;
}

void ezGALDeviceDX11::DestroyQueryPlatform(ezGALQuery* pQuery)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

ezGALVertexDeclaration* ezGALDeviceDX11::CreateVertexDeclarationPlatform(const ezGALVertexDeclarationCreationDescription& Description)
{
  ezGALVertexDeclarationDX11* pVertexDeclaration = EZ_NEW(&m_Allocator, ezGALVertexDeclarationDX11, Description);

  if(pVertexDeclaration->InitPlatform(this).Succeeded())
  {
    return pVertexDeclaration;
  }
  else
  {
    EZ_DELETE(&m_Allocator, pVertexDeclaration);
    return nullptr;
  }
}

void ezGALDeviceDX11::DestroyVertexDeclarationPlatform(ezGALVertexDeclaration* pVertexDeclaration)
{
  // Why is it necessary to cast this here and then call the virtual function???
  // couldn't the renderer abstraction call DeInitPlatform directly (also EZ_DEFAULT_DELETE) ?

  ezGALVertexDeclarationDX11* pVertexDeclarationDX11 = static_cast<ezGALVertexDeclarationDX11*>(pVertexDeclaration);
  pVertexDeclarationDX11->DeInitPlatform(this);

  EZ_DELETE(&m_Allocator, pVertexDeclarationDX11);
}


void ezGALDeviceDX11::GetQueryDataPlatform(ezGALQuery* pQuery, ezUInt64* puiRendererdPixels)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
  *puiRendererdPixels = 42;
}





// Swap chain functions

void ezGALDeviceDX11::PresentPlatform(ezGALSwapChain* pSwapChain)
{
  IDXGISwapChain* pDXGISwapChain = static_cast<ezGALSwapChainDX11*>(pSwapChain)->GetDXSwapChain();

  pDXGISwapChain->Present(pSwapChain->GetDescription().m_bVerticalSynchronization ? 1 : 0, 0);
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


void ezGALDeviceDX11::FillCapabilitiesPlatform()
{
  switch (m_FeatureLevel)
  {
  case D3D_FEATURE_LEVEL_11_1:
    m_Capabilities.m_bB5G6R5Textures = true;
  case D3D_FEATURE_LEVEL_11_0:
    m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::VertexShader] = true;
    m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::HullShader] = true;
    m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::DomainShader] = true;
    m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::GeometryShader] = true;
    m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::PixelShader] = true;
    m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::ComputeShader] = true;
    m_Capabilities.m_bInstancing = true;
    m_Capabilities.m_b32BitIndices = true;
    m_Capabilities.m_bIndirectDraw = true;
    m_Capabilities.m_bStreamOut = true;
    m_Capabilities.m_uiMaxConstantBuffers = D3D11_COMMONSHADER_CONSTANT_BUFFER_HW_SLOT_COUNT;
    m_Capabilities.m_bTextureArrays = true;
    m_Capabilities.m_bCubemapArrays = true;
    m_Capabilities.m_uiMaxTextureDimension = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
    m_Capabilities.m_uiMaxCubemapDimension = D3D11_REQ_TEXTURECUBE_DIMENSION;
    m_Capabilities.m_uiMax3DTextureDimension = D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
    m_Capabilities.m_uiMaxAnisotropy = D3D11_REQ_MAXANISOTROPY;
    m_Capabilities.m_uiMaxRendertargets = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;
    m_Capabilities.m_uiUAVCount = (m_FeatureLevel == D3D_FEATURE_LEVEL_11_1 ? 64 : 8);
    m_Capabilities.m_bAlphaToCoverage = true;
    break;

  case D3D_FEATURE_LEVEL_10_1:
  case D3D_FEATURE_LEVEL_10_0:
    m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::VertexShader] = true;
    m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::HullShader] = false;
    m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::DomainShader] = false;
    m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::GeometryShader] = true;
    m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::PixelShader] = true;
    m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::ComputeShader] = false;
    m_Capabilities.m_bInstancing = true;
    m_Capabilities.m_b32BitIndices = true;
    m_Capabilities.m_bIndirectDraw = false;
    m_Capabilities.m_bStreamOut = true;
    m_Capabilities.m_uiMaxConstantBuffers = D3D11_COMMONSHADER_CONSTANT_BUFFER_HW_SLOT_COUNT;
    m_Capabilities.m_bTextureArrays = true;
    m_Capabilities.m_bCubemapArrays = (m_FeatureLevel == D3D_FEATURE_LEVEL_10_1 ? true : false);
    m_Capabilities.m_uiMaxTextureDimension = D3D10_REQ_TEXTURE2D_U_OR_V_DIMENSION;
    m_Capabilities.m_uiMaxCubemapDimension = D3D10_REQ_TEXTURECUBE_DIMENSION;
    m_Capabilities.m_uiMax3DTextureDimension = D3D10_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
    m_Capabilities.m_uiMaxAnisotropy = D3D10_REQ_MAXANISOTROPY;
    m_Capabilities.m_uiMaxRendertargets = D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT;
    m_Capabilities.m_uiUAVCount = 0;
    m_Capabilities.m_bAlphaToCoverage = true;
    break;

  case D3D_FEATURE_LEVEL_9_3:
    m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::VertexShader] = true;
    m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::HullShader] = false;
    m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::DomainShader] = false;
    m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::GeometryShader] = false;
    m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::PixelShader] = true;
    m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::ComputeShader] = false;
    m_Capabilities.m_bInstancing = true;
    m_Capabilities.m_b32BitIndices = true;
    m_Capabilities.m_bIndirectDraw = false;
    m_Capabilities.m_bStreamOut = false;
    m_Capabilities.m_uiMaxConstantBuffers = D3D11_COMMONSHADER_CONSTANT_BUFFER_HW_SLOT_COUNT;
    m_Capabilities.m_bTextureArrays = false;
    m_Capabilities.m_bCubemapArrays = false;
    m_Capabilities.m_uiMaxTextureDimension = D3D_FL9_3_REQ_TEXTURE1D_U_DIMENSION;
    m_Capabilities.m_uiMaxCubemapDimension = D3D_FL9_3_REQ_TEXTURECUBE_DIMENSION;
    m_Capabilities.m_uiMax3DTextureDimension = 0;
    m_Capabilities.m_uiMaxAnisotropy = 16;
    m_Capabilities.m_uiMaxRendertargets = D3D_FL9_3_SIMULTANEOUS_RENDER_TARGET_COUNT;
    m_Capabilities.m_uiUAVCount = 0;
    m_Capabilities.m_bAlphaToCoverage = false;
    break;

  }

}


void ezGALDeviceDX11::FillFormatLookupTable()
{
  ///       The list below is in the same order as the ezGALResourceFormat enum. No format should be missing except the ones that are just different names for the same enum value.

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RGBAFloat,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32B32A32_TYPELESS).
      RT(DXGI_FORMAT_R32G32B32A32_FLOAT).
      VA(DXGI_FORMAT_R32G32B32A32_FLOAT).
      RV(DXGI_FORMAT_R32G32B32A32_FLOAT)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RGBAUInt,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32B32A32_TYPELESS).
      RT(DXGI_FORMAT_R32G32B32A32_UINT).
      VA(DXGI_FORMAT_R32G32B32A32_UINT).
      RV(DXGI_FORMAT_R32G32B32A32_UINT)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RGBAInt,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32B32A32_TYPELESS).
      RT(DXGI_FORMAT_R32G32B32A32_SINT).
      VA(DXGI_FORMAT_R32G32B32A32_SINT).
      RV(DXGI_FORMAT_R32G32B32A32_SINT)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RGBFloat,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32B32_TYPELESS).
      RT(DXGI_FORMAT_R32G32B32_FLOAT).
      VA(DXGI_FORMAT_R32G32B32_FLOAT).
      RV(DXGI_FORMAT_R32G32B32_FLOAT)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RGBUInt,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32B32_TYPELESS).
      RT(DXGI_FORMAT_R32G32B32_UINT).
      VA(DXGI_FORMAT_R32G32B32_UINT).
      RV(DXGI_FORMAT_R32G32B32_UINT)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RGBInt,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32B32_TYPELESS).
      RT(DXGI_FORMAT_R32G32B32_SINT).
      VA(DXGI_FORMAT_R32G32B32_SINT).
      RV(DXGI_FORMAT_R32G32B32_SINT)
      );

  // Supported with DX 11.1
  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::B5G6R5UNormalized,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_B5G6R5_UNORM).
      RT(DXGI_FORMAT_B5G6R5_UNORM).
      VA(DXGI_FORMAT_B5G6R5_UNORM).
      RV(DXGI_FORMAT_B5G6R5_UNORM)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::BGRAUByteNormalized,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_B8G8R8A8_TYPELESS).
      RT(DXGI_FORMAT_B8G8R8A8_UNORM).
      VA(DXGI_FORMAT_B8G8R8A8_UNORM).
      RV(DXGI_FORMAT_B8G8R8A8_UNORM)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::BGRAUByteNormalizedsRGB,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_B8G8R8A8_TYPELESS).
      RT(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB).
      RV(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RGBAHalf,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16B16A16_TYPELESS).
      RT(DXGI_FORMAT_R16G16B16A16_FLOAT).
      VA(DXGI_FORMAT_R16G16B16A16_FLOAT).
      RV(DXGI_FORMAT_R16G16B16A16_FLOAT)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RGBAUShort,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16B16A16_TYPELESS).
      RT(DXGI_FORMAT_R16G16B16A16_UINT).
      VA(DXGI_FORMAT_R16G16B16A16_UINT).
      RV(DXGI_FORMAT_R16G16B16A16_UINT)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RGBAUShortNormalized,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16B16A16_TYPELESS).
      RT(DXGI_FORMAT_R16G16B16A16_UNORM).
      VA(DXGI_FORMAT_R16G16B16A16_UNORM).
      RV(DXGI_FORMAT_R16G16B16A16_UNORM)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RGBAShort,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16B16A16_TYPELESS).
      RT(DXGI_FORMAT_R16G16B16A16_SINT).
      VA(DXGI_FORMAT_R16G16B16A16_SINT).
      RV(DXGI_FORMAT_R16G16B16A16_SINT)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RGBAShortNormalized,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16B16A16_TYPELESS).
      RT(DXGI_FORMAT_R16G16B16A16_SNORM).
      VA(DXGI_FORMAT_R16G16B16A16_SNORM).
      RV(DXGI_FORMAT_R16G16B16A16_SNORM)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RGFloat,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32_TYPELESS).
      RT(DXGI_FORMAT_R32G32_FLOAT).
      VA(DXGI_FORMAT_R32G32_FLOAT).
      RV(DXGI_FORMAT_R32G32_FLOAT));

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RGUInt,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32_TYPELESS).
      RT(DXGI_FORMAT_R32G32_UINT).
      VA(DXGI_FORMAT_R32G32_UINT).
      RV(DXGI_FORMAT_R32G32_UINT));

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RGInt,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32_TYPELESS).
      RT(DXGI_FORMAT_R32G32_SINT).
      VA(DXGI_FORMAT_R32G32_SINT).
      RV(DXGI_FORMAT_R32G32_SINT));

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RGB10A2UInt,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R10G10B10A2_TYPELESS).
      RT(DXGI_FORMAT_R10G10B10A2_UINT).
      VA(DXGI_FORMAT_R10G10B10A2_UINT).
      RV(DXGI_FORMAT_R10G10B10A2_UINT));

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RGB10A2UIntNormalized,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R10G10B10A2_TYPELESS).
      RT(DXGI_FORMAT_R10G10B10A2_UNORM).
      VA(DXGI_FORMAT_R10G10B10A2_UNORM).
      RV(DXGI_FORMAT_R10G10B10A2_UNORM));

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RG11B10Float,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R11G11B10_FLOAT).
      RT(DXGI_FORMAT_R11G11B10_FLOAT).
      VA(DXGI_FORMAT_R11G11B10_FLOAT).
      RV(DXGI_FORMAT_R11G11B10_FLOAT));

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RGBAUByteNormalized, 
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8B8A8_TYPELESS).
      RT(DXGI_FORMAT_R8G8B8A8_UNORM).
      VA(DXGI_FORMAT_R8G8B8A8_UNORM).
      RV(DXGI_FORMAT_R8G8B8A8_UNORM)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RGBAUByteNormalizedsRGB, 
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8B8A8_TYPELESS).
      RT(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB).
      RV(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RGBAUByte, 
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8B8A8_TYPELESS).
      RT(DXGI_FORMAT_R8G8B8A8_UINT).
      VA(DXGI_FORMAT_R8G8B8A8_UINT).
      RV(DXGI_FORMAT_R8G8B8A8_UINT)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RGBAByteNormalized, 
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8B8A8_TYPELESS).
      RT(DXGI_FORMAT_R8G8B8A8_SNORM).
      VA(DXGI_FORMAT_R8G8B8A8_SNORM).
      RV(DXGI_FORMAT_R8G8B8A8_SNORM)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RGBAByte, 
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8B8A8_TYPELESS).
      RT(DXGI_FORMAT_R8G8B8A8_SINT).
      VA(DXGI_FORMAT_R8G8B8A8_SINT).
      RV(DXGI_FORMAT_R8G8B8A8_SINT)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RGHalf, 
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16_TYPELESS).
      RT(DXGI_FORMAT_R16G16_FLOAT).
      VA(DXGI_FORMAT_R16G16_FLOAT).
      RV(DXGI_FORMAT_R16G16_FLOAT)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RGUShort, 
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16_TYPELESS).
      RT(DXGI_FORMAT_R16G16_UINT).
      VA(DXGI_FORMAT_R16G16_UINT).
      RV(DXGI_FORMAT_R16G16_UINT)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RGUShortNormalized, 
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16_TYPELESS).
      RT(DXGI_FORMAT_R16G16_UNORM).
      VA(DXGI_FORMAT_R16G16_UNORM).
      RV(DXGI_FORMAT_R16G16_UNORM)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RGShort, 
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16_TYPELESS).
      RT(DXGI_FORMAT_R16G16_SINT).
      VA(DXGI_FORMAT_R16G16_SINT).
      RV(DXGI_FORMAT_R16G16_SINT)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RGShortNormalized, 
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16_TYPELESS).
      RT(DXGI_FORMAT_R16G16_SNORM).
      VA(DXGI_FORMAT_R16G16_SNORM).
      RV(DXGI_FORMAT_R16G16_SNORM)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RGUByte, 
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8_TYPELESS).
      RT(DXGI_FORMAT_R8G8_UINT).
      VA(DXGI_FORMAT_R8G8_UINT).
      RV(DXGI_FORMAT_R8G8_UINT)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RGUByteNormalized, 
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8_TYPELESS).
      RT(DXGI_FORMAT_R8G8_UNORM).
      VA(DXGI_FORMAT_R8G8_UNORM).
      RV(DXGI_FORMAT_R8G8_UNORM)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RGByte, 
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8_TYPELESS).
      RT(DXGI_FORMAT_R8G8_SINT).
      VA(DXGI_FORMAT_R8G8_SINT).
      RV(DXGI_FORMAT_R8G8_SINT)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RGByteNormalized, 
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8_TYPELESS).
      RT(DXGI_FORMAT_R8G8_SNORM).
      VA(DXGI_FORMAT_R8G8_SNORM).
      RV(DXGI_FORMAT_R8G8_SNORM)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::DFloat, 
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R32_TYPELESS).
      RT(DXGI_FORMAT_D32_FLOAT).
      RV(DXGI_FORMAT_D32_FLOAT).
      D(DXGI_FORMAT_D32_FLOAT).
      DS(DXGI_FORMAT_D32_FLOAT)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RFloat,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R32_TYPELESS).
      RT(DXGI_FORMAT_R32_FLOAT).
      VA(DXGI_FORMAT_R32_FLOAT).
      RV(DXGI_FORMAT_R32_FLOAT)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RUInt,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R32_TYPELESS).
      RT(DXGI_FORMAT_R32_UINT).
      VA(DXGI_FORMAT_R32_UINT).
      RV(DXGI_FORMAT_R32_UINT)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RInt,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R32_TYPELESS).
      RT(DXGI_FORMAT_R32_SINT).
      VA(DXGI_FORMAT_R32_SINT).
      RV(DXGI_FORMAT_R32_SINT)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RHalf,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R16_TYPELESS).
      RT(DXGI_FORMAT_R16_FLOAT).
      VA(DXGI_FORMAT_R16_FLOAT).
      RV(DXGI_FORMAT_R16_FLOAT)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RUShort,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R16_TYPELESS).
      RT(DXGI_FORMAT_R16_UINT).
      VA(DXGI_FORMAT_R16_UINT).
      RV(DXGI_FORMAT_R16_UINT)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RUShortNormalized,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R16_TYPELESS).
      RT(DXGI_FORMAT_R16_UNORM).
      VA(DXGI_FORMAT_R16_UNORM).
      RV(DXGI_FORMAT_R16_UNORM).
      D(DXGI_FORMAT_D16_UNORM)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RShort,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R16_TYPELESS).
      RT(DXGI_FORMAT_R16_SINT).
      VA(DXGI_FORMAT_R16_SINT).
      RV(DXGI_FORMAT_R16_SINT)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RShortNormalized,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R16_TYPELESS).
      RT(DXGI_FORMAT_R16_SNORM).
      VA(DXGI_FORMAT_R16_SNORM).
      RV(DXGI_FORMAT_R16_SNORM)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RUByte,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R8_TYPELESS).
      RT(DXGI_FORMAT_R8_UINT).
      VA(DXGI_FORMAT_R8_UINT).
      RV(DXGI_FORMAT_R8_UINT)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RUByteNormalized,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R8_TYPELESS).
      RT(DXGI_FORMAT_R8_UNORM).
      VA(DXGI_FORMAT_R8_UNORM).
      RV(DXGI_FORMAT_R8_UNORM)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RByte,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R8_TYPELESS).
      RT(DXGI_FORMAT_R8_SINT).
      VA(DXGI_FORMAT_R8_SINT).
      RV(DXGI_FORMAT_R8_SINT)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RByteNormalized,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R8_TYPELESS).
      RT(DXGI_FORMAT_R8_SNORM).
      VA(DXGI_FORMAT_R8_SNORM).
      RV(DXGI_FORMAT_R8_SNORM)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::AUByteNormalized,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R8_TYPELESS).
      RT(DXGI_FORMAT_A8_UNORM).
      VA(DXGI_FORMAT_A8_UNORM).
      RV(DXGI_FORMAT_A8_UNORM)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::D24S8,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R24G8_TYPELESS).
      DS(DXGI_FORMAT_D24_UNORM_S8_UINT).
      D(DXGI_FORMAT_R24_UNORM_X8_TYPELESS).
      S(DXGI_FORMAT_X24_TYPELESS_G8_UINT)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::BC1,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_BC1_TYPELESS).
      RV(DXGI_FORMAT_BC1_UNORM)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::BC1sRGB,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_BC1_TYPELESS).
      RV(DXGI_FORMAT_BC1_UNORM_SRGB)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::BC2,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_BC2_TYPELESS).
      RV(DXGI_FORMAT_BC2_UNORM)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::BC2sRGB,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_BC2_TYPELESS).
      RV(DXGI_FORMAT_BC2_UNORM_SRGB)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::BC3,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_BC3_TYPELESS).
      RV(DXGI_FORMAT_BC3_UNORM)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::BC3sRGB,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_BC3_TYPELESS).
      RV(DXGI_FORMAT_BC3_UNORM_SRGB)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::BC4UNormalized,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_BC4_TYPELESS).
      RV(DXGI_FORMAT_BC4_UNORM)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::BC4Normalized,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_BC4_TYPELESS).
      RV(DXGI_FORMAT_BC4_SNORM)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::BC5UNormalized,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_BC5_TYPELESS).
      RV(DXGI_FORMAT_BC5_UNORM)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::BC5Normalized,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_BC5_TYPELESS).
      RV(DXGI_FORMAT_BC5_SNORM)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::BC6UFloat,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_BC6H_TYPELESS).
      RV(DXGI_FORMAT_BC6H_UF16)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::BC6Float,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_BC6H_TYPELESS).
      RV(DXGI_FORMAT_BC6H_SF16)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::BC7UNormalized,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_BC7_TYPELESS).
      RV(DXGI_FORMAT_BC7_UNORM)
      );

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::BC7UNormalizedsRGB,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_BC7_TYPELESS).
      RV(DXGI_FORMAT_BC7_UNORM_SRGB)
      );

}



EZ_STATICLINK_FILE(RendererDX11, RendererDX11_Device_Implementation_DeviceDX11);

