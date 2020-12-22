#include <RendererDX11PCH.h>

#include <Core/System/Window.h>
#include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#include <RendererDX11/CommandEncoder/CommandEncoderImplDX11.h>
#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Device/PassDX11.h>
#include <RendererDX11/Device/SwapChainDX11.h>
#include <RendererDX11/Resources/BufferDX11.h>
#include <RendererDX11/Resources/FenceDX11.h>
#include <RendererDX11/Resources/QueryDX11.h>
#include <RendererDX11/Resources/RenderTargetViewDX11.h>
#include <RendererDX11/Resources/ResourceViewDX11.h>
#include <RendererDX11/Resources/TextureDX11.h>
#include <RendererDX11/Resources/UnorderedAccessViewDX11.h>
#include <RendererDX11/Shader/ShaderDX11.h>
#include <RendererDX11/Shader/VertexDeclarationDX11.h>
#include <RendererDX11/State/StateDX11.h>
#include <RendererFoundation/Device/DeviceFactory.h>
#include <RendererFoundation/CommandEncoder/RenderCommandEncoder.h>

#include <d3d11.h>
#include <d3d11_3.h>
#include <dxgidebug.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
#  include <d3d11_1.h>
#endif

ezGALDevice* CreateDX11Device(ezAllocatorBase* pAllocator, const ezGALDeviceCreationDescription& Description)
{
  return EZ_NEW(pAllocator, ezGALDeviceDX11, Description);
}

static bool s_Dummy = ezGALDeviceFactory::RegisterCreatorFunc("DX11", &CreateDX11Device, "DX11_SM50", "ezShaderCompilerHLSL");

ezGALDeviceDX11::ezGALDeviceDX11(const ezGALDeviceCreationDescription& Description)
  : ezGALDevice(Description)
  , m_pDevice(nullptr)
  , m_pDevice3(nullptr)
  , m_pDebug(nullptr)
  , m_pDXGIFactory(nullptr)
  , m_pDXGIAdapter(nullptr)
  , m_pDXGIDevice(nullptr)
  , m_FeatureLevel(D3D_FEATURE_LEVEL_9_1)
  , m_uiFrameCounter(0)
{
}

ezGALDeviceDX11::~ezGALDeviceDX11() = default;

// Init & shutdown functions

ezResult ezGALDeviceDX11::InitPlatform(DWORD dwFlags, IDXGIAdapter* pUsedAdapter)
{
  EZ_LOG_BLOCK("ezGALDeviceDX11::InitPlatform");

retry:

  if (m_Description.m_bDebugDevice)
    dwFlags |= D3D11_CREATE_DEVICE_DEBUG;
  else
    dwFlags &= ~D3D11_CREATE_DEVICE_DEBUG;

  D3D_FEATURE_LEVEL FeatureLevels[] = {D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0, D3D_FEATURE_LEVEL_9_3};
  ID3D11DeviceContext* pImmediateContext = nullptr;

  D3D_DRIVER_TYPE driverType = D3D_DRIVER_TYPE_HARDWARE;
  // driverType = D3D_DRIVER_TYPE_REFERENCE; // enables the Reference Device

  if (pUsedAdapter != nullptr)
  {
    // required by the specification
    driverType = D3D_DRIVER_TYPE_UNKNOWN;
  }

  // Manually step through feature levels - if a Win 7 system doesn't have the 11.1 runtime installed
  // The create device call will fail even though the 11.0 (or lower) level could've been
  // initialized successfully
  int FeatureLevelIdx = 0;
  for (FeatureLevelIdx = 0; FeatureLevelIdx < EZ_ARRAY_SIZE(FeatureLevels); FeatureLevelIdx++)
  {
    if (SUCCEEDED(D3D11CreateDevice(pUsedAdapter, driverType, nullptr, dwFlags, &FeatureLevels[FeatureLevelIdx], 1, D3D11_SDK_VERSION, &m_pDevice, (D3D_FEATURE_LEVEL*)&m_FeatureLevel, &pImmediateContext)))
    {
      break;
    }
  }

  // Nothing could be initialized:
  if (pImmediateContext == nullptr)
  {
    if (m_Description.m_bDebugDevice)
    {
      ezLog::Error("Couldn't initialize D3D11 debug device!");

      m_Description.m_bDebugDevice = false;
      goto retry;
    }

    ezLog::Error("Couldn't initialize D3D11 device!");
    return EZ_FAILURE;
  }
  else
  {
    m_pImmediateContext = pImmediateContext;

    const char* FeatureLevelNames[] = {"11.1", "11.0", "10.1", "10", "9.3"};

    EZ_CHECK_AT_COMPILETIME(EZ_ARRAY_SIZE(FeatureLevels) == EZ_ARRAY_SIZE(FeatureLevelNames));

    ezLog::Success("Initialized D3D11 device with feature level {0}.", FeatureLevelNames[FeatureLevelIdx]);
  }

  if (m_Description.m_bDebugDevice)
  {
    if (SUCCEEDED(m_pDevice->QueryInterface(__uuidof(ID3D11Debug), (void**)&m_pDebug)))
    {
      ID3D11InfoQueue* pInfoQueue = nullptr;
      if (SUCCEEDED(m_pDebug->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&pInfoQueue)))
      {
        // only do this when a debugger is attached, otherwise the app would crash on every DX error
        if (IsDebuggerPresent())
        {
          pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
          pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
          pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, TRUE);
        }

        // Ignore list.
        {
          D3D11_MESSAGE_ID hide[] = {
            // Hide messages about abandoned query results. This can easily happen when a GPUStopwatch is suddenly unused.
            D3D11_MESSAGE_ID_QUERY_BEGIN_ABANDONING_PREVIOUS_RESULTS, D3D11_MESSAGE_ID_QUERY_END_ABANDONING_PREVIOUS_RESULTS,
            // Don't break on invalid input assembly. This can easily happen when using the wrong mesh-material combination.
            D3D11_MESSAGE_ID_CREATEINPUTLAYOUT_MISSINGELEMENT,
            // Add more message IDs here as needed
          };
          D3D11_INFO_QUEUE_FILTER filter;
          ezMemoryUtils::ZeroFill(&filter, 1);
          filter.DenyList.NumIDs = _countof(hide);
          filter.DenyList.pIDList = hide;
          pInfoQueue->AddStorageFilterEntries(&filter);
        }

        pInfoQueue->Release();
      }
    }
  }


  // Create default pass
  m_pDefaultPass = EZ_NEW(&m_Allocator, ezGALPassDX11, *this);

  if (FAILED(m_pDevice->QueryInterface(__uuidof(IDXGIDevice1), (void**)&m_pDXGIDevice)))
  {
    ezLog::Error("Couldn't get the DXGIDevice1 interface of the D3D11 device - this may happen when running on Windows Vista without SP2 "
                 "installed!");
    return EZ_FAILURE;
  }

  if (FAILED(m_pDevice->QueryInterface(__uuidof(ID3D11Device3), (void**)&m_pDevice3)))
  {
    ezLog::Info("D3D device doesn't support ID3D11Device3, some features might be unavailable.");
  }

  if (FAILED(m_pDXGIDevice->SetMaximumFrameLatency(1)))
  {
    ezLog::Warning("Failed to set max frames latency");
  }

  if (FAILED(m_pDXGIDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&m_pDXGIAdapter)))
  {
    return EZ_FAILURE;
  }

  if (FAILED(m_pDXGIAdapter->GetParent(__uuidof(IDXGIFactory1), (void**)&m_pDXGIFactory)))
  {
    return EZ_FAILURE;
  }

  // Fill lookup table
  FillFormatLookupTable();

  ezClipSpaceDepthRange::Default = ezClipSpaceDepthRange::ZeroToOne;

  // Per frame data & timer data
  D3D11_QUERY_DESC disjointQueryDesc;
  disjointQueryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
  disjointQueryDesc.MiscFlags = 0;

  D3D11_QUERY_DESC timerQueryDesc;
  timerQueryDesc.Query = D3D11_QUERY_TIMESTAMP;
  timerQueryDesc.MiscFlags = 0;

  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(m_PerFrameData); ++i)
  {
    auto& perFrameData = m_PerFrameData[i];
    perFrameData.m_pFence = CreateFencePlatform();

    if (FAILED(m_pDevice->CreateQuery(&disjointQueryDesc, &perFrameData.m_pDisjointTimerQuery)))
    {
      ezLog::Error("Creation of native DirectX query for disjoint query has failed!");
      return EZ_FAILURE;
    }
  }

  m_Timestamps.SetCountUninitialized(1024);
  for (ezUInt32 i = 0; i < m_Timestamps.GetCount(); ++i)
  {
    if (FAILED(m_pDevice->CreateQuery(&timerQueryDesc, &m_Timestamps[i])))
    {
      ezLog::Error("Creation of native DirectX query for timestamp has failed!");
      return EZ_FAILURE;
    }
  }

  m_SyncTimeDiff.SetZero();

  return EZ_SUCCESS;
}

ezResult ezGALDeviceDX11::InitPlatform()
{
  return InitPlatform(0, nullptr);
}

void ezGALDeviceDX11::ReportLiveGpuObjects()
{
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
  // not implemented
  return;

#else

  const HMODULE hDxgiDebugDLL = LoadLibraryW(L"Dxgidebug.dll");

  if (hDxgiDebugDLL == nullptr)
    return;

  typedef HRESULT(WINAPI * FnGetDebugInterfacePtr)(REFIID, void**);
  FnGetDebugInterfacePtr GetDebugInterfacePtr = (FnGetDebugInterfacePtr)GetProcAddress(hDxgiDebugDLL, "DXGIGetDebugInterface");

  if (GetDebugInterfacePtr == nullptr)
    return;

  IDXGIDebug* dxgiDebug = nullptr;
  GetDebugInterfacePtr(IID_PPV_ARGS(&dxgiDebug));

  if (dxgiDebug == nullptr)
    return;

  OutputDebugStringW(L" +++++ Live DX11 Objects: +++++\n");

  // prints to OutputDebugString
  dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);

  OutputDebugStringW(L" ----- Live DX11 Objects: -----\n");

  dxgiDebug->Release();

#endif
}

ezResult ezGALDeviceDX11::ShutdownPlatform()
{
  for (ezUInt32 type = 0; type < TempResourceType::ENUM_COUNT; ++type)
  {
    for (auto it = m_FreeTempResources[type].GetIterator(); it.IsValid(); ++it)
    {
      ezDynamicArray<ID3D11Resource*>& resources = it.Value();
      for (auto pResource : resources)
      {
        EZ_GAL_DX11_RELEASE(pResource);
      }
    }
    m_FreeTempResources[type].Clear();

    for (auto& tempResource : m_UsedTempResources[type])
    {
      EZ_GAL_DX11_RELEASE(tempResource.m_pResource);
    }
    m_UsedTempResources[type].Clear();
  }

  for (auto& timestamp : m_Timestamps)
  {
    EZ_GAL_DX11_RELEASE(timestamp);
  }
  m_Timestamps.Clear();

  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(m_PerFrameData); ++i)
  {
    auto& perFrameData = m_PerFrameData[i];

    DestroyFencePlatform(perFrameData.m_pFence);
    perFrameData.m_pFence = nullptr;

    EZ_GAL_DX11_RELEASE(perFrameData.m_pDisjointTimerQuery);
  }

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
  // Force immediate destruction of all objects destroyed so far.
  // This is necessary if we want to create a new primary swap chain/device right after this.
  // See: https://msdn.microsoft.com/en-us/library/windows/desktop/ff476425(v=vs.85).aspx#Defer_Issues_with_Flip
  // Strictly speaking we should do this right after we destroy the swap chain and flush all contexts that are affected.
  // However, the particular usecase where this problem comes up is usually a restart scenario.
  ezGALContextDX11* primaryContextDX = static_cast<ezGALContextDX11*>(m_pPrimaryContext);
  if (primaryContextDX)
  {
    primaryContextDX->GetDXContext()->ClearState();
    primaryContextDX->GetDXContext()->Flush();
  }
#endif

  m_pDefaultPass = nullptr;

  EZ_GAL_DX11_RELEASE(m_pImmediateContext);
  EZ_GAL_DX11_RELEASE(m_pDevice3);
  EZ_GAL_DX11_RELEASE(m_pDevice);
  EZ_GAL_DX11_RELEASE(m_pDebug);
  EZ_GAL_DX11_RELEASE(m_pDXGIFactory);
  EZ_GAL_DX11_RELEASE(m_pDXGIAdapter);
  EZ_GAL_DX11_RELEASE(m_pDXGIDevice);

  ReportLiveGpuObjects();

  return EZ_SUCCESS;
}

// Pass functions

ezGALPass* ezGALDeviceDX11::BeginPassPlatform(const char* szName)
{
  m_pDefaultPass->BeginPass(szName);

  return m_pDefaultPass.Borrow();
}

void ezGALDeviceDX11::EndPassPlatform(ezGALPass* pPass)
{
  EZ_ASSERT_DEV(m_pDefaultPass.Borrow() == pPass, "Invalid pass");

  m_pDefaultPass->EndPass();
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
}

void ezGALDeviceDX11::DestroyBlendStatePlatform(ezGALBlendState* pBlendState)
{
  ezGALBlendStateDX11* pState = static_cast<ezGALBlendStateDX11*>(pBlendState);
  pState->DeInitPlatform(this).IgnoreResult();
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
  pDX11DepthStencilState->DeInitPlatform(this).IgnoreResult();
  EZ_DELETE(&m_Allocator, pDX11DepthStencilState);
}

ezGALRasterizerState* ezGALDeviceDX11::CreateRasterizerStatePlatform(const ezGALRasterizerStateCreationDescription& Description)
{
  ezGALRasterizerStateDX11* pDX11RasterizerState = EZ_NEW(&m_Allocator, ezGALRasterizerStateDX11, Description);

  if (pDX11RasterizerState->InitPlatform(this).Succeeded())
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
  pDX11RasterizerState->DeInitPlatform(this).IgnoreResult();
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
  pDX11SamplerState->DeInitPlatform(this).IgnoreResult();
  EZ_DELETE(&m_Allocator, pDX11SamplerState);
}


// Resource creation functions

ezGALShader* ezGALDeviceDX11::CreateShaderPlatform(const ezGALShaderCreationDescription& Description)
{
  ezGALShaderDX11* pShader = EZ_NEW(&m_Allocator, ezGALShaderDX11, Description);

  if (!pShader->InitPlatform(this).Succeeded())
  {
    EZ_DELETE(&m_Allocator, pShader);
    return nullptr;
  }

  return pShader;
}

void ezGALDeviceDX11::DestroyShaderPlatform(ezGALShader* pShader)
{
  ezGALShaderDX11* pDX11Shader = static_cast<ezGALShaderDX11*>(pShader);
  pDX11Shader->DeInitPlatform(this).IgnoreResult();
  EZ_DELETE(&m_Allocator, pDX11Shader);
}

ezGALBuffer* ezGALDeviceDX11::CreateBufferPlatform(const ezGALBufferCreationDescription& Description, ezArrayPtr<const ezUInt8> pInitialData)
{
  ezGALBufferDX11* pBuffer = EZ_NEW(&m_Allocator, ezGALBufferDX11, Description);

  if (!pBuffer->InitPlatform(this, pInitialData).Succeeded())
  {
    EZ_DELETE(&m_Allocator, pBuffer);
    return nullptr;
  }

  return pBuffer;
}

void ezGALDeviceDX11::DestroyBufferPlatform(ezGALBuffer* pBuffer)
{
  ezGALBufferDX11* pDX11Buffer = static_cast<ezGALBufferDX11*>(pBuffer);
  pDX11Buffer->DeInitPlatform(this).IgnoreResult();
  EZ_DELETE(&m_Allocator, pDX11Buffer);
}

ezGALTexture* ezGALDeviceDX11::CreateTexturePlatform(const ezGALTextureCreationDescription& Description, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData)
{
  ezGALTextureDX11* pTexture = EZ_NEW(&m_Allocator, ezGALTextureDX11, Description);

  if (!pTexture->InitPlatform(this, pInitialData).Succeeded())
  {
    EZ_DELETE(&m_Allocator, pTexture);
    return nullptr;
  }

  return pTexture;
}

void ezGALDeviceDX11::DestroyTexturePlatform(ezGALTexture* pTexture)
{
  ezGALTextureDX11* pDX11Texture = static_cast<ezGALTextureDX11*>(pTexture);
  pDX11Texture->DeInitPlatform(this).IgnoreResult();
  EZ_DELETE(&m_Allocator, pDX11Texture);
}

ezGALResourceView* ezGALDeviceDX11::CreateResourceViewPlatform(ezGALResourceBase* pResource, const ezGALResourceViewCreationDescription& Description)
{
  ezGALResourceViewDX11* pResourceView = EZ_NEW(&m_Allocator, ezGALResourceViewDX11, pResource, Description);

  if (!pResourceView->InitPlatform(this).Succeeded())
  {
    EZ_DELETE(&m_Allocator, pResourceView);
    return nullptr;
  }

  return pResourceView;
}

void ezGALDeviceDX11::DestroyResourceViewPlatform(ezGALResourceView* pResourceView)
{
  ezGALResourceViewDX11* pDX11ResourceView = static_cast<ezGALResourceViewDX11*>(pResourceView);
  pDX11ResourceView->DeInitPlatform(this).IgnoreResult();
  EZ_DELETE(&m_Allocator, pDX11ResourceView);
}

ezGALRenderTargetView* ezGALDeviceDX11::CreateRenderTargetViewPlatform(ezGALTexture* pTexture, const ezGALRenderTargetViewCreationDescription& Description)
{
  ezGALRenderTargetViewDX11* pRTView = EZ_NEW(&m_Allocator, ezGALRenderTargetViewDX11, pTexture, Description);

  if (!pRTView->InitPlatform(this).Succeeded())
  {
    EZ_DELETE(&m_Allocator, pRTView);
    return nullptr;
  }

  return pRTView;
}

void ezGALDeviceDX11::DestroyRenderTargetViewPlatform(ezGALRenderTargetView* pRenderTargetView)
{
  ezGALRenderTargetViewDX11* pDX11RenderTargetView = static_cast<ezGALRenderTargetViewDX11*>(pRenderTargetView);
  pDX11RenderTargetView->DeInitPlatform(this).IgnoreResult();
  EZ_DELETE(&m_Allocator, pDX11RenderTargetView);
}

ezGALUnorderedAccessView* ezGALDeviceDX11::CreateUnorderedAccessViewPlatform(ezGALResourceBase* pTextureOfBuffer, const ezGALUnorderedAccessViewCreationDescription& Description)
{
  ezGALUnorderedAccessViewDX11* pUnorderedAccessView = EZ_NEW(&m_Allocator, ezGALUnorderedAccessViewDX11, pTextureOfBuffer, Description);

  if (!pUnorderedAccessView->InitPlatform(this).Succeeded())
  {
    EZ_DELETE(&m_Allocator, pUnorderedAccessView);
    return nullptr;
  }

  return pUnorderedAccessView;
}

void ezGALDeviceDX11::DestroyUnorderedAccessViewPlatform(ezGALUnorderedAccessView* pUnorderedAccessView)
{
  ezGALUnorderedAccessViewDX11* pUnorderedAccessViewDX11 = static_cast<ezGALUnorderedAccessViewDX11*>(pUnorderedAccessView);
  pUnorderedAccessViewDX11->DeInitPlatform(this).IgnoreResult();
  EZ_DELETE(&m_Allocator, pUnorderedAccessViewDX11);
}



// Other rendering creation functions
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
  pSwapChainDX11->DeInitPlatform(this).IgnoreResult();
  EZ_DELETE(&m_Allocator, pSwapChainDX11);
}

ezGALFence* ezGALDeviceDX11::CreateFencePlatform()
{
  ezGALFenceDX11* pFence = EZ_NEW(&m_Allocator, ezGALFenceDX11);

  if (!pFence->InitPlatform(this).Succeeded())
  {
    EZ_DELETE(&m_Allocator, pFence);
    return nullptr;
  }

  return pFence;
}

void ezGALDeviceDX11::DestroyFencePlatform(ezGALFence* pFence)
{
  ezGALFenceDX11* pFenceDX11 = static_cast<ezGALFenceDX11*>(pFence);
  pFenceDX11->DeInitPlatform(this).IgnoreResult();
  EZ_DELETE(&m_Allocator, pFenceDX11);
}

ezGALQuery* ezGALDeviceDX11::CreateQueryPlatform(const ezGALQueryCreationDescription& Description)
{
  ezGALQueryDX11* pQuery = EZ_NEW(&m_Allocator, ezGALQueryDX11, Description);

  if (!pQuery->InitPlatform(this).Succeeded())
  {
    EZ_DELETE(&m_Allocator, pQuery);
    return nullptr;
  }

  return pQuery;
}

void ezGALDeviceDX11::DestroyQueryPlatform(ezGALQuery* pQuery)
{
  ezGALQueryDX11* pQueryDX11 = static_cast<ezGALQueryDX11*>(pQuery);
  pQueryDX11->DeInitPlatform(this).IgnoreResult();
  EZ_DELETE(&m_Allocator, pQueryDX11);
}

ezGALVertexDeclaration* ezGALDeviceDX11::CreateVertexDeclarationPlatform(const ezGALVertexDeclarationCreationDescription& Description)
{
  ezGALVertexDeclarationDX11* pVertexDeclaration = EZ_NEW(&m_Allocator, ezGALVertexDeclarationDX11, Description);

  if (pVertexDeclaration->InitPlatform(this).Succeeded())
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
  ezGALVertexDeclarationDX11* pVertexDeclarationDX11 = static_cast<ezGALVertexDeclarationDX11*>(pVertexDeclaration);
  pVertexDeclarationDX11->DeInitPlatform(this).IgnoreResult();
  EZ_DELETE(&m_Allocator, pVertexDeclarationDX11);
}

ezGALTimestampHandle ezGALDeviceDX11::GetTimestampPlatform()
{
  ezUInt32 uiIndex = m_uiNextTimestamp;
  m_uiNextTimestamp = (m_uiNextTimestamp + 1) % m_Timestamps.GetCount();
  return {uiIndex, m_uiFrameCounter};
}

ezResult ezGALDeviceDX11::GetTimestampResultPlatform(ezGALTimestampHandle hTimestamp, ezTime& result)
{
  // Check whether frequency and sync timer are already available for the frame of the timestamp
  ezUInt64 uiFrameCounter = hTimestamp.m_uiFrameCounter;

  PerFrameData* pPerFrameData = nullptr;
  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(m_PerFrameData); ++i)
  {
    if (m_PerFrameData[i].m_uiFrame == uiFrameCounter && m_PerFrameData[i].m_fInvTicksPerSecond >= 0.0)
    {
      pPerFrameData = &m_PerFrameData[i];
      break;
    }
  }

  if (pPerFrameData == nullptr)
  {
    return EZ_FAILURE;
  }

  ID3D11Query* pQuery = GetTimestamp(hTimestamp);

  ezUInt64 uiTimestamp;
  if (FAILED(m_pImmediateContext->GetData(pQuery, &uiTimestamp, sizeof(uiTimestamp), D3D11_ASYNC_GETDATA_DONOTFLUSH)))
  {
    return EZ_FAILURE;
  }

  if (pPerFrameData->m_fInvTicksPerSecond == 0.0)
  {
    result.SetZero();
  }
  else
  {
    result = ezTime::Seconds(double(uiTimestamp) * pPerFrameData->m_fInvTicksPerSecond) + m_SyncTimeDiff;
  }
  return EZ_SUCCESS;
}

// Swap chain functions

void ezGALDeviceDX11::PresentPlatform(ezGALSwapChain* pSwapChain, bool bVSync)
{
  auto pDXSwapChain = static_cast<ezGALSwapChainDX11*>(pSwapChain);
  IDXGISwapChain* pDXGISwapChain = pDXSwapChain->GetDXSwapChain();

  // If there is a "actual backbuffer" (see it's documentation for detailed explanation), copy to it.
  if (!pDXSwapChain->m_hActualBackBufferTexture.IsInvalidated())
  {
    m_pDefaultPass->m_pRenderCommandEncoder->CopyTexture(pDXSwapChain->m_hActualBackBufferTexture, pDXSwapChain->m_hBackBufferTexture);
  }

  HRESULT result = pDXGISwapChain->Present(bVSync ? 1 : 0, 0);
  if (FAILED(result))
  {
    ezLog::Error("Swap chain Present failed with {0}", (ezUInt32)result);
    return;
  }

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
  // Since the swap chain can't be in discard mode, we do the discarding ourselves.
  ID3D11DeviceContext1* deviceContext1 = nullptr;
  if (FAILED(m_pImmediateContext->QueryInterface(&deviceContext1)))
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
#endif
}

// Misc functions

void ezGALDeviceDX11::BeginFramePlatform()
{
  auto& pCommandEncoder = m_pDefaultPass->m_pCommandEncoderImpl;

  // check if fence is reached and wait if the disjoint timer is about to be re-used
  {
    auto& perFrameData = m_PerFrameData[m_uiCurrentPerFrameData];
    if (perFrameData.m_uiFrame != ((ezUInt64)-1))
    {
      bool bFenceReached = pCommandEncoder->IsFenceReachedPlatform(perFrameData.m_pFence);
      if (!bFenceReached && m_uiNextPerFrameData == m_uiCurrentPerFrameData)
      {
        pCommandEncoder->WaitForFencePlatform(perFrameData.m_pFence);
      }
    }
  }

  {
    auto& perFrameData = m_PerFrameData[m_uiNextPerFrameData];
    m_pImmediateContext->Begin(perFrameData.m_pDisjointTimerQuery);

    perFrameData.m_fInvTicksPerSecond = -1.0f;
  }
}

void ezGALDeviceDX11::EndFramePlatform()
{
  auto& pCommandEncoder = m_pDefaultPass->m_pCommandEncoderImpl;

  // end disjoint query
  {
    auto& perFrameData = m_PerFrameData[m_uiNextPerFrameData];
    m_pImmediateContext->End(perFrameData.m_pDisjointTimerQuery);
  }

  // check if fence is reached and update per frame data
  {
    auto& perFrameData = m_PerFrameData[m_uiCurrentPerFrameData];
    if (perFrameData.m_uiFrame != ((ezUInt64)-1))
    {
      if (pCommandEncoder->IsFenceReachedPlatform(perFrameData.m_pFence))
      {
        FreeTempResources(perFrameData.m_uiFrame);

        D3D11_QUERY_DATA_TIMESTAMP_DISJOINT data;
        if (FAILED(m_pImmediateContext->GetData(perFrameData.m_pDisjointTimerQuery, &data, sizeof(data), D3D11_ASYNC_GETDATA_DONOTFLUSH)) || data.Disjoint)
        {
          perFrameData.m_fInvTicksPerSecond = 0.0f;
        }
        else
        {
          perFrameData.m_fInvTicksPerSecond = 1.0 / (double)data.Frequency;

          if (m_bSyncTimeNeeded)
          {
            ezGALTimestampHandle hTimestamp = m_pDefaultPass->m_pRenderCommandEncoder->InsertTimestamp();
            ID3D11Query* pQuery = GetTimestamp(hTimestamp);

            ezUInt64 uiTimestamp;
            while (m_pImmediateContext->GetData(pQuery, &uiTimestamp, sizeof(uiTimestamp), 0) != S_OK)
            {
              ezThreadUtils::YieldTimeSlice();
            }

            m_SyncTimeDiff = ezTime::Now() - ezTime::Seconds(double(uiTimestamp) * perFrameData.m_fInvTicksPerSecond);
            m_bSyncTimeNeeded = false;
          }
        }

        m_uiCurrentPerFrameData = (m_uiCurrentPerFrameData + 1) % EZ_ARRAY_SIZE(m_PerFrameData);
      }
    }
  }

  {
    auto& perFrameData = m_PerFrameData[m_uiNextPerFrameData];
    perFrameData.m_uiFrame = m_uiFrameCounter;

    // insert fence
    pCommandEncoder->InsertFencePlatform(perFrameData.m_pFence);

    m_uiNextPerFrameData = (m_uiNextPerFrameData + 1) % EZ_ARRAY_SIZE(m_PerFrameData);
  }

  ++m_uiFrameCounter;
}

void ezGALDeviceDX11::SetPrimarySwapChainPlatform(ezGALSwapChain* pSwapChain)
{
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
  // Make window association
  m_pDXGIFactory->MakeWindowAssociation(ezMinWindows::ToNative(pSwapChain->GetDescription().m_pWindow->GetNativeWindowHandle()), 0);
#endif
}


void ezGALDeviceDX11::FillCapabilitiesPlatform()
{
  {
    DXGI_ADAPTER_DESC1 adapterDesc;
    m_pDXGIAdapter->GetDesc1(&adapterDesc);

    m_Capabilities.m_sAdapterName = ezStringUtf8(adapterDesc.Description).GetData();
    m_Capabilities.m_uiDedicatedVRAM = static_cast<ezUInt64>(adapterDesc.DedicatedVideoMemory);
    m_Capabilities.m_uiDedicatedSystemRAM = static_cast<ezUInt64>(adapterDesc.DedicatedSystemMemory);
    m_Capabilities.m_uiSharedSystemRAM = static_cast<ezUInt64>(adapterDesc.SharedSystemMemory);
    m_Capabilities.m_bHardwareAccelerated = (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0;
  }

  m_Capabilities.m_bMultithreadedResourceCreation = true;

  switch (m_FeatureLevel)
  {
    case D3D_FEATURE_LEVEL_11_1:
      m_Capabilities.m_bB5G6R5Textures = true;
      m_Capabilities.m_bNoOverwriteBufferUpdate = true;

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

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  if (m_pDevice3)
  {
    D3D11_FEATURE_DATA_D3D11_OPTIONS2 featureOpts2;
    if (SUCCEEDED(m_pDevice3->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS2, &featureOpts2, sizeof(featureOpts2))))
    {
      m_Capabilities.m_bConservativeRasterization = (featureOpts2.ConservativeRasterizationTier != D3D11_CONSERVATIVE_RASTERIZATION_NOT_SUPPORTED);
    }
  }
}

ID3D11Resource* ezGALDeviceDX11::FindTempBuffer(ezUInt32 uiSize)
{
  const ezUInt32 uiExpGrowthLimit = 16 * 1024 * 1024;

  uiSize = ezMath::Max(uiSize, 256U);
  if (uiSize < uiExpGrowthLimit)
  {
    uiSize = ezMath::PowerOfTwo_Ceil(uiSize);
  }
  else
  {
    uiSize = ezMemoryUtils::AlignSize(uiSize, uiExpGrowthLimit);
  }

  ID3D11Resource* pResource = nullptr;
  auto it = m_FreeTempResources[TempResourceType::Buffer].Find(uiSize);
  if (it.IsValid())
  {
    ezDynamicArray<ID3D11Resource*>& resources = it.Value();
    if (!resources.IsEmpty())
    {
      pResource = resources[0];
      resources.RemoveAtAndSwap(0);
    }
  }

  if (pResource == nullptr)
  {
    D3D11_BUFFER_DESC desc;
    desc.ByteWidth = uiSize;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    ID3D11Buffer* pBuffer = nullptr;
    if (!SUCCEEDED(m_pDevice->CreateBuffer(&desc, nullptr, &pBuffer)))
    {
      return nullptr;
    }

    pResource = pBuffer;
  }

  auto& tempResource = m_UsedTempResources[TempResourceType::Buffer].ExpandAndGetRef();
  tempResource.m_pResource = pResource;
  tempResource.m_uiFrame = m_uiFrameCounter;
  tempResource.m_uiHash = uiSize;

  return pResource;
}


ID3D11Resource* ezGALDeviceDX11::FindTempTexture(ezUInt32 uiWidth, ezUInt32 uiHeight, ezUInt32 uiDepth, ezGALResourceFormat::Enum format)
{
  ezUInt32 data[] = {uiWidth, uiHeight, uiDepth, (ezUInt32)format};
  ezUInt32 uiHash = ezHashingUtils::xxHash32(data, sizeof(data));

  ID3D11Resource* pResource = nullptr;
  auto it = m_FreeTempResources[TempResourceType::Texture].Find(uiHash);
  if (it.IsValid())
  {
    ezDynamicArray<ID3D11Resource*>& resources = it.Value();
    if (!resources.IsEmpty())
    {
      pResource = resources[0];
      resources.RemoveAtAndSwap(0);
    }
  }

  if (pResource == nullptr)
  {
    if (uiDepth == 1)
    {
      D3D11_TEXTURE2D_DESC desc;
      desc.Width = uiWidth;
      desc.Height = uiHeight;
      desc.MipLevels = 1;
      desc.ArraySize = 1;
      desc.Format = GetFormatLookupTable().GetFormatInfo(format).m_eStorage;
      desc.SampleDesc.Count = 1;
      desc.SampleDesc.Quality = 0;
      desc.Usage = D3D11_USAGE_STAGING;
      desc.BindFlags = 0;
      desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
      desc.MiscFlags = 0;

      ID3D11Texture2D* pTexture = nullptr;
      if (!SUCCEEDED(m_pDevice->CreateTexture2D(&desc, nullptr, &pTexture)))
      {
        return nullptr;
      }

      pResource = pTexture;
    }
    else
    {
      EZ_ASSERT_NOT_IMPLEMENTED;
      return nullptr;
    }
  }

  auto& tempResource = m_UsedTempResources[TempResourceType::Texture].ExpandAndGetRef();
  tempResource.m_pResource = pResource;
  tempResource.m_uiFrame = m_uiFrameCounter;
  tempResource.m_uiHash = uiHash;

  return pResource;
}

void ezGALDeviceDX11::FreeTempResources(ezUInt64 uiFrame)
{
  for (ezUInt32 type = 0; type < TempResourceType::ENUM_COUNT; ++type)
  {
    while (!m_UsedTempResources[type].IsEmpty())
    {
      auto& usedTempResource = m_UsedTempResources[type].PeekFront();
      if (usedTempResource.m_uiFrame == uiFrame)
      {
        auto it = m_FreeTempResources[type].Find(usedTempResource.m_uiHash);
        if (!it.IsValid())
        {
          it = m_FreeTempResources[type].Insert(usedTempResource.m_uiHash, ezDynamicArray<ID3D11Resource*>(&m_Allocator));
        }

        it.Value().PushBack(usedTempResource.m_pResource);
        m_UsedTempResources[type].PopFront();
      }
      else
      {
        break;
      }
    }
  }
}

void ezGALDeviceDX11::FillFormatLookupTable()
{
  ///       The list below is in the same order as the ezGALResourceFormat enum. No format should be missing except the ones that are just
  ///       different names for the same enum value.

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGBAFloat, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32B32A32_TYPELESS).RT(DXGI_FORMAT_R32G32B32A32_FLOAT).VA(DXGI_FORMAT_R32G32B32A32_FLOAT).RV(DXGI_FORMAT_R32G32B32A32_FLOAT));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGBAUInt, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32B32A32_TYPELESS).RT(DXGI_FORMAT_R32G32B32A32_UINT).VA(DXGI_FORMAT_R32G32B32A32_UINT).RV(DXGI_FORMAT_R32G32B32A32_UINT));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGBAInt, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32B32A32_TYPELESS).RT(DXGI_FORMAT_R32G32B32A32_SINT).VA(DXGI_FORMAT_R32G32B32A32_SINT).RV(DXGI_FORMAT_R32G32B32A32_SINT));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGBFloat, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32B32_TYPELESS).RT(DXGI_FORMAT_R32G32B32_FLOAT).VA(DXGI_FORMAT_R32G32B32_FLOAT).RV(DXGI_FORMAT_R32G32B32_FLOAT));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGBUInt, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32B32_TYPELESS).RT(DXGI_FORMAT_R32G32B32_UINT).VA(DXGI_FORMAT_R32G32B32_UINT).RV(DXGI_FORMAT_R32G32B32_UINT));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGBInt, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32B32_TYPELESS).RT(DXGI_FORMAT_R32G32B32_SINT).VA(DXGI_FORMAT_R32G32B32_SINT).RV(DXGI_FORMAT_R32G32B32_SINT));

  // Supported with DX 11.1
  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::B5G6R5UNormalized, ezGALFormatLookupEntryDX11(DXGI_FORMAT_B5G6R5_UNORM).RT(DXGI_FORMAT_B5G6R5_UNORM).VA(DXGI_FORMAT_B5G6R5_UNORM).RV(DXGI_FORMAT_B5G6R5_UNORM));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::BGRAUByteNormalized, ezGALFormatLookupEntryDX11(DXGI_FORMAT_B8G8R8A8_TYPELESS).RT(DXGI_FORMAT_B8G8R8A8_UNORM).VA(DXGI_FORMAT_B8G8R8A8_UNORM).RV(DXGI_FORMAT_B8G8R8A8_UNORM));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::BGRAUByteNormalizedsRGB, ezGALFormatLookupEntryDX11(DXGI_FORMAT_B8G8R8A8_TYPELESS).RT(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB).RV(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGBAHalf, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16B16A16_TYPELESS).RT(DXGI_FORMAT_R16G16B16A16_FLOAT).VA(DXGI_FORMAT_R16G16B16A16_FLOAT).RV(DXGI_FORMAT_R16G16B16A16_FLOAT));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGBAUShort, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16B16A16_TYPELESS).RT(DXGI_FORMAT_R16G16B16A16_UINT).VA(DXGI_FORMAT_R16G16B16A16_UINT).RV(DXGI_FORMAT_R16G16B16A16_UINT));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGBAUShortNormalized, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16B16A16_TYPELESS).RT(DXGI_FORMAT_R16G16B16A16_UNORM).VA(DXGI_FORMAT_R16G16B16A16_UNORM).RV(DXGI_FORMAT_R16G16B16A16_UNORM));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGBAShort, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16B16A16_TYPELESS).RT(DXGI_FORMAT_R16G16B16A16_SINT).VA(DXGI_FORMAT_R16G16B16A16_SINT).RV(DXGI_FORMAT_R16G16B16A16_SINT));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGBAShortNormalized, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16B16A16_TYPELESS).RT(DXGI_FORMAT_R16G16B16A16_SNORM).VA(DXGI_FORMAT_R16G16B16A16_SNORM).RV(DXGI_FORMAT_R16G16B16A16_SNORM));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGFloat, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32_TYPELESS).RT(DXGI_FORMAT_R32G32_FLOAT).VA(DXGI_FORMAT_R32G32_FLOAT).RV(DXGI_FORMAT_R32G32_FLOAT));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGUInt, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32_TYPELESS).RT(DXGI_FORMAT_R32G32_UINT).VA(DXGI_FORMAT_R32G32_UINT).RV(DXGI_FORMAT_R32G32_UINT));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGInt, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32_TYPELESS).RT(DXGI_FORMAT_R32G32_SINT).VA(DXGI_FORMAT_R32G32_SINT).RV(DXGI_FORMAT_R32G32_SINT));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGB10A2UInt, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R10G10B10A2_TYPELESS).RT(DXGI_FORMAT_R10G10B10A2_UINT).VA(DXGI_FORMAT_R10G10B10A2_UINT).RV(DXGI_FORMAT_R10G10B10A2_UINT));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGB10A2UIntNormalized, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R10G10B10A2_TYPELESS).RT(DXGI_FORMAT_R10G10B10A2_UNORM).VA(DXGI_FORMAT_R10G10B10A2_UNORM).RV(DXGI_FORMAT_R10G10B10A2_UNORM));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RG11B10Float, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R11G11B10_FLOAT).RT(DXGI_FORMAT_R11G11B10_FLOAT).VA(DXGI_FORMAT_R11G11B10_FLOAT).RV(DXGI_FORMAT_R11G11B10_FLOAT));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGBAUByteNormalized, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8B8A8_TYPELESS).RT(DXGI_FORMAT_R8G8B8A8_UNORM).VA(DXGI_FORMAT_R8G8B8A8_UNORM).RV(DXGI_FORMAT_R8G8B8A8_UNORM));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGBAUByteNormalizedsRGB, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8B8A8_TYPELESS).RT(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB).RV(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGBAUByte, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8B8A8_TYPELESS).RT(DXGI_FORMAT_R8G8B8A8_UINT).VA(DXGI_FORMAT_R8G8B8A8_UINT).RV(DXGI_FORMAT_R8G8B8A8_UINT));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGBAByteNormalized, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8B8A8_TYPELESS).RT(DXGI_FORMAT_R8G8B8A8_SNORM).VA(DXGI_FORMAT_R8G8B8A8_SNORM).RV(DXGI_FORMAT_R8G8B8A8_SNORM));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGBAByte, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8B8A8_TYPELESS).RT(DXGI_FORMAT_R8G8B8A8_SINT).VA(DXGI_FORMAT_R8G8B8A8_SINT).RV(DXGI_FORMAT_R8G8B8A8_SINT));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGHalf, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16_TYPELESS).RT(DXGI_FORMAT_R16G16_FLOAT).VA(DXGI_FORMAT_R16G16_FLOAT).RV(DXGI_FORMAT_R16G16_FLOAT));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGUShort, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16_TYPELESS).RT(DXGI_FORMAT_R16G16_UINT).VA(DXGI_FORMAT_R16G16_UINT).RV(DXGI_FORMAT_R16G16_UINT));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGUShortNormalized, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16_TYPELESS).RT(DXGI_FORMAT_R16G16_UNORM).VA(DXGI_FORMAT_R16G16_UNORM).RV(DXGI_FORMAT_R16G16_UNORM));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGShort, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16_TYPELESS).RT(DXGI_FORMAT_R16G16_SINT).VA(DXGI_FORMAT_R16G16_SINT).RV(DXGI_FORMAT_R16G16_SINT));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGShortNormalized, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16_TYPELESS).RT(DXGI_FORMAT_R16G16_SNORM).VA(DXGI_FORMAT_R16G16_SNORM).RV(DXGI_FORMAT_R16G16_SNORM));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGUByte, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8_TYPELESS).RT(DXGI_FORMAT_R8G8_UINT).VA(DXGI_FORMAT_R8G8_UINT).RV(DXGI_FORMAT_R8G8_UINT));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGUByteNormalized, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8_TYPELESS).RT(DXGI_FORMAT_R8G8_UNORM).VA(DXGI_FORMAT_R8G8_UNORM).RV(DXGI_FORMAT_R8G8_UNORM));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGByte, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8_TYPELESS).RT(DXGI_FORMAT_R8G8_SINT).VA(DXGI_FORMAT_R8G8_SINT).RV(DXGI_FORMAT_R8G8_SINT));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGByteNormalized, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8_TYPELESS).RT(DXGI_FORMAT_R8G8_SNORM).VA(DXGI_FORMAT_R8G8_SNORM).RV(DXGI_FORMAT_R8G8_SNORM));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::DFloat, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R32_TYPELESS).RV(DXGI_FORMAT_R32_FLOAT).D(DXGI_FORMAT_R32_FLOAT).DS(DXGI_FORMAT_D32_FLOAT));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RFloat, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R32_TYPELESS).RT(DXGI_FORMAT_R32_FLOAT).VA(DXGI_FORMAT_R32_FLOAT).RV(DXGI_FORMAT_R32_FLOAT));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RUInt, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R32_TYPELESS).RT(DXGI_FORMAT_R32_UINT).VA(DXGI_FORMAT_R32_UINT).RV(DXGI_FORMAT_R32_UINT));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RInt, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R32_TYPELESS).RT(DXGI_FORMAT_R32_SINT).VA(DXGI_FORMAT_R32_SINT).RV(DXGI_FORMAT_R32_SINT));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RHalf, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R16_TYPELESS).RT(DXGI_FORMAT_R16_FLOAT).VA(DXGI_FORMAT_R16_FLOAT).RV(DXGI_FORMAT_R16_FLOAT));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RUShort, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R16_TYPELESS).RT(DXGI_FORMAT_R16_UINT).VA(DXGI_FORMAT_R16_UINT).RV(DXGI_FORMAT_R16_UINT));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RUShortNormalized, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R16_TYPELESS).RT(DXGI_FORMAT_R16_UNORM).VA(DXGI_FORMAT_R16_UNORM).RV(DXGI_FORMAT_R16_UNORM));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RShort, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R16_TYPELESS).RT(DXGI_FORMAT_R16_SINT).VA(DXGI_FORMAT_R16_SINT).RV(DXGI_FORMAT_R16_SINT));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RShortNormalized, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R16_TYPELESS).RT(DXGI_FORMAT_R16_SNORM).VA(DXGI_FORMAT_R16_SNORM).RV(DXGI_FORMAT_R16_SNORM));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RUByte, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R8_TYPELESS).RT(DXGI_FORMAT_R8_UINT).VA(DXGI_FORMAT_R8_UINT).RV(DXGI_FORMAT_R8_UINT));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RUByteNormalized, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R8_TYPELESS).RT(DXGI_FORMAT_R8_UNORM).VA(DXGI_FORMAT_R8_UNORM).RV(DXGI_FORMAT_R8_UNORM));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RByte, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R8_TYPELESS).RT(DXGI_FORMAT_R8_SINT).VA(DXGI_FORMAT_R8_SINT).RV(DXGI_FORMAT_R8_SINT));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RByteNormalized, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R8_TYPELESS).RT(DXGI_FORMAT_R8_SNORM).VA(DXGI_FORMAT_R8_SNORM).RV(DXGI_FORMAT_R8_SNORM));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::AUByteNormalized, ezGALFormatLookupEntryDX11(DXGI_FORMAT_A8_UNORM).RT(DXGI_FORMAT_A8_UNORM).VA(DXGI_FORMAT_A8_UNORM).RV(DXGI_FORMAT_A8_UNORM));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::D16, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R16_TYPELESS).RV(DXGI_FORMAT_R16_UNORM).DS(DXGI_FORMAT_D16_UNORM).D(DXGI_FORMAT_R16_UNORM));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::D24S8, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R24G8_TYPELESS).DS(DXGI_FORMAT_D24_UNORM_S8_UINT).D(DXGI_FORMAT_R24_UNORM_X8_TYPELESS).S(DXGI_FORMAT_X24_TYPELESS_G8_UINT));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::BC1, ezGALFormatLookupEntryDX11(DXGI_FORMAT_BC1_TYPELESS).RV(DXGI_FORMAT_BC1_UNORM));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::BC1sRGB, ezGALFormatLookupEntryDX11(DXGI_FORMAT_BC1_TYPELESS).RV(DXGI_FORMAT_BC1_UNORM_SRGB));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::BC2, ezGALFormatLookupEntryDX11(DXGI_FORMAT_BC2_TYPELESS).RV(DXGI_FORMAT_BC2_UNORM));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::BC2sRGB, ezGALFormatLookupEntryDX11(DXGI_FORMAT_BC2_TYPELESS).RV(DXGI_FORMAT_BC2_UNORM_SRGB));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::BC3, ezGALFormatLookupEntryDX11(DXGI_FORMAT_BC3_TYPELESS).RV(DXGI_FORMAT_BC3_UNORM));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::BC3sRGB, ezGALFormatLookupEntryDX11(DXGI_FORMAT_BC3_TYPELESS).RV(DXGI_FORMAT_BC3_UNORM_SRGB));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::BC4UNormalized, ezGALFormatLookupEntryDX11(DXGI_FORMAT_BC4_TYPELESS).RV(DXGI_FORMAT_BC4_UNORM));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::BC4Normalized, ezGALFormatLookupEntryDX11(DXGI_FORMAT_BC4_TYPELESS).RV(DXGI_FORMAT_BC4_SNORM));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::BC5UNormalized, ezGALFormatLookupEntryDX11(DXGI_FORMAT_BC5_TYPELESS).RV(DXGI_FORMAT_BC5_UNORM));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::BC5Normalized, ezGALFormatLookupEntryDX11(DXGI_FORMAT_BC5_TYPELESS).RV(DXGI_FORMAT_BC5_SNORM));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::BC6UFloat, ezGALFormatLookupEntryDX11(DXGI_FORMAT_BC6H_TYPELESS).RV(DXGI_FORMAT_BC6H_UF16));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::BC6Float, ezGALFormatLookupEntryDX11(DXGI_FORMAT_BC6H_TYPELESS).RV(DXGI_FORMAT_BC6H_SF16));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::BC7UNormalized, ezGALFormatLookupEntryDX11(DXGI_FORMAT_BC7_TYPELESS).RV(DXGI_FORMAT_BC7_UNORM));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::BC7UNormalizedsRGB, ezGALFormatLookupEntryDX11(DXGI_FORMAT_BC7_TYPELESS).RV(DXGI_FORMAT_BC7_UNORM_SRGB));
}



EZ_STATICLINK_FILE(RendererDX11, RendererDX11_Device_Implementation_DeviceDX11);
