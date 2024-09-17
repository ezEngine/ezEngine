#include <RendererWebGPU/RendererWebGPUPCH.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/System/Window.h>
#include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/System/SystemInformation.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/DeviceFactory.h>
#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererWebGPU/CommandEncoder/CommandEncoderImplWebGPU.h>
#include <RendererWebGPU/Device/DeviceWebGPU.h>
#include <RendererWebGPU/Device/SwapChainWebGPU.h>
#include <RendererWebGPU/Resources/BufferWebGPU.h>
#include <RendererWebGPU/Resources/RenderTargetViewWebGPU.h>
#include <RendererWebGPU/Resources/ResourceViewWebGPU.h>
#include <RendererWebGPU/Resources/SharedTextureWebGPU.h>
#include <RendererWebGPU/Resources/TextureWebGPU.h>
#include <RendererWebGPU/Resources/UnorderedAccessViewWebGPU.h>
#include <RendererWebGPU/Shader/ShaderWebGPU.h>
#include <RendererWebGPU/Shader/VertexDeclarationWebGPU.h>
#include <RendererWebGPU/State/StateWebGPU.h>

ezInternal::NewInstance<ezGALDevice> CreateWebGPUDevice(ezAllocator* pAllocator, const ezGALDeviceCreationDescription& description)
{
  return EZ_NEW(pAllocator, ezGALDeviceWebGPU, description);
}

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(RendererWebGPU, DeviceFactory)

ON_CORESYSTEMS_STARTUP
{
  ezGALDeviceFactory::RegisterCreatorFunc("WebGPU", &CreateWebGPUDevice, "WGSL", "ezShaderCompilerWebGPU");
}

ON_CORESYSTEMS_SHUTDOWN
{
  ezGALDeviceFactory::UnregisterCreatorFunc("WebGPU");
}

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

ezGALDeviceWebGPU::ezGALDeviceWebGPU(const ezGALDeviceCreationDescription& Description)
  : ezGALDevice(Description)
{
}

ezGALDeviceWebGPU::~ezGALDeviceWebGPU() = default;

// Init & shutdown functions

ezStringView ezGALDeviceWebGPU::GetRendererPlatform()
{
  EZ_WEBGPU_TRACE();
  return "WebGPU";
}

void ezGALDeviceWebGPU::WebGPUErrorCallback(WGPUErrorType type, char const* szMessage, void* pUserdata)
{
  switch (type)
  {
    case WGPUErrorType_DeviceLost:
      ezLog::Error("WebGPU Device Lost: {}", szMessage);
      break;
    case WGPUErrorType_Internal:
      ezLog::Error("WebGPU Internal Error: {}", szMessage);
      break;
    case WGPUErrorType_NoError:
      ezLog::Info("WebGPU: {}", szMessage);
      break;
    case WGPUErrorType_OutOfMemory:
      ezLog::Error("WebGPU Out Of Memory: {}", szMessage);
      break;
    case WGPUErrorType_Unknown:
      ezLog::Error("WebGPU: {}", szMessage);
      break;
    case WGPUErrorType_Validation:
      ezLog::SeriousWarning("WebGPU Validation: {}", szMessage);
      break;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}

EZ_DEFINE_AS_POD_TYPE(wgpu::FeatureName);


static wgpu::Instance s_pInstance;
static wgpu::Adapter s_Adapter;
static wgpu::Device s_Device;

EZ_ON_GLOBAL_EVENT(WebApp_PreInit)
{
  bool* pInitDone = reinterpret_cast<bool*>(param0.Get<void*>());

  if (s_pInstance == nullptr)
  {
    ezGALDeviceWebGPU::PreInitWebGPU();
  }

  if (s_Device == nullptr)
  {
    *pInitDone = false;
  }
}

void ezGALDeviceWebGPU::PreInitWebGPU()
{
  if (s_pInstance != nullptr)
    return;

  s_pInstance = wgpu::CreateInstance();

  wgpu::RequestAdapterOptions opt;
  opt.powerPreference = wgpu::PowerPreference::HighPerformance;

  s_pInstance.RequestAdapter(
    &opt,
    [](WGPURequestAdapterStatus status, WGPUAdapter adapter, const char* message, void* userdata)
    {
      if (status != WGPURequestAdapterStatus_Success)
      {
        ezLog::Error("WebGPU Adapter: {}", message);
      }
      else
      {
        ezLog::Success("Got WebGPU Adapter");

        s_Adapter = wgpu::Adapter::Acquire(adapter);

        {
          ezHybridArray<wgpu::FeatureName, 32> features;
          features.PushBack(wgpu::FeatureName::DepthClipControl);
          features.PushBack(wgpu::FeatureName::TextureCompressionBC);

          wgpu::DeviceDescriptor desc;
          desc.requiredFeatures = features.GetData();
          desc.requiredFeatureCount = features.GetCount();

#if EZ_DISABLED(EZ_PLATFORM_WEB)
          desc.uncapturedErrorCallbackInfo.callback = ezGALDeviceWebGPU::WebGPUErrorCallback;
#endif

          s_Adapter.RequestDevice(
            &desc, [](WGPURequestDeviceStatus status, WGPUDevice device, const char* message, void* userdata)
            {
              if (status != WGPURequestDeviceStatus_Success)
              {
                ezLog::Error("WebGPU Device: {}", message);
              }
              else
              {
                ezLog::Success("Got WebGPU Device");

                s_Device = wgpu::Device::Acquire(device);
              } },
            nullptr);
        }
      }
    },
    nullptr);
}

ezResult ezGALDeviceWebGPU::InitPlatform()
{
  EZ_WEBGPU_TRACE();

  // WebGPU cannot upload resources on anything other than the main thread
  ezResource::UpdateGraphicsResource = ezResource::DoUpdate::OnMainThread;

  PreInitWebGPU();

  m_Instance = s_pInstance;
  m_Adapter = s_Adapter;
  m_Device = s_Device;

  ezGALWindowSwapChain::SetFactoryMethod([this](const ezGALWindowSwapChainCreationDescription& desc) -> ezGALSwapChainHandle
    { return CreateSwapChain([&desc](ezAllocator* pAllocator) -> ezGALSwapChain*
        { return EZ_NEW(pAllocator, ezGALSwapChainWebGPU, desc); }); });

  m_pCommandEncoderImpl = EZ_DEFAULT_NEW(ezGALCommandEncoderImplWebGPU, *this);
  m_pCommandEncoder = EZ_DEFAULT_NEW(ezGALCommandEncoder, *this, *m_pCommandEncoderImpl);

  return EZ_SUCCESS;
}

ezResult ezGALDeviceWebGPU::ShutdownPlatform()
{
  EZ_WEBGPU_TRACE();

  m_pCommandEncoder = nullptr;
  m_pCommandEncoderImpl = nullptr;

  m_Instance = nullptr;

  return EZ_SUCCESS;
}

// Command encoder functions

ezGALCommandEncoder* ezGALDeviceWebGPU::BeginCommandsPlatform(const char* szName)
{
  EZ_WEBGPU_TRACE();

#if EZ_ENABLED(EZ_USE_PROFILING)
  // TODO WebGPU: m_pPassTimingScope = ezProfilingScopeAndMarker::Start(m_pCommandEncoder.Borrow(), szName);
#else
  EZ_IGNORE_UNUSED(szName);
#endif

  return m_pCommandEncoder.Borrow();
}

void ezGALDeviceWebGPU::EndCommandsPlatform(ezGALCommandEncoder* pPass)
{
  EZ_WEBGPU_TRACE();

  EZ_ASSERT_DEV(m_pCommandEncoder.Borrow() == pPass, "Invalid pass");
  EZ_IGNORE_UNUSED(pPass);

#if EZ_ENABLED(EZ_USE_PROFILING)
  // TODO WebGPU: ezProfilingScopeAndMarker::Stop(m_pCommandEncoder.Borrow(), m_pPassTimingScope);
#endif
}

void ezGALDeviceWebGPU::FlushPlatform()
{
  EZ_WEBGPU_TRACE();
}

// State creation functions

ezGALBlendState* ezGALDeviceWebGPU::CreateBlendStatePlatform(const ezGALBlendStateCreationDescription& Description)
{
  ezGALBlendStateWebGPU* pState = EZ_NEW(&m_Allocator, ezGALBlendStateWebGPU, Description);

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

void ezGALDeviceWebGPU::DestroyBlendStatePlatform(ezGALBlendState* pBlendState)
{
  ezGALBlendStateWebGPU* pState = static_cast<ezGALBlendStateWebGPU*>(pBlendState);
  pState->DeInitPlatform(this).IgnoreResult();
  EZ_DELETE(&m_Allocator, pState);
}

ezGALDepthStencilState* ezGALDeviceWebGPU::CreateDepthStencilStatePlatform(const ezGALDepthStencilStateCreationDescription& Description)
{
  ezGALDepthStencilStateWebGPU* pWebGPUDepthStencilState = EZ_NEW(&m_Allocator, ezGALDepthStencilStateWebGPU, Description);

  if (pWebGPUDepthStencilState->InitPlatform(this).Succeeded())
  {
    return pWebGPUDepthStencilState;
  }
  else
  {
    EZ_DELETE(&m_Allocator, pWebGPUDepthStencilState);
    return nullptr;
  }
}

void ezGALDeviceWebGPU::DestroyDepthStencilStatePlatform(ezGALDepthStencilState* pDepthStencilState)
{
  ezGALDepthStencilStateWebGPU* pWebGPUDepthStencilState = static_cast<ezGALDepthStencilStateWebGPU*>(pDepthStencilState);
  pWebGPUDepthStencilState->DeInitPlatform(this).IgnoreResult();
  EZ_DELETE(&m_Allocator, pWebGPUDepthStencilState);
}

ezGALRasterizerState* ezGALDeviceWebGPU::CreateRasterizerStatePlatform(const ezGALRasterizerStateCreationDescription& Description)
{
  ezGALRasterizerStateWebGPU* pWebGPURasterizerState = EZ_NEW(&m_Allocator, ezGALRasterizerStateWebGPU, Description);

  if (pWebGPURasterizerState->InitPlatform(this).Succeeded())
  {
    return pWebGPURasterizerState;
  }
  else
  {
    EZ_DELETE(&m_Allocator, pWebGPURasterizerState);
    return nullptr;
  }
}

void ezGALDeviceWebGPU::DestroyRasterizerStatePlatform(ezGALRasterizerState* pRasterizerState)
{
  ezGALRasterizerStateWebGPU* pWebGPURasterizerState = static_cast<ezGALRasterizerStateWebGPU*>(pRasterizerState);
  pWebGPURasterizerState->DeInitPlatform(this).IgnoreResult();
  EZ_DELETE(&m_Allocator, pWebGPURasterizerState);
}

ezGALSamplerState* ezGALDeviceWebGPU::CreateSamplerStatePlatform(const ezGALSamplerStateCreationDescription& Description)
{
  ezGALSamplerStateWebGPU* pWebGPUSamplerState = EZ_NEW(&m_Allocator, ezGALSamplerStateWebGPU, Description);

  if (pWebGPUSamplerState->InitPlatform(this).Succeeded())
  {
    return pWebGPUSamplerState;
  }
  else
  {
    EZ_DELETE(&m_Allocator, pWebGPUSamplerState);
    return nullptr;
  }
}

void ezGALDeviceWebGPU::DestroySamplerStatePlatform(ezGALSamplerState* pSamplerState)
{
  ezGALSamplerStateWebGPU* pWebGPUSamplerState = static_cast<ezGALSamplerStateWebGPU*>(pSamplerState);
  pWebGPUSamplerState->DeInitPlatform(this).IgnoreResult();
  EZ_DELETE(&m_Allocator, pWebGPUSamplerState);
}


// Resource creation functions

ezGALShader* ezGALDeviceWebGPU::CreateShaderPlatform(const ezGALShaderCreationDescription& Description)
{
  ezGALShaderWebGPU* pShader = EZ_NEW(&m_Allocator, ezGALShaderWebGPU, Description);

  if (!pShader->InitPlatform(this).Succeeded())
  {
    EZ_DELETE(&m_Allocator, pShader);
    return nullptr;
  }

  return pShader;
}

void ezGALDeviceWebGPU::DestroyShaderPlatform(ezGALShader* pShader)
{
  ezGALShaderWebGPU* pWebGPUShader = static_cast<ezGALShaderWebGPU*>(pShader);
  pWebGPUShader->DeInitPlatform(this).IgnoreResult();
  EZ_DELETE(&m_Allocator, pWebGPUShader);
}

ezGALBuffer* ezGALDeviceWebGPU::CreateBufferPlatform(const ezGALBufferCreationDescription& Description, ezArrayPtr<const ezUInt8> pInitialData)
{
  ezGALBufferWebGPU* pBuffer = EZ_NEW(&m_Allocator, ezGALBufferWebGPU, Description);

  if (!pBuffer->InitPlatform(this, pInitialData).Succeeded())
  {
    EZ_DELETE(&m_Allocator, pBuffer);
    return nullptr;
  }

  return pBuffer;
}

void ezGALDeviceWebGPU::DestroyBufferPlatform(ezGALBuffer* pBuffer)
{
  ezGALBufferWebGPU* pWebGPUBuffer = static_cast<ezGALBufferWebGPU*>(pBuffer);
  pWebGPUBuffer->DeInitPlatform(this).IgnoreResult();
  EZ_DELETE(&m_Allocator, pWebGPUBuffer);
}

ezGALTexture* ezGALDeviceWebGPU::CreateTexturePlatform(const ezGALTextureCreationDescription& Description, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData)
{
  ezGALTextureWebGPU* pTexture = EZ_NEW(&m_Allocator, ezGALTextureWebGPU, Description);

  if (!pTexture->InitPlatform(this, pInitialData).Succeeded())
  {
    EZ_DELETE(&m_Allocator, pTexture);
    return nullptr;
  }

  return pTexture;
}

void ezGALDeviceWebGPU::DestroyTexturePlatform(ezGALTexture* pTexture)
{
  ezGALTextureWebGPU* pWebGPUTexture = static_cast<ezGALTextureWebGPU*>(pTexture);
  pWebGPUTexture->DeInitPlatform(this).IgnoreResult();
  EZ_DELETE(&m_Allocator, pWebGPUTexture);
}

ezGALTexture* ezGALDeviceWebGPU::CreateSharedTexturePlatform(const ezGALTextureCreationDescription& Description, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData, ezEnum<ezGALSharedTextureType> sharedType, ezGALPlatformSharedHandle handle)
{
  ezGALSharedTextureWebGPU* pTexture = EZ_NEW(&m_Allocator, ezGALSharedTextureWebGPU, Description, sharedType, handle);

  if (!pTexture->InitPlatform(this, pInitialData).Succeeded())
  {
    EZ_DELETE(&m_Allocator, pTexture);
    return nullptr;
  }

  return pTexture;
}

void ezGALDeviceWebGPU::DestroySharedTexturePlatform(ezGALTexture* pTexture)
{
  ezGALSharedTextureWebGPU* pWebGPUTexture = static_cast<ezGALSharedTextureWebGPU*>(pTexture);
  pWebGPUTexture->DeInitPlatform(this).IgnoreResult();
  EZ_DELETE(&m_Allocator, pWebGPUTexture);
}

ezGALTextureResourceView* ezGALDeviceWebGPU::CreateResourceViewPlatform(ezGALTexture* pResource, const ezGALTextureResourceViewCreationDescription& Description)
{
  ezGALTextureResourceViewWebGPU* pResourceView = EZ_NEW(&m_Allocator, ezGALTextureResourceViewWebGPU, pResource, Description);

  if (!pResourceView->InitPlatform(this).Succeeded())
  {
    EZ_DELETE(&m_Allocator, pResourceView);
    return nullptr;
  }

  return pResourceView;
}

void ezGALDeviceWebGPU::DestroyResourceViewPlatform(ezGALTextureResourceView* pResourceView)
{
  ezGALTextureResourceViewWebGPU* pWebGPUResourceView = static_cast<ezGALTextureResourceViewWebGPU*>(pResourceView);
  pWebGPUResourceView->DeInitPlatform(this).IgnoreResult();
  EZ_DELETE(&m_Allocator, pWebGPUResourceView);
}

ezGALBufferResourceView* ezGALDeviceWebGPU::CreateResourceViewPlatform(ezGALBuffer* pResource, const ezGALBufferResourceViewCreationDescription& Description)
{
  ezGALBufferResourceViewWebGPU* pResourceView = EZ_NEW(&m_Allocator, ezGALBufferResourceViewWebGPU, pResource, Description);

  if (!pResourceView->InitPlatform(this).Succeeded())
  {
    EZ_DELETE(&m_Allocator, pResourceView);
    return nullptr;
  }

  return pResourceView;
}

void ezGALDeviceWebGPU::DestroyResourceViewPlatform(ezGALBufferResourceView* pResourceView)
{
  ezGALBufferResourceViewWebGPU* pWebGPUResourceView = static_cast<ezGALBufferResourceViewWebGPU*>(pResourceView);
  pWebGPUResourceView->DeInitPlatform(this).IgnoreResult();
  EZ_DELETE(&m_Allocator, pWebGPUResourceView);
}

ezGALRenderTargetView* ezGALDeviceWebGPU::CreateRenderTargetViewPlatform(ezGALTexture* pTexture, const ezGALRenderTargetViewCreationDescription& Description)
{
  ezGALRenderTargetViewWebGPU* pRTView = EZ_NEW(&m_Allocator, ezGALRenderTargetViewWebGPU, pTexture, Description);

  if (!pRTView->InitPlatform(this).Succeeded())
  {
    EZ_DELETE(&m_Allocator, pRTView);
    return nullptr;
  }

  return pRTView;
}

void ezGALDeviceWebGPU::DestroyRenderTargetViewPlatform(ezGALRenderTargetView* pRenderTargetView)
{
  ezGALRenderTargetViewWebGPU* pWebGPURenderTargetView = static_cast<ezGALRenderTargetViewWebGPU*>(pRenderTargetView);
  pWebGPURenderTargetView->DeInitPlatform(this).IgnoreResult();
  EZ_DELETE(&m_Allocator, pWebGPURenderTargetView);
}

ezGALTextureUnorderedAccessView* ezGALDeviceWebGPU::CreateUnorderedAccessViewPlatform(ezGALTexture* pTextureOfBuffer, const ezGALTextureUnorderedAccessViewCreationDescription& Description)
{
  ezGALTextureUnorderedAccessViewWebGPU* pUnorderedAccessView = EZ_NEW(&m_Allocator, ezGALTextureUnorderedAccessViewWebGPU, pTextureOfBuffer, Description);

  if (!pUnorderedAccessView->InitPlatform(this).Succeeded())
  {
    EZ_DELETE(&m_Allocator, pUnorderedAccessView);
    return nullptr;
  }

  return pUnorderedAccessView;
}

void ezGALDeviceWebGPU::DestroyUnorderedAccessViewPlatform(ezGALTextureUnorderedAccessView* pUnorderedAccessView)
{
  ezGALTextureUnorderedAccessViewWebGPU* pUnorderedAccessViewWebGPU = static_cast<ezGALTextureUnorderedAccessViewWebGPU*>(pUnorderedAccessView);
  pUnorderedAccessViewWebGPU->DeInitPlatform(this).IgnoreResult();
  EZ_DELETE(&m_Allocator, pUnorderedAccessViewWebGPU);
}

ezGALBufferUnorderedAccessView* ezGALDeviceWebGPU::CreateUnorderedAccessViewPlatform(ezGALBuffer* pBufferOfBuffer, const ezGALBufferUnorderedAccessViewCreationDescription& Description)
{
  ezGALBufferUnorderedAccessViewWebGPU* pUnorderedAccessView = EZ_NEW(&m_Allocator, ezGALBufferUnorderedAccessViewWebGPU, pBufferOfBuffer, Description);

  if (!pUnorderedAccessView->InitPlatform(this).Succeeded())
  {
    EZ_DELETE(&m_Allocator, pUnorderedAccessView);
    return nullptr;
  }

  return pUnorderedAccessView;
}

void ezGALDeviceWebGPU::DestroyUnorderedAccessViewPlatform(ezGALBufferUnorderedAccessView* pUnorderedAccessView)
{
  ezGALBufferUnorderedAccessViewWebGPU* pUnorderedAccessViewWebGPU = static_cast<ezGALBufferUnorderedAccessViewWebGPU*>(pUnorderedAccessView);
  pUnorderedAccessViewWebGPU->DeInitPlatform(this).IgnoreResult();
  EZ_DELETE(&m_Allocator, pUnorderedAccessViewWebGPU);
}

// Other rendering creation functions

ezGALVertexDeclaration* ezGALDeviceWebGPU::CreateVertexDeclarationPlatform(const ezGALVertexDeclarationCreationDescription& Description)
{
  ezGALVertexDeclarationWebGPU* pVertexDeclaration = EZ_NEW(&m_Allocator, ezGALVertexDeclarationWebGPU, Description);

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

void ezGALDeviceWebGPU::DestroyVertexDeclarationPlatform(ezGALVertexDeclaration* pVertexDeclaration)
{
  ezGALVertexDeclarationWebGPU* pVertexDeclarationWebGPU = static_cast<ezGALVertexDeclarationWebGPU*>(pVertexDeclaration);
  pVertexDeclarationWebGPU->DeInitPlatform(this).IgnoreResult();
  EZ_DELETE(&m_Allocator, pVertexDeclarationWebGPU);
}

ezEnum<ezGALAsyncResult> ezGALDeviceWebGPU::GetTimestampResultPlatform(ezGALTimestampHandle hTimestamp, ezTime& out_result)
{
  EZ_WEBGPU_TRACE();

  return {}; // m_pQueryPool->GetTimestampResult(hTimestamp, out_result);
}

ezEnum<ezGALAsyncResult> ezGALDeviceWebGPU::GetOcclusionResultPlatform(ezGALOcclusionHandle hOcclusion, ezUInt64& out_uiResult)
{
  EZ_WEBGPU_TRACE();

  return {}; // m_pQueryPool->GetOcclusionQueryResult(hOcclusion, out_uiResult);
}

ezEnum<ezGALAsyncResult> ezGALDeviceWebGPU::GetFenceResultPlatform(ezGALFenceHandle hFence, ezTime timeout)
{
  EZ_WEBGPU_TRACE();

  return {}; // m_pFenceQueue->GetFenceResult(hFence, timeout);
}

// Swap chain functions

void ezGALDeviceWebGPU::PresentPlatform(const ezGALSwapChain* pSwapChain, bool bVSync)
{
  EZ_WEBGPU_TRACE();
}

// Misc functions

void ezGALDeviceWebGPU::BeginFramePlatform(ezArrayPtr<ezGALSwapChain*> swapchains, const ezUInt64 uiAppFrame)
{
  EZ_WEBGPU_TRACE();

#if EZ_ENABLED(EZ_USE_PROFILING)
  ezStringBuilder sb;
  sb.SetFormat("Frame {}", uiAppFrame);
  // TODO WebGPU: m_pFrameTimingScope = ezProfilingScopeAndMarker::Start(m_pCommandEncoder.Borrow(), sb);
#else
  EZ_IGNORE_UNUSED(uiAppFrame);
#endif

  for (ezGALSwapChain* pSwapChain : swapchains)
  {
    pSwapChain->AcquireNextRenderTarget(this);
  }
}

void ezGALDeviceWebGPU::EndFramePlatform(ezArrayPtr<ezGALSwapChain*> swapchains)
{
  EZ_WEBGPU_TRACE();

  for (ezGALSwapChain* pSwapChain : swapchains)
  {
    pSwapChain->PresentRenderTarget(this);
  }

#if EZ_DISABLED(EZ_PLATFORM_WEB)
  m_Instance.ProcessEvents();
#endif

#if EZ_ENABLED(EZ_USE_PROFILING)
  // TODO WebGPU: ezProfilingScopeAndMarker::Stop(m_pCommandEncoder.Borrow(), m_pFrameTimingScope);
#endif
}

ezUInt64 ezGALDeviceWebGPU::GetCurrentFramePlatform() const
{
  EZ_WEBGPU_TRACE();

  return 0;
}

ezUInt64 ezGALDeviceWebGPU::GetSafeFramePlatform() const
{
  EZ_WEBGPU_TRACE();

  return 0;
}

void ezGALDeviceWebGPU::FillCapabilitiesPlatform()
{
  // EZ_WEBGPU_TRACE();

  wgpu::SupportedLimits limits;
  m_Device.GetLimits(&limits);
  const auto& l = limits.limits;

#if EZ_DISABLED(EZ_PLATFORM_WEB)
  wgpu::AdapterInfo info;
  m_Device.GetAdapter().GetInfo(&info);

  // TODO WebGPU: adapter capabilities
  // TODO: these capabilities are barely used, clean them up
  m_Capabilities.m_sAdapterName = info.device;
  m_Capabilities.m_bHardwareAccelerated = (info.adapterType != wgpu::AdapterType::CPU);
#else
  m_Capabilities.m_sAdapterName = "Browser";
  m_Capabilities.m_bHardwareAccelerated = true;
#endif

  m_Capabilities.m_bMultithreadedResourceCreation = true; // assume yes ?
  m_Capabilities.m_b32BitIndices = true;
  m_Capabilities.m_bAlphaToCoverage = false;              // don't know
  m_Capabilities.m_bInstancing = true;
  m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::ComputeShader] = true;
  m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::PixelShader] = true;
  m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::VertexShader] = true;
  m_Capabilities.m_bTextureArrays = true;
  m_Capabilities.m_uiMax3DTextureDimension = (ezUInt16)l.maxTextureDimension3D;
  m_Capabilities.m_uiMaxRendertargets = (ezUInt16)l.maxColorAttachments;
}

void ezGALDeviceWebGPU::WaitIdlePlatform()
{
  EZ_WEBGPU_TRACE();

  DestroyDeadObjects();
}

const ezGALSharedTexture* ezGALDeviceWebGPU::GetSharedTexture(ezGALTextureHandle hTexture) const
{
  EZ_WEBGPU_TRACE();

  auto pTexture = GetTexture(hTexture);
  if (pTexture == nullptr)
  {
    return nullptr;
  }

  // Resolve proxy texture if any
  return static_cast<const ezGALSharedTextureWebGPU*>(pTexture->GetParentResource());
}

EZ_STATICLINK_FILE(RendererWebGPU, RendererWebGPU_Device_Implementation_DeviceWebGPU);
