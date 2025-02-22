#include <RendererDX11/RendererDX11PCH.h>

#include <Core/System/Window.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#include <Foundation/System/SystemInformation.h>
#include <RendererDX11/CommandEncoder/CommandEncoderImplDX11.h>
#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Device/SwapChainDX11.h>
#include <RendererDX11/Pools/FencePoolDX11.h>
#include <RendererDX11/Pools/QueryPoolDX11.h>
#include <RendererDX11/Resources/BufferDX11.h>
#include <RendererDX11/Resources/ReadbackBufferDX11.h>
#include <RendererDX11/Resources/ReadbackTextureDX11.h>
#include <RendererDX11/Resources/RenderTargetViewDX11.h>
#include <RendererDX11/Resources/ResourceViewDX11.h>
#include <RendererDX11/Resources/SharedTextureDX11.h>
#include <RendererDX11/Resources/TextureDX11.h>
#include <RendererDX11/Resources/UnorderedAccessViewDX11.h>
#include <RendererDX11/Shader/ShaderDX11.h>
#include <RendererDX11/Shader/VertexDeclarationDX11.h>
#include <RendererDX11/State/StateDX11.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/DeviceFactory.h>
#include <RendererFoundation/Profiling/Profiling.h>

#include <d3d11.h>
#include <d3d11_3.h>
#include <dxgidebug.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
#  include <d3d11_1.h>
#endif

ezInternal::NewInstance<ezGALDevice> CreateDX11Device(ezAllocator* pAllocator, const ezGALDeviceCreationDescription& description)
{
  return EZ_NEW(pAllocator, ezGALDeviceDX11, description);
}

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(RendererDX11, DeviceFactory)

ON_CORESYSTEMS_STARTUP
{
  ezGALDeviceFactory::RegisterCreatorFunc("DX11", &CreateDX11Device, "DX11_SM50", "ezShaderCompilerHLSL");
}

ON_CORESYSTEMS_SHUTDOWN
{
  ezGALDeviceFactory::UnregisterCreatorFunc("DX11");
}

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

ezGALDeviceDX11::ezGALDeviceDX11(const ezGALDeviceCreationDescription& Description)
  : ezGALDevice(Description)
  // NOLINTNEXTLINE
  , m_uiFeatureLevel(D3D_FEATURE_LEVEL_9_1)
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
    if (SUCCEEDED(D3D11CreateDevice(pUsedAdapter, driverType, nullptr, dwFlags, &FeatureLevels[FeatureLevelIdx], 1, D3D11_SDK_VERSION, &m_pDevice, (D3D_FEATURE_LEVEL*)&m_uiFeatureLevel, &pImmediateContext)))
    {
      break;
    }
  }

  // Nothing could be initialized:
  if (pImmediateContext == nullptr)
  {
    if (m_Description.m_bDebugDevice)
    {
      ezLog::Warning("Couldn't initialize D3D11 debug device!");

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

    static_assert(EZ_ARRAY_SIZE(FeatureLevels) == EZ_ARRAY_SIZE(FeatureLevelNames));

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
        if (ezSystemInformation::IsDebuggerAttached())
        {
          pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
          pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
          // pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, TRUE);
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
          filter.DenyList.NumIDs = EZ_ARRAY_SIZE(hide);
          filter.DenyList.pIDList = hide;
          pInfoQueue->AddStorageFilterEntries(&filter);
        }

        pInfoQueue->Release();
      }
    }
  }


  // Create default pass
  m_pCommandEncoderImpl = EZ_DEFAULT_NEW(ezGALCommandEncoderImplDX11, *this);
  m_pCommandEncoder = EZ_DEFAULT_NEW(ezGALCommandEncoder, *this, *m_pCommandEncoderImpl);

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

  ezFencePoolDX11::Initialize(this);
  m_pFenceQueue = EZ_NEW(&m_Allocator, ezFenceQueueDX11, &m_Allocator);

  // Fill lookup table
  FillFormatLookupTable();

  ezClipSpaceDepthRange::Default = ezClipSpaceDepthRange::ZeroToOne;
  ezClipSpaceYMode::RenderToTextureDefault = ezClipSpaceYMode::Regular;

  m_pQueryPool = EZ_NEW(&m_Allocator, ezQueryPoolDX11, this);
  if (m_pQueryPool->Initialize().Failed())
  {
    return EZ_FAILURE;
  }


  ezGALWindowSwapChain::SetFactoryMethod([this](const ezGALWindowSwapChainCreationDescription& desc) -> ezGALSwapChainHandle
    { return CreateSwapChain([&desc](ezAllocator* pAllocator) -> ezGALSwapChain*
        { return EZ_NEW(pAllocator, ezGALSwapChainDX11, desc); }); });

  return EZ_SUCCESS;
}

ezStringView ezGALDeviceDX11::GetRendererPlatform()
{
  return "DX11";
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

#elif EZ_ENABLED(EZ_PLATFORM_WINDOWS)

  const HMODULE hDxgiDebugDLL = LoadLibraryW(L"Dxgidebug.dll");

  if (hDxgiDebugDLL == nullptr)
    return;

  using FnGetDebugInterfacePtr = HRESULT(WINAPI*)(REFIID, void**);
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

void ezGALDeviceDX11::FlushDeadObjects()
{
  DestroyDeadObjects();
}

ezResult ezGALDeviceDX11::ShutdownPlatform()
{
  ezGALWindowSwapChain::SetFactoryMethod({});
  for (ezUInt32 type = 0; type < TempResourceType::ENUM_COUNT; ++type)
  {
    for (auto it = m_FreeTempResources[type].GetIterator(); it.IsValid(); ++it)
    {
      ezDynamicArray<TempResource>& tempResources = it.Value();
      for (auto tempResource : tempResources)
      {
        EZ_GAL_DX11_RELEASE(tempResource.m_pResource);
      }
    }
    m_FreeTempResources[type].Clear();

    for (auto& tempResource : m_UsedTempResources[type])
    {
      EZ_GAL_DX11_RELEASE(tempResource.m_pResource);
    }
    m_UsedTempResources[type].Clear();
  }

  m_pQueryPool->DeInitialize();
  m_pQueryPool = nullptr;
  m_pFenceQueue = nullptr;
  ezFencePoolDX11::DeInitialize();


#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
  // Force immediate destruction of all objects destroyed so far.
  // This is necessary if we want to create a new primary swap chain/device right after this.
  // See: https://msdn.microsoft.com/en-us/library/windows/desktop/ff476425(v=vs.85).aspx#Defer_Issues_with_Flip
  // Strictly speaking we should do this right after we destroy the swap chain and flush all contexts that are affected.
  // However, the particular usecase where this problem comes up is usually a restart scenario.
  if (m_pImmediateContext != nullptr)
  {
    m_pImmediateContext->ClearState();
    m_pImmediateContext->Flush();
  }
#endif

  m_pCommandEncoder = nullptr;
  m_pCommandEncoderImpl = nullptr;

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

// Command encoder functions

ezGALCommandEncoder* ezGALDeviceDX11::BeginCommandsPlatform(const char* szName)
{
#if EZ_ENABLED(EZ_USE_PROFILING)
  m_pPassTimingScope = ezProfilingScopeAndMarker::Start(m_pCommandEncoder.Borrow(), szName);
#else
  EZ_IGNORE_UNUSED(szName);
#endif

  return m_pCommandEncoder.Borrow();
}

void ezGALDeviceDX11::EndCommandsPlatform(ezGALCommandEncoder* pPass)
{
  EZ_ASSERT_DEV(m_pCommandEncoder.Borrow() == pPass, "Invalid pass");
  EZ_IGNORE_UNUSED(pPass);

#if EZ_ENABLED(EZ_USE_PROFILING)
  ezProfilingScopeAndMarker::Stop(m_pCommandEncoder.Borrow(), m_pPassTimingScope);
#endif
}

void ezGALDeviceDX11::FlushPlatform()
{
  m_pCommandEncoderImpl->FlushPlatform();
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

ezGALTexture* ezGALDeviceDX11::CreateSharedTexturePlatform(const ezGALTextureCreationDescription& Description, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData, ezEnum<ezGALSharedTextureType> sharedType, ezGALPlatformSharedHandle handle)
{
  ezGALSharedTextureDX11* pTexture = EZ_NEW(&m_Allocator, ezGALSharedTextureDX11, Description, sharedType, handle);

  if (!pTexture->InitPlatform(this, pInitialData).Succeeded())
  {
    EZ_DELETE(&m_Allocator, pTexture);
    return nullptr;
  }

  return pTexture;
}

void ezGALDeviceDX11::DestroySharedTexturePlatform(ezGALTexture* pTexture)
{
  ezGALSharedTextureDX11* pDX11Texture = static_cast<ezGALSharedTextureDX11*>(pTexture);
  pDX11Texture->DeInitPlatform(this).IgnoreResult();
  EZ_DELETE(&m_Allocator, pDX11Texture);
}

ezGALReadbackBuffer* ezGALDeviceDX11::CreateReadbackBufferPlatform(const ezGALBufferCreationDescription& Description)
{
  ezGALReadbackBufferDX11* pReadbackBuffer = EZ_NEW(&m_Allocator, ezGALReadbackBufferDX11, Description);

  if (!pReadbackBuffer->InitPlatform(this).Succeeded())
  {
    EZ_DELETE(&m_Allocator, pReadbackBuffer);
    return nullptr;
  }

  return pReadbackBuffer;
}

void ezGALDeviceDX11::DestroyReadbackBufferPlatform(ezGALReadbackBuffer* pReadbackBuffer)
{
  ezGALReadbackBufferDX11* pDX11ReadbackBuffer = static_cast<ezGALReadbackBufferDX11*>(pReadbackBuffer);

  pDX11ReadbackBuffer->DeInitPlatform(this).IgnoreResult();
  EZ_DELETE(&m_Allocator, pDX11ReadbackBuffer);
}

ezGALReadbackTexture* ezGALDeviceDX11::CreateReadbackTexturePlatform(const ezGALTextureCreationDescription& Description)
{
  ezGALReadbackTextureDX11* pReadbackTexture = EZ_NEW(&m_Allocator, ezGALReadbackTextureDX11, Description);

  if (!pReadbackTexture->InitPlatform(this).Succeeded())
  {
    EZ_DELETE(&m_Allocator, pReadbackTexture);
    return nullptr;
  }

  return pReadbackTexture;
}

void ezGALDeviceDX11::DestroyReadbackTexturePlatform(ezGALReadbackTexture* pReadbackTexture)
{
  ezGALReadbackTextureDX11* pDX11ReadbackTexture = static_cast<ezGALReadbackTextureDX11*>(pReadbackTexture);

  pDX11ReadbackTexture->DeInitPlatform(this).IgnoreResult();
  EZ_DELETE(&m_Allocator, pDX11ReadbackTexture);
}

ezGALTextureResourceView* ezGALDeviceDX11::CreateResourceViewPlatform(ezGALTexture* pResource, const ezGALTextureResourceViewCreationDescription& Description)
{
  ezGALTextureResourceViewDX11* pResourceView = EZ_NEW(&m_Allocator, ezGALTextureResourceViewDX11, pResource, Description);

  if (!pResourceView->InitPlatform(this).Succeeded())
  {
    EZ_DELETE(&m_Allocator, pResourceView);
    return nullptr;
  }

  return pResourceView;
}

void ezGALDeviceDX11::DestroyResourceViewPlatform(ezGALTextureResourceView* pResourceView)
{
  ezGALTextureResourceViewDX11* pDX11ResourceView = static_cast<ezGALTextureResourceViewDX11*>(pResourceView);
  pDX11ResourceView->DeInitPlatform(this).IgnoreResult();
  EZ_DELETE(&m_Allocator, pDX11ResourceView);
}

ezGALBufferResourceView* ezGALDeviceDX11::CreateResourceViewPlatform(ezGALBuffer* pResource, const ezGALBufferResourceViewCreationDescription& Description)
{
  ezGALBufferResourceViewDX11* pResourceView = EZ_NEW(&m_Allocator, ezGALBufferResourceViewDX11, pResource, Description);

  if (!pResourceView->InitPlatform(this).Succeeded())
  {
    EZ_DELETE(&m_Allocator, pResourceView);
    return nullptr;
  }

  return pResourceView;
}

void ezGALDeviceDX11::DestroyResourceViewPlatform(ezGALBufferResourceView* pResourceView)
{
  ezGALBufferResourceViewDX11* pDX11ResourceView = static_cast<ezGALBufferResourceViewDX11*>(pResourceView);
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

ezGALTextureUnorderedAccessView* ezGALDeviceDX11::CreateUnorderedAccessViewPlatform(ezGALTexture* pTextureOfBuffer, const ezGALTextureUnorderedAccessViewCreationDescription& Description)
{
  ezGALTextureUnorderedAccessViewDX11* pUnorderedAccessView = EZ_NEW(&m_Allocator, ezGALTextureUnorderedAccessViewDX11, pTextureOfBuffer, Description);

  if (!pUnorderedAccessView->InitPlatform(this).Succeeded())
  {
    EZ_DELETE(&m_Allocator, pUnorderedAccessView);
    return nullptr;
  }

  return pUnorderedAccessView;
}

void ezGALDeviceDX11::DestroyUnorderedAccessViewPlatform(ezGALTextureUnorderedAccessView* pUnorderedAccessView)
{
  ezGALTextureUnorderedAccessViewDX11* pUnorderedAccessViewDX11 = static_cast<ezGALTextureUnorderedAccessViewDX11*>(pUnorderedAccessView);
  pUnorderedAccessViewDX11->DeInitPlatform(this).IgnoreResult();
  EZ_DELETE(&m_Allocator, pUnorderedAccessViewDX11);
}

ezGALBufferUnorderedAccessView* ezGALDeviceDX11::CreateUnorderedAccessViewPlatform(ezGALBuffer* pBufferOfBuffer, const ezGALBufferUnorderedAccessViewCreationDescription& Description)
{
  ezGALBufferUnorderedAccessViewDX11* pUnorderedAccessView = EZ_NEW(&m_Allocator, ezGALBufferUnorderedAccessViewDX11, pBufferOfBuffer, Description);

  if (!pUnorderedAccessView->InitPlatform(this).Succeeded())
  {
    EZ_DELETE(&m_Allocator, pUnorderedAccessView);
    return nullptr;
  }

  return pUnorderedAccessView;
}

void ezGALDeviceDX11::DestroyUnorderedAccessViewPlatform(ezGALBufferUnorderedAccessView* pUnorderedAccessView)
{
  ezGALBufferUnorderedAccessViewDX11* pUnorderedAccessViewDX11 = static_cast<ezGALBufferUnorderedAccessViewDX11*>(pUnorderedAccessView);
  pUnorderedAccessViewDX11->DeInitPlatform(this).IgnoreResult();
  EZ_DELETE(&m_Allocator, pUnorderedAccessViewDX11);
}

// Other rendering creation functions

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

void ezGALDeviceDX11::UpdateBufferForNextFramePlatform(const ezGALBuffer* pBuffer, ezConstByteArrayPtr sourceData, ezUInt32 uiDestOffset)
{
  const ezGALBufferDX11* pBufferDX11 = static_cast<const ezGALBufferDX11*>(pBuffer);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  for (auto& copy : m_PendingCopies)
  {
    if (copy.m_pDestResource == pBufferDX11->GetDXBuffer())
    {
      const bool bWholeBuffer = copy.m_vSourceSize.x == ezInvalidIndex;
      const bool bOverlapping = uiDestOffset < copy.m_vDestPoint.x + copy.m_vSourceSize.x && copy.m_vDestPoint.x < uiDestOffset + sourceData.GetCount();
      if (bWholeBuffer || bOverlapping)
      {
        ezLog::Error("Buffer range is already updated for next frame.");
        return;
      }
    }
  }
#endif

  const ezUInt32 uiDestBufferSize = pBuffer->GetDescription().m_uiTotalSize;

  auto& copy = m_PendingCopies.ExpandAndGetRef();
  copy.m_SourceResource = CopyToTempBuffer(sourceData, m_uiFrameCounter + 1);
  copy.m_pDestResource = pBufferDX11->GetDXBuffer();
  copy.m_vDestPoint.Set(uiDestOffset, 0, 0);

  const bool bWholeBuffer = uiDestOffset == 0 && copy.m_SourceResource.m_uiRowPitch == uiDestBufferSize;
  copy.m_vSourceSize = bWholeBuffer ? ezVec3U32(ezInvalidIndex) : ezVec3U32(sourceData.GetCount(), 1, 1);
}

void ezGALDeviceDX11::UpdateTextureForNextFramePlatform(const ezGALTexture* pTexture, const ezGALSystemMemoryDescription& sourceData, const ezGALTextureSubresource& destinationSubResource, const ezBoundingBoxu32& destinationBox)
{
  const ezGALTextureDX11* pTextureDX11 = static_cast<const ezGALTextureDX11*>(pTexture);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  for (auto& copy : m_PendingCopies)
  {
    if (copy.m_pDestResource == pTextureDX11->GetDXTexture())
    {
      ezLog::Error("Texture is already updated for next frame.");
      return;
    }
  }
#endif

  auto& desc = pTexture->GetDescription();

  const ezUInt32 uiWidth = destinationBox.m_vMax.x - destinationBox.m_vMin.x;
  const ezUInt32 uiHeight = destinationBox.m_vMax.y - destinationBox.m_vMin.y;
  const ezUInt32 uiDepth = destinationBox.m_vMax.z - destinationBox.m_vMin.z;
  bool bWholeTexture = destinationBox.m_vMin.IsZero() && destinationBox.m_vMax == ezVec3U32(desc.m_uiWidth, desc.m_uiHeight, desc.m_uiDepth);

  auto& copy = m_PendingCopies.ExpandAndGetRef();
  copy.m_SourceResource = CopyToTempTexture(sourceData, uiWidth, uiHeight, uiDepth, desc.m_Format, m_uiFrameCounter + 1);
  copy.m_pDestResource = pTextureDX11->GetDXTexture();
  copy.m_uiDestSubResource = D3D11CalcSubresource(destinationSubResource.m_uiMipLevel, destinationSubResource.m_uiArraySlice, desc.m_uiMipLevelCount);
  copy.m_vDestPoint = destinationBox.m_vMin;
  copy.m_vSourceSize = bWholeTexture ? ezVec3U32(ezInvalidIndex) : ezVec3U32(uiWidth, uiHeight, uiDepth);
}

ezEnum<ezGALAsyncResult> ezGALDeviceDX11::GetTimestampResultPlatform(ezGALTimestampHandle hTimestamp, ezTime& out_result)
{
  return m_pQueryPool->GetTimestampResult(hTimestamp, out_result);
}

ezEnum<ezGALAsyncResult> ezGALDeviceDX11::GetOcclusionResultPlatform(ezGALOcclusionHandle hOcclusion, ezUInt64& out_uiResult)
{
  return m_pQueryPool->GetOcclusionQueryResult(hOcclusion, out_uiResult);
}

ezEnum<ezGALAsyncResult> ezGALDeviceDX11::GetFenceResultPlatform(ezGALFenceHandle hFence, ezTime timeout)
{
  if (m_pFenceQueue->GetCurrentFenceHandle() == hFence && timeout.IsPositive())
  {
    // Fence has not been submitted yet, force submit of the command buffer or we would deadlock here.
    Flush();
  }

  return m_pFenceQueue->GetFenceResult(hFence, timeout);
}

ezResult ezGALDeviceDX11::LockBufferPlatform(const ezGALReadbackBuffer* pBuffer, ezArrayPtr<const ezUInt8>& out_Memory) const
{
  const ezGALReadbackBufferDX11* pDXBuffer = static_cast<const ezGALReadbackBufferDX11*>(pBuffer);

  D3D11_MAPPED_SUBRESOURCE Mapped;
  HRESULT hr = GetDXImmediateContext()->Map(pDXBuffer->GetDXBuffer(), 0, D3D11_MAP_READ, 0, &Mapped);
  if (FAILED(hr))
  {
    return EZ_FAILURE;
  }
  out_Memory = ezArrayPtr<const ezUInt8>(reinterpret_cast<const ezUInt8*>(Mapped.pData), pBuffer->GetDescription().m_uiTotalSize);
  return EZ_SUCCESS;
}

void ezGALDeviceDX11::UnlockBufferPlatform(const ezGALReadbackBuffer* pBuffer) const
{
  const ezGALReadbackBufferDX11* pDXBuffer = static_cast<const ezGALReadbackBufferDX11*>(pBuffer);
  GetDXImmediateContext()->Unmap(pDXBuffer->GetDXBuffer(), 0);
}

ezResult ezGALDeviceDX11::LockTexturePlatform(const ezGALReadbackTexture* pTexture, const ezArrayPtr<const ezGALTextureSubresource>& subResources, ezDynamicArray<ezGALSystemMemoryDescription>& out_Memory) const
{
  out_Memory.Clear();
  const ezGALReadbackTextureDX11* pDXTexture = static_cast<const ezGALReadbackTextureDX11*>(pTexture);

  const ezUInt32 uiSubResources = subResources.GetCount();
  for (ezUInt32 i = 0; i < uiSubResources; i++)
  {
    const ezGALTextureSubresource& subRes = subResources[i];
    ezGALSystemMemoryDescription& memDesc = out_Memory.ExpandAndGetRef();
    const ezUInt32 uiSubResourceIndex = D3D11CalcSubresource(subRes.m_uiMipLevel, subRes.m_uiArraySlice, pTexture->GetDescription().m_uiMipLevelCount);

    D3D11_MAPPED_SUBRESOURCE Mapped;
    if (FAILED(GetDXImmediateContext()->Map(pDXTexture->GetDXTexture(), uiSubResourceIndex, D3D11_MAP_READ, 0, &Mapped)))
    {
      ezLog::Error("Failed to map sub resource with miplevel {} and array slice {}.", subRes.m_uiMipLevel, subRes.m_uiArraySlice);
    }
    else
    {
      switch (pTexture->GetDescription().m_Type)
      {
        case ezGALTextureType::Texture2D:
        case ezGALTextureType::Texture2DProxy:
        case ezGALTextureType::Texture2DShared:
        case ezGALTextureType::TextureCube:
          memDesc.m_pData = ezMakeByteBlobPtr(Mapped.pData, Mapped.RowPitch * pTexture->GetDescription().m_uiHeight);
          memDesc.m_uiRowPitch = Mapped.RowPitch;
          memDesc.m_uiSlicePitch = 0;
        case ezGALTextureType::Texture3D:
          memDesc.m_pData = ezMakeByteBlobPtr(Mapped.pData, Mapped.DepthPitch * pTexture->GetDescription().m_uiDepth);
          memDesc.m_uiRowPitch = Mapped.RowPitch;
          memDesc.m_uiSlicePitch = Mapped.DepthPitch;
          break;
        default:
          break;
      }
    }
  }
  return EZ_SUCCESS;
}

void ezGALDeviceDX11::UnlockTexturePlatform(const ezGALReadbackTexture* pTexture, const ezArrayPtr<const ezGALTextureSubresource>& subResources) const
{
  const ezGALReadbackTextureDX11* pDXTexture = static_cast<const ezGALReadbackTextureDX11*>(pTexture);

  const ezUInt32 uiSubResources = subResources.GetCount();
  for (ezUInt32 i = 0; i < uiSubResources; i++)
  {
    const ezGALTextureSubresource& subRes = subResources[i];
    const ezUInt32 uiSubResourceIndex = D3D11CalcSubresource(subRes.m_uiMipLevel, subRes.m_uiArraySlice, pTexture->GetDescription().m_uiMipLevelCount);

    GetDXImmediateContext()->Unmap(pDXTexture->GetDXTexture(), uiSubResourceIndex);
  }
}

// Swap chain functions

void ezGALDeviceDX11::PresentPlatform(const ezGALSwapChain* pSwapChain, bool bVSync)
{
  EZ_IGNORE_UNUSED(pSwapChain);
  EZ_IGNORE_UNUSED(bVSync);
}

// Misc functions

void ezGALDeviceDX11::BeginFramePlatform(ezArrayPtr<ezGALSwapChain*> swapchains, const ezUInt64 uiAppFrame)
{
  // check if fence is reached
  for (ezUInt64 uiFrame = m_uiSafeFrame + 1; uiFrame < m_uiFrameCounter; uiFrame++)
  {
    auto& perFrameData = m_PerFrameData[uiFrame % FRAMES];

    // if we accumulate more frames than we can hold in the ring buffer, force waiting for fences.
    const bool bForce = uiFrame % FRAMES == m_uiFrameCounter % FRAMES;
    if (perFrameData.m_uiFrame != ((ezUInt64)-1))
    {
      EZ_ASSERT_DEBUG(uiFrame == perFrameData.m_uiFrame, "Frame data was likely overwritten and no longer matches the expected previous frame index. This should have been prevented by bForce above.");
      bool bFenceReached = m_pFenceQueue->GetFenceResult(perFrameData.m_hFence) == ezGALAsyncResult::Ready;
      if (!bFenceReached && bForce)
      {
        m_pFenceQueue->GetFenceResult(perFrameData.m_hFence, ezTime::MakeFromHours(1));
        bFenceReached = true;
      }
      if (bFenceReached)
      {
        FreeTempResources(perFrameData.m_uiFrame);
        perFrameData.m_hFence = {};
        m_uiSafeFrame = uiFrame;
      }
      else
        break;
    }
  }

  EZ_ASSERT_DEBUG((m_uiFrameCounter % FRAMES) == m_uiCurrentPerFrameData, "");
  m_PerFrameData[m_uiCurrentPerFrameData].m_uiFrame = m_uiFrameCounter;

  m_pQueryPool->BeginFrame();

#if EZ_ENABLED(EZ_USE_PROFILING)
  ezStringBuilder sb;
  sb.SetFormat("RENDER FRAME {}", uiAppFrame);
  m_pFrameTimingScope = ezProfilingScopeAndMarker::Start(m_pCommandEncoder.Borrow(), sb);
#else
  EZ_IGNORE_UNUSED(uiAppFrame);
#endif

  ProcessPendingCopies();

  for (ezGALSwapChain* pSwapChain : swapchains)
  {
    pSwapChain->AcquireNextRenderTarget(this);
  }
}

void ezGALDeviceDX11::EndFramePlatform(ezArrayPtr<ezGALSwapChain*> swapchains)
{
  for (ezGALSwapChain* pSwapChain : swapchains)
  {
    pSwapChain->PresentRenderTarget(this);
  }

#if EZ_ENABLED(EZ_USE_PROFILING)
  ezProfilingScopeAndMarker::Stop(m_pCommandEncoder.Borrow(), m_pFrameTimingScope);
#endif

  m_pCommandEncoderImpl->EndFrame();
  m_pQueryPool->EndFrame();

  auto& currentFrameData = m_PerFrameData[m_uiCurrentPerFrameData];
  currentFrameData.m_hFence = m_pFenceQueue->SubmitCurrentFence();

  ++m_uiFrameCounter;
  m_uiCurrentPerFrameData = (m_uiFrameCounter) % FRAMES;
}

ezUInt64 ezGALDeviceDX11::GetCurrentFramePlatform() const
{
  return m_uiFrameCounter;
}

ezUInt64 ezGALDeviceDX11::GetSafeFramePlatform() const
{
  return m_uiSafeFrame;
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
    m_Capabilities.m_bSupportsTexelBuffer = true;
    m_Capabilities.m_bSupportsMultiSampledArrays = true;
  }

  m_Capabilities.m_bSupportsMultithreadedResourceCreation = true;

  switch (m_uiFeatureLevel)
  {
    case D3D_FEATURE_LEVEL_11_1:
      m_Capabilities.m_bSupportsNoOverwriteBufferUpdate = true;
      [[fallthrough]];

    case D3D_FEATURE_LEVEL_11_0:
      m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::VertexShader] = true;
      m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::HullShader] = true;
      m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::DomainShader] = true;
      m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::GeometryShader] = true;
      m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::PixelShader] = true;
      m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::ComputeShader] = true;
      m_Capabilities.m_bSupportsIndirectDraw = true;
      m_Capabilities.m_bSupportsSharedTextures = true;
      break;

      // not supported any longer

      // case D3D_FEATURE_LEVEL_10_1:
      // case D3D_FEATURE_LEVEL_10_0:
      //   m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::VertexShader] = true;
      //   m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::HullShader] = false;
      //   m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::DomainShader] = false;
      //   m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::GeometryShader] = true;
      //   m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::PixelShader] = true;
      //   m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::ComputeShader] = false;
      //   m_Capabilities.m_bSupportsIndirectDraw = false;
      //   m_Capabilities.m_bTextureArrays = true;
      //   m_Capabilities.m_bCubemapArrays = (m_uiFeatureLevel == D3D_FEATURE_LEVEL_10_1 ? true : false);
      //   break;

      // case D3D_FEATURE_LEVEL_9_3:
      //   m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::VertexShader] = true;
      //   m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::HullShader] = false;
      //   m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::DomainShader] = false;
      //   m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::GeometryShader] = false;
      //   m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::PixelShader] = true;
      //   m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::ComputeShader] = false;
      //   m_Capabilities.m_bSupportsIndirectDraw = false;
      //   m_Capabilities.m_bTextureArrays = false;
      //   m_Capabilities.m_bCubemapArrays = false;
      //   break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  if (m_pDevice3)
  {
    D3D11_FEATURE_DATA_D3D11_OPTIONS2 featureOpts2;
    if (SUCCEEDED(m_pDevice3->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS2, &featureOpts2, sizeof(featureOpts2))))
    {
      m_Capabilities.m_bSupportsConservativeRasterization = (featureOpts2.ConservativeRasterizationTier != D3D11_CONSERVATIVE_RASTERIZATION_NOT_SUPPORTED);
    }

    D3D11_FEATURE_DATA_D3D11_OPTIONS3 featureOpts3;
    if (SUCCEEDED(m_pDevice3->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS3, &featureOpts3, sizeof(featureOpts3))))
    {
      m_Capabilities.m_bSupportsVSRenderTargetArrayIndex = featureOpts3.VPAndRTArrayIndexFromAnyShaderFeedingRasterizer != 0;
    }
  }

  m_Capabilities.m_FormatSupport.SetCount(ezGALResourceFormat::ENUM_COUNT);
  for (ezUInt32 i = 0; i < ezGALResourceFormat::ENUM_COUNT; i++)
  {
    ezGALResourceFormat::Enum format = (ezGALResourceFormat::Enum)i;
    const ezGALFormatLookupEntryDX11& entry = m_FormatLookupTable.GetFormatInfo(format);
    const bool bIsDepth = ezGALResourceFormat::IsDepthFormat(format);
    if (bIsDepth)
    {
      UINT uiSampleSupport;
      if (SUCCEEDED(m_pDevice3->CheckFormatSupport(entry.m_eDepthOnlyType, &uiSampleSupport)))
      {
        if (uiSampleSupport & D3D11_FORMAT_SUPPORT::D3D11_FORMAT_SUPPORT_SHADER_SAMPLE)
          m_Capabilities.m_FormatSupport[i].Add(ezGALResourceFormatSupport::Texture);
      }

      UINT uiRenderSupport;
      if (SUCCEEDED(m_pDevice3->CheckFormatSupport(entry.m_eDepthStencilType, &uiRenderSupport)))
      {
        if (uiRenderSupport & D3D11_FORMAT_SUPPORT::D3D11_FORMAT_SUPPORT_DEPTH_STENCIL)
          m_Capabilities.m_FormatSupport[i].Add(ezGALResourceFormatSupport::RenderTarget);
      }
    }
    else
    {
      UINT uiSampleSupport;
      if (SUCCEEDED(m_pDevice3->CheckFormatSupport(entry.m_eResourceViewType, &uiSampleSupport)))
      {
        UINT uiSampleFlag = ezGALResourceFormat::IsIntegerFormat(format) ? D3D11_FORMAT_SUPPORT::D3D11_FORMAT_SUPPORT_SHADER_LOAD : D3D11_FORMAT_SUPPORT::D3D11_FORMAT_SUPPORT_SHADER_SAMPLE;
        if (uiSampleSupport & uiSampleFlag)
          m_Capabilities.m_FormatSupport[i].Add(ezGALResourceFormatSupport::Texture);

        if (uiSampleSupport & D3D11_FORMAT_SUPPORT::D3D11_FORMAT_SUPPORT_TYPED_UNORDERED_ACCESS_VIEW)
          m_Capabilities.m_FormatSupport[i].Add(ezGALResourceFormatSupport::TextureRW);
      }

      UINT uiVertexSupport;
      if (SUCCEEDED(m_pDevice3->CheckFormatSupport(entry.m_eVertexAttributeType, &uiVertexSupport)))
      {
        if (uiVertexSupport & D3D11_FORMAT_SUPPORT::D3D11_FORMAT_SUPPORT_IA_VERTEX_BUFFER)
          m_Capabilities.m_FormatSupport[i].Add(ezGALResourceFormatSupport::VertexAttribute);
      }

      UINT uiRenderSupport;
      if (SUCCEEDED(m_pDevice3->CheckFormatSupport(entry.m_eRenderTarget, &uiRenderSupport)))
      {
        if (uiRenderSupport & D3D11_FORMAT_SUPPORT::D3D11_FORMAT_SUPPORT_RENDER_TARGET)
          m_Capabilities.m_FormatSupport[i].Add(ezGALResourceFormatSupport::RenderTarget);
      }

      UINT uiMSAALevels;
      if (SUCCEEDED(m_pDevice3->CheckMultisampleQualityLevels(entry.m_eRenderTarget, 2, &uiMSAALevels)))
      {
        if (uiMSAALevels > 0)
          m_Capabilities.m_FormatSupport[i].Add(ezGALResourceFormatSupport::MSAA2x);
      }
      if (SUCCEEDED(m_pDevice3->CheckMultisampleQualityLevels(entry.m_eRenderTarget, 4, &uiMSAALevels)))
      {
        if (uiMSAALevels > 0)
          m_Capabilities.m_FormatSupport[i].Add(ezGALResourceFormatSupport::MSAA4x);
      }
      if (SUCCEEDED(m_pDevice3->CheckMultisampleQualityLevels(entry.m_eRenderTarget, 8, &uiMSAALevels)))
      {
        if (uiMSAALevels > 0)
          m_Capabilities.m_FormatSupport[i].Add(ezGALResourceFormatSupport::MSAA8x);
      }
    }
  }
}

void ezGALDeviceDX11::WaitIdlePlatform()
{
  m_pImmediateContext->Flush();
  DestroyDeadObjects();
}

const ezGALSharedTexture* ezGALDeviceDX11::GetSharedTexture(ezGALTextureHandle hTexture) const
{
  auto pTexture = GetTexture(hTexture);
  if (pTexture == nullptr)
  {
    return nullptr;
  }

  // Resolve proxy texture if any
  return static_cast<const ezGALSharedTextureDX11*>(pTexture->GetParentResource());
}

ezGALDeviceDX11::TempResource ezGALDeviceDX11::CopyToTempBuffer(ezConstByteArrayPtr sourceData, ezUInt64 uiLastUseFrame /*= ezUInt64(-1)*/)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  constexpr ezUInt32 uiExpGrowthLimit = 16 * 1024 * 1024;

  ezUInt32 uiSize = ezMath::Max(sourceData.GetCount(), 256U);
  if (uiSize < uiExpGrowthLimit)
  {
    uiSize = ezMath::PowerOfTwo_Ceil(uiSize);
  }
  else
  {
    uiSize = ezMemoryUtils::AlignSize(uiSize, uiExpGrowthLimit);
  }

  TempResource tempResource;
  auto it = m_FreeTempResources[TempResourceType::Buffer].Find(uiSize);
  if (it.IsValid())
  {
    ezDynamicArray<TempResource>& resources = it.Value();
    if (!resources.IsEmpty())
    {
      tempResource = resources[0];
      resources.RemoveAtAndSwap(0);
    }
  }

  if (tempResource.m_pResource == nullptr)
  {
    D3D11_BUFFER_DESC desc;
    desc.ByteWidth = uiSize;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA initData;
    initData.pSysMem = sourceData.GetPtr();
    initData.SysMemPitch = initData.SysMemSlicePitch = 0;

    if (uiSize > sourceData.GetCount())
    {
      ezUInt8* dataCopy = EZ_NEW_RAW_BUFFER(ezFrameAllocator::GetCurrentAllocator(), ezUInt8, uiSize);
      memcpy(dataCopy, sourceData.GetPtr(), sourceData.GetCount());
      initData.pSysMem = dataCopy;
    }

    ID3D11Buffer* pBuffer = nullptr;
    if (!SUCCEEDED(m_pDevice->CreateBuffer(&desc, &initData, &pBuffer)))
    {
      return {};
    }

    tempResource.m_pResource = pBuffer;
    tempResource.m_uiRowPitch = uiSize;
  }
  else
  {
    EZ_ASSERT_DEBUG(tempResource.m_pData != nullptr, "Must be mapped at this point");
    memcpy(tempResource.m_pData, sourceData.GetPtr(), sourceData.GetCount());
  }

  auto& usedTempResource = m_UsedTempResources[TempResourceType::Buffer].ExpandAndGetRef();
  usedTempResource.m_pResource = tempResource.m_pResource;
  usedTempResource.m_uiFrame = uiLastUseFrame != ezUInt64(-1) ? uiLastUseFrame : m_uiFrameCounter;
  usedTempResource.m_uiHash = uiSize;

  return tempResource;
}


ezGALDeviceDX11::TempResource ezGALDeviceDX11::CopyToTempTexture(const ezGALSystemMemoryDescription& sourceData, ezUInt32 uiWidth, ezUInt32 uiHeight, ezUInt32 uiDepth, ezGALResourceFormat::Enum format, ezUInt64 uiLastUseFrame /*= ezUInt64(-1)*/)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezUInt32 hashData[] = {uiWidth, uiHeight, uiDepth, (ezUInt32)format};
  ezUInt32 uiHash = ezHashingUtils::xxHash32(hashData, sizeof(hashData));

  TempResource tempResource;
  auto it = m_FreeTempResources[TempResourceType::Texture].Find(uiHash);
  if (it.IsValid())
  {
    ezDynamicArray<TempResource>& resources = it.Value();
    if (!resources.IsEmpty())
    {
      tempResource = resources[0];
      resources.RemoveAtAndSwap(0);
    }
  }

  if (tempResource.m_pResource == nullptr)
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

      D3D11_SUBRESOURCE_DATA initData;
      initData.pSysMem = sourceData.m_pData.GetPtr();
      initData.SysMemPitch = sourceData.m_uiRowPitch;
      initData.SysMemSlicePitch = sourceData.m_uiSlicePitch;

      ID3D11Texture2D* pTexture = nullptr;
      if (!SUCCEEDED(m_pDevice->CreateTexture2D(&desc, &initData, &pTexture)))
      {
        return {};
      }

      tempResource.m_pResource = pTexture;
    }
    else
    {
      EZ_ASSERT_NOT_IMPLEMENTED;
      return {};
    }
  }
  else
  {
    EZ_ASSERT_DEBUG(tempResource.m_pData != nullptr, "Must be mapped at this point");
    if (tempResource.m_uiRowPitch == sourceData.m_uiRowPitch && tempResource.m_uiDepthPitch == sourceData.m_uiSlicePitch)
    {
      memcpy(tempResource.m_pData, sourceData.m_pData.GetPtr(), sourceData.m_uiSlicePitch * uiDepth);
    }
    else
    {
      // Copy row by row
      for (ezUInt32 z = 0; z < uiDepth; ++z)
      {
        const void* pSource = ezMemoryUtils::AddByteOffset(sourceData.m_pData.GetPtr(), z * sourceData.m_uiSlicePitch);
        void* pDest = ezMemoryUtils::AddByteOffset(tempResource.m_pData, z * tempResource.m_uiDepthPitch);

        for (ezUInt32 y = 0; y < uiHeight; ++y)
        {
          memcpy(pDest, pSource, sourceData.m_uiRowPitch);

          pSource = ezMemoryUtils::AddByteOffset(pSource, sourceData.m_uiRowPitch);
          pDest = ezMemoryUtils::AddByteOffset(pDest, tempResource.m_uiRowPitch);
        }
      }
    }
  }

  auto& usedTempResource = m_UsedTempResources[TempResourceType::Buffer].ExpandAndGetRef();
  usedTempResource.m_pResource = tempResource.m_pResource;
  usedTempResource.m_uiFrame = uiLastUseFrame != ezUInt64(-1) ? uiLastUseFrame : m_uiFrameCounter;
  usedTempResource.m_uiHash = uiHash;

  return tempResource;
}

void ezGALDeviceDX11::UnmapTempResource(TempResource& tempResource)
{
  if (tempResource.m_pData != nullptr)
  {
    m_pImmediateContext->Unmap(tempResource.m_pResource, 0);
    tempResource.m_pData = nullptr;
    tempResource.m_uiRowPitch = 0;
    tempResource.m_uiDepthPitch = 0;
  }
}

void ezGALDeviceDX11::FreeTempResources(ezUInt64 uiFrame)
{
  for (ezUInt32 type = 0; type < TempResourceType::ENUM_COUNT; ++type)
  {
    auto& usedTempResources = m_UsedTempResources[type];

    for (ezUInt32 i = 0; i < usedTempResources.GetCount();)
    {
      UsedTempResource& usedTempResource = usedTempResources[i];
      if (usedTempResource.m_uiFrame <= uiFrame)
      {
        auto it = m_FreeTempResources[type].Find(usedTempResource.m_uiHash);
        if (!it.IsValid())
        {
          it = m_FreeTempResources[type].Insert(usedTempResource.m_uiHash, ezDynamicArray<TempResource>(&m_Allocator));
        }

        D3D11_MAPPED_SUBRESOURCE mapped;
        HRESULT hRes = m_pImmediateContext->Map(usedTempResource.m_pResource, 0, D3D11_MAP_WRITE, 0, &mapped);
        EZ_ASSERT_DEV(SUCCEEDED(hRes), "Implementation error");
        EZ_IGNORE_UNUSED(hRes);

        auto& tempResources = it.Value().ExpandAndGetRef();
        tempResources.m_pResource = usedTempResource.m_pResource;
        tempResources.m_pData = mapped.pData;
        tempResources.m_uiRowPitch = mapped.RowPitch;
        tempResources.m_uiDepthPitch = mapped.DepthPitch;

        usedTempResources.RemoveAtAndSwap(i);
      }
      else
      {
        ++i;
      }
    }
  }
}

void ezGALDeviceDX11::ProcessPendingCopies()
{
  EZ_PROFILE_AND_MARKER(GetCommandEncoder(), "PendingCopies");

  for (auto& copy : m_PendingCopies)
  {
    UnmapTempResource(copy.m_SourceResource);

    if (copy.m_vSourceSize == ezVec3U32(ezInvalidIndex))
    {
      m_pImmediateContext->CopyResource(copy.m_pDestResource, copy.m_SourceResource.m_pResource);
    }
    else
    {
      D3D11_BOX srcBox = {0, 0, 0, copy.m_vSourceSize.x, copy.m_vSourceSize.y, copy.m_vSourceSize.z};
      m_pImmediateContext->CopySubresourceRegion(copy.m_pDestResource, copy.m_uiDestSubResource, copy.m_vDestPoint.x, copy.m_vDestPoint.y, copy.m_vDestPoint.z, copy.m_SourceResource.m_pResource, 0, &srcBox);
    }
  }
  m_PendingCopies.Clear();
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

ezGALCommandEncoder* ezGALDeviceDX11::GetCommandEncoder() const
{
  return m_pCommandEncoder.Borrow();
}

ezFenceQueueDX11& ezGALDeviceDX11::GetFenceQueue() const
{
  return *m_pFenceQueue.Borrow();
}

ezQueryPoolDX11& ezGALDeviceDX11::GetQueryPool() const
{
  return *m_pQueryPool.Borrow();
}

EZ_STATICLINK_FILE(RendererDX11, RendererDX11_Device_Implementation_DeviceDX11);
