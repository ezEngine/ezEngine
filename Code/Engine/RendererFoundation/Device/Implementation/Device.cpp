
#include <RendererFoundation/PCH.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/State/State.h>
#include <RendererFoundation/Shader/Shader.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/Texture.h>
#include <RendererFoundation/Context/Context.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Logging/Log.h>

ezGALDevice* ezGALDevice::s_pDefaultDevice = nullptr;

ezGALDevice::ezGALDevice(const ezGALDeviceCreationDescription& Description)
  : m_Allocator("GALDevice", ezFoundation::GetDefaultAllocator()),
    m_AllocatorWrapper(&m_Allocator),
    m_Description(Description),
    m_pPrimaryContext(nullptr),
    m_bFrameBeginCalled(false)
{
}

ezGALDevice::~ezGALDevice()
{
  // Check for object leaks
  {
    EZ_LOG_BLOCK("ezGALDevice object leak report");

    if (!m_Shaders.IsEmpty())
      ezLog::Warning("%d shaders have not been cleaned up", m_Shaders.GetCount());

    if (!m_BlendStates.IsEmpty())
      ezLog::Warning("%d blend states have not been cleaned up", m_BlendStates.GetCount());

    if (!m_DepthStencilStates.IsEmpty())
      ezLog::Warning("%d depth stencil states have not been cleaned up", m_DepthStencilStates.GetCount());

    if (!m_RasterizerStates.IsEmpty())
      ezLog::Warning("%d rasterizer states have not been cleaned up", m_RasterizerStates.GetCount());

    if (!m_Buffers.IsEmpty())
      ezLog::Warning("%d buffers have not been cleaned up", m_Buffers.GetCount());

    if (!m_Textures.IsEmpty())
      ezLog::Warning("%d textures have not been cleaned up", m_Textures.GetCount());

    if (!m_ResourceViews.IsEmpty())
      ezLog::Warning("%d resource views have not been cleaned up", m_ResourceViews.GetCount());

    if (!m_RenderTargetViews.IsEmpty())
      ezLog::Warning("%d render target views have not been cleaned up", m_RenderTargetViews.GetCount());

    if (!m_SwapChains.IsEmpty())
      ezLog::Warning("%d swap chains have not been cleaned up", m_SwapChains.GetCount());

    if (!m_Fences.IsEmpty())
      ezLog::Warning("%d fences have not been cleaned up", m_Fences.GetCount());

    if (!m_Queries.IsEmpty())
      ezLog::Warning("%d queries have not been cleaned up", m_Queries.GetCount());

    if (!m_VertexDeclarations.IsEmpty())
      ezLog::Warning("%d vertex declarations have not been cleaned up", m_VertexDeclarations.GetCount());
  }
}

ezResult ezGALDevice::Init()
{
  EZ_LOG_BLOCK("ezGALDevice::Init");

  ezResult PlatformInitResult = InitPlatform();

  if (PlatformInitResult == EZ_FAILURE)
  {
    return EZ_FAILURE;
  }

  // Fill the capabilities
  FillCapabilitiesPlatform();

  // Create primary swapchain if requested
  if (m_Description.m_bCreatePrimarySwapChain)
  {
    ezGALSwapChainHandle hSwapChain = CreateSwapChain(m_Description.m_PrimarySwapChainDescription);

    if (hSwapChain.IsInvalidated())
    {
      ezLog::Error("Primary swap chain couldn't be created!");
      return EZ_FAILURE;
    }

    // And make it the primary swap chain
    SetPrimarySwapChain(hSwapChain);
  }

  return EZ_SUCCESS;
}

ezResult ezGALDevice::Shutdown()
{
  EZ_LOG_BLOCK("ezGALDevice::Shutdown");

  // If we created a primary swap chain, release it
  if (!m_hPrimarySwapChain.IsInvalidated())
  {
    DestroySwapChain(m_hPrimarySwapChain);
    m_hPrimarySwapChain.Invalidate();
  }

  return ShutdownPlatform();
}

/// \todo State / resource creation needs to check if multithreaded resource creation is allowed and if not if it is in the render thread
// There may also be the need for a mutex protecting the various creation / destroy functions since they access non-threadsafe data structures

ezGALBlendStateHandle ezGALDevice::CreateBlendState(const ezGALBlendStateCreationDescription& Description)
{
  // Hash description and return potential existing one (including inc. refcount)
  ezUInt32 uiHash = Description.CalculateHash();

  auto it = m_BlendStateMap.Find(uiHash);
  if (it.IsValid())
  {
    ezGALBlendState* pBlendState = m_BlendStates[it.Value()];
    pBlendState->AddRef();
    return it.Value();
  }

  ezGALBlendState* pBlendState = CreateBlendStatePlatform(Description);

  if (pBlendState != nullptr)
  {
    pBlendState->AddRef();

    ezGALBlendStateHandle hHandle(m_BlendStates.Insert(pBlendState));
    m_BlendStateMap[uiHash] = hHandle;

    return hHandle;
  }

  return ezGALBlendStateHandle();
}

void ezGALDevice::DestroyBlendState(ezGALBlendStateHandle hBlendState)
{
  ezGALBlendState* pBlendState = nullptr;

  if (m_BlendStates.TryGetValue(hBlendState, pBlendState))
  {
    pBlendState->ReleaseRef();

    if (pBlendState->GetRefCount() == 0)
    {
      m_BlendStates.Remove(hBlendState);
      m_BlendStateMap.Remove(pBlendState->GetDescription().CalculateHash());

      DestroyBlendStatePlatform(pBlendState);
    }
  }
  else
  {
    ezLog::Warning("DestroyBlendState called on invalid handle (double free?)");
  }
}

ezGALDepthStencilStateHandle ezGALDevice::CreateDepthStencilState(const ezGALDepthStencilStateCreationDescription& Description)
{
  /// \todo Hash description and return potential existing one (including inc. refcount)

  ezGALDepthStencilState* pDepthStencilState = CreateDepthStencilStatePlatform(Description);


  if (pDepthStencilState == nullptr)
  {
    return ezGALDepthStencilStateHandle();
  }
  else
  {
    return ezGALDepthStencilStateHandle(m_DepthStencilStates.Insert(pDepthStencilState));
  }

  return ezGALDepthStencilStateHandle();
}

void ezGALDevice::DestroyDepthStencilState(ezGALDepthStencilStateHandle hDepthStencilState)
{
  ezGALDepthStencilState* pDepthStencilState = nullptr;

  /// \todo Only remove if refcount = 0

  if (m_DepthStencilStates.Remove(hDepthStencilState, &pDepthStencilState))
  {
    DestroyDepthStencilStatePlatform(pDepthStencilState);
  }
  else
  {
    ezLog::Warning("DestroyDepthStencilState called on invalid handle (double free?)");
  }
}

ezGALRasterizerStateHandle ezGALDevice::CreateRasterizerState(const ezGALRasterizerStateCreationDescription& Description)
{
  /// \todo Hash description and return potential existing one (including inc. refcount)

  ezGALRasterizerState* pRasterizerState = CreateRasterizerStatePlatform(Description);

  if (pRasterizerState == nullptr)
  {
    return ezGALRasterizerStateHandle();
  }
  else
  {
    return ezGALRasterizerStateHandle(m_RasterizerStates.Insert(pRasterizerState));
  }

  return ezGALRasterizerStateHandle();
}

void ezGALDevice::DestroyRasterizerState(ezGALRasterizerStateHandle hRasterizerState)
{
  ezGALRasterizerState* pRasterizerState = nullptr;

  /// \todo Only remove if refcount = 0

  if (m_RasterizerStates.Remove(hRasterizerState, &pRasterizerState))
  {
    DestroyRasterizerStatePlatform(pRasterizerState);
  }
  else
  {
    ezLog::Warning("DestroyRasterizerState called on invalid handle (double free?)");
  }
}

ezGALSamplerStateHandle ezGALDevice::CreateSamplerState(const ezGALSamplerStateCreationDescription& Description)
{
  /// \todo Platform independent validation
  /// \todo Hash description and return potential existing one (including inc. refcount)

  ezGALSamplerState* pSamplerState = CreateSamplerStatePlatform(Description);

  if (pSamplerState == nullptr)
  {
    return ezGALSamplerStateHandle();
  }
  else
  {
    return ezGALSamplerStateHandle(m_SamplerStates.Insert(pSamplerState));
  }
}

void ezGALDevice::DestroySamplerState(ezGALSamplerStateHandle hSamplerState)
{
  ezGALSamplerState* pSamplerState = nullptr;

  /// \todo Only remove if refcount = 0

  if (m_SamplerStates.Remove(hSamplerState, &pSamplerState))
  {
    DestroySamplerStatePlatform(pSamplerState);
  }
  else
  {
    ezLog::Warning("DestroySamplerState called on invalid handle (double free?)");
  }
}



ezGALShaderHandle ezGALDevice::CreateShader(const ezGALShaderCreationDescription& Description)
{
  bool bHasByteCodes = false;

  for (ezUInt32 uiStage = 0; uiStage < ezGALShaderStage::ENUM_COUNT; uiStage++)
  {
    if (Description.HasByteCodeForStage((ezGALShaderStage::Enum)uiStage))
    {
      bHasByteCodes = true;
      break;
    }
  }

  if (!bHasByteCodes)
  {
    ezLog::Error("Can't create a shader which supplies no bytecodes at all!");
    return ezGALShaderHandle();
  }

  ezGALShader* pShader = CreateShaderPlatform(Description);

  if (pShader == nullptr)
  {
    return ezGALShaderHandle();
  }
  else
  {
    return ezGALShaderHandle(m_Shaders.Insert(pShader));
  }
}

void ezGALDevice::DestroyShader(ezGALShaderHandle hShader)
{
  ezGALShader* pShader = nullptr;

  if (m_Shaders.Remove(hShader, &pShader))
  {
    DestroyShaderPlatform(pShader);
  }
  else
  {
    ezLog::Warning("DestroyShader called on invalid handle (double free?)");
  }
}


ezGALBufferHandle ezGALDevice::CreateBuffer(const ezGALBufferCreationDescription& Description, const void* pInitialData)
{
  if (Description.m_uiTotalSize == 0)
  {
    ezLog::Error("Trying to create a buffer with size of 0 is not possible!");
    return ezGALBufferHandle();
  }

  if (Description.m_ResourceAccess.IsImmutable() && pInitialData == nullptr)
  {
    ezLog::Error("Trying to create an immutable buffer but not supplying initial data is not possible!");
    return ezGALBufferHandle();
  }

  /// \todo Platform independent validation (buffer type supported)

  ezGALBuffer* pBuffer = CreateBufferPlatform(Description, pInitialData);

  if (pBuffer == nullptr)
  {
    return ezGALBufferHandle();
  }
  else
  {
    return ezGALBufferHandle(m_Buffers.Insert(pBuffer));
  }
}

void ezGALDevice::DestroyBuffer(ezGALBufferHandle hBuffer)
{
  ezGALBuffer* pBuffer = nullptr;

  if (m_Buffers.Remove(hBuffer, &pBuffer))
  {
    DestroyBufferPlatform(pBuffer);
  }
  else
  {
    ezLog::Warning("DestroyBuffer called on invalid handle (double free?)");
  }
}


// Helper functions for buffers (for common, simple use cases)
ezGALBufferHandle ezGALDevice::CreateVertexBuffer(ezUInt32 uiVertexSize, ezUInt32 uiVertexCount, const void* pInitialData /*= nullptr*/)
{
  ezGALBufferCreationDescription Desc;
  Desc.m_uiStructSize = uiVertexSize;
  Desc.m_uiTotalSize = uiVertexSize * uiVertexCount;
  Desc.m_BufferType = ezGALBufferType::VertexBuffer;
  Desc.m_ResourceAccess.m_bImmutable = pInitialData != nullptr;

  return CreateBuffer(Desc, pInitialData);
}

ezGALBufferHandle ezGALDevice::CreateIndexBuffer(ezGALIndexType::Enum IndexType, ezUInt32 uiIndexCount, const void* pInitialData /*= nullptr*/)
{
  ezGALBufferCreationDescription Desc;
  Desc.m_uiStructSize = ezGALIndexType::GetSize(IndexType);
  Desc.m_uiTotalSize = Desc.m_uiStructSize * uiIndexCount;
  Desc.m_BufferType = ezGALBufferType::IndexBuffer;
  Desc.m_ResourceAccess.m_bImmutable = pInitialData != nullptr;

  return CreateBuffer(Desc, pInitialData);
}

ezGALBufferHandle ezGALDevice::CreateConstantBuffer(ezUInt32 uiBufferSize)
{
  ezGALBufferCreationDescription Desc;
  Desc.m_uiStructSize = 0;
  Desc.m_uiTotalSize = uiBufferSize;
  Desc.m_BufferType = ezGALBufferType::ConstantBuffer;
  Desc.m_ResourceAccess.m_bImmutable = false;

  return CreateBuffer(Desc, nullptr);
}



ezGALTextureHandle ezGALDevice::CreateTexture(const ezGALTextureCreationDescription& Description, const ezArrayPtr<ezGALSystemMemoryDescription>* pInitialData)
{
  /// \todo Platform independent validation (desc width & height < platform maximum, format, etc.)

  if (Description.m_ResourceAccess.IsImmutable() && (pInitialData == nullptr || pInitialData->GetCount() < Description.m_uiMipSliceCount) && !Description.m_bCreateRenderTarget)
  {
    ezLog::Error("Trying to create an immutable texture but not supplying initial data (or not enough data pointers) is not possible!");
    return ezGALTextureHandle();
  }

  if (Description.m_uiWidth == 0 || Description.m_uiHeight == 0)
  {
    ezLog::Error("Trying to create a texture with width or height == 0 is not possible!");
    return ezGALTextureHandle();
  }

  ezGALTexture* pTexture = CreateTexturePlatform(Description, pInitialData);

  if (pTexture == nullptr)
  {
    return ezGALTextureHandle();
  }
  else
  {
    return ezGALTextureHandle(m_Textures.Insert(pTexture));
  }
}

void ezGALDevice::DestroyTexture(ezGALTextureHandle hTexture)
{
  ezGALTexture* pTexture = nullptr;

  if (m_Textures.Remove(hTexture, &pTexture))
  {
    DestroyTexturePlatform(pTexture);
  }
  else
  {
    ezLog::Warning("DestroyTexture called on invalid handle (double free?)");
  }
}

ezGALResourceViewHandle ezGALDevice::CreateResourceView(const ezGALResourceViewCreationDescription& Description)
{
  /// \todo Platform independent validation
  /// \todo Hash description and return potential existing one (including inc. refcount)

  ezGALResourceView* pResourceView = CreateResourceViewPlatform(Description);

  if (pResourceView == nullptr)
  {
    return ezGALResourceViewHandle();
  }
  else
  {
    return ezGALResourceViewHandle(m_ResourceViews.Insert(pResourceView));
  }
}

void ezGALDevice::DestroyResourceView(ezGALResourceViewHandle hResourceView)
{
  ezGALResourceView* pResourceView = nullptr;

  /// \todo Only remove if refcount = 0

  if (m_ResourceViews.Remove(hResourceView, &pResourceView))
  {
    DestroyResourceViewPlatform(pResourceView);
  }
  else
  {
    ezLog::Warning("DestroyResourceView called on invalid handle (double free?)");
  }
}


ezGALRenderTargetViewHandle ezGALDevice::CreateRenderTargetView(const ezGALRenderTargetViewCreationDescription& Description)
{
  if (Description.m_hBuffer.IsInvalidated() && Description.m_hTexture.IsInvalidated())
  {
    ezLog::Error("Trying to create a rendertarget view with neither a valid texture or buffer handle!");
    return ezGALRenderTargetViewHandle();
  }

  /// \todo Platform independent validation
  /// \todo Hash description and return potential existing one (including inc. refcount)

  ezGALRenderTargetView* pRenderTargetView = CreateRenderTargetViewPlatform(Description);

  if (pRenderTargetView == nullptr)
  {
    return ezGALRenderTargetViewHandle();
  }
  else
  {
    return ezGALRenderTargetViewHandle(m_RenderTargetViews.Insert(pRenderTargetView));
  }
}

void ezGALDevice::DestroyRenderTargetView(ezGALRenderTargetViewHandle hRenderTargetView)
{
  ezGALRenderTargetView* pRenderTargetView = nullptr;

  /// \todo Only remove if refcount = 0

  if (m_RenderTargetViews.Remove(hRenderTargetView, &pRenderTargetView))
  {
    DestroyRenderTargetViewPlatform(pRenderTargetView);
  }
  else
  {
    ezLog::Warning("DestroyRenderTargetView called on invalid handle (double free?)");
  }
}



ezGALSwapChainHandle ezGALDevice::CreateSwapChain(const ezGALSwapChainCreationDescription& Description)
{
  /// \todo Platform independent validation
  if (Description.m_pWindow == nullptr)
  {
    ezLog::Error("The description for the swap chain creation contained an invalid (nullptr) window handle!");
    return ezGALSwapChainHandle();
  }


  ezGALSwapChain* pSwapChain = CreateSwapChainPlatform(Description);

  if (pSwapChain == nullptr)
  {
    return ezGALSwapChainHandle();
  }
  else
  {
    return ezGALSwapChainHandle(m_SwapChains.Insert(pSwapChain));
  }
}

void ezGALDevice::DestroySwapChain(ezGALSwapChainHandle hSwapChain)
{
  ezGALSwapChain* pSwapChain = nullptr;

  if (m_SwapChains.Remove(hSwapChain, &pSwapChain))
  {
    DestroySwapChainPlatform(pSwapChain);
  }
  else
  {
    ezLog::Warning("DestroySwapChain called on invalid handle (double free?)");
  }
}

ezGALFenceHandle ezGALDevice::CreateFence()
{
  /// \todo Platform independent validation

  ezGALFence* pFence = CreateFencePlatform();

  if (pFence == nullptr)
  {
    return ezGALFenceHandle();
  }
  else
  {
    return ezGALFenceHandle(m_Fences.Insert(pFence));
  }
}

void ezGALDevice::DestroyFence(ezGALFenceHandle& hFence)
{
  ezGALFence* pFence = nullptr;

  if (m_Fences.Remove(hFence, &pFence))
  {
    DestroyFencePlatform(pFence);
  }
  else
  {
    ezLog::Warning("DestroyFence called on invalid handle (double free?)");
  }
}

ezGALQueryHandle ezGALDevice::CreateQuery(const ezGALQueryCreationDescription& Description)
{
  /// \todo Platform independent validation

  ezGALQuery* pQuery = CreateQueryPlatform(Description);

  if (pQuery == nullptr)
  {
    return ezGALQueryHandle();
  }
  else
  {
    return ezGALQueryHandle(m_Queries.Insert(pQuery));
  }
}

void ezGALDevice::DestroyQuery(ezGALQueryHandle hQuery)
{
  ezGALQuery* pQuery = nullptr;

  if (m_Queries.Remove(hQuery, &pQuery))
  {
    DestroyQueryPlatform(pQuery);
  }
  else
  {
    ezLog::Warning("DestroyQuery called on invalid handle (double free?)");
  }
}

ezGALVertexDeclarationHandle ezGALDevice::CreateVertexDeclaration(const ezGALVertexDeclarationCreationDescription& Description)
{
  /// \todo Platform independent validation
  /// \todo Hash description and return potential existing one (including inc. refcount)

  ezGALVertexDeclaration* pVertexDeclaration = CreateVertexDeclarationPlatform(Description);

  if (pVertexDeclaration == nullptr)
  {
    return ezGALVertexDeclarationHandle();
  }
  else
  {
    return ezGALVertexDeclarationHandle(m_VertexDeclarations.Insert(pVertexDeclaration));
  }
}

void ezGALDevice::DestroyVertexDeclaration(ezGALVertexDeclarationHandle hVertexDeclaration)
{
  ezGALVertexDeclaration* pVertexDeclaration = nullptr;

  if (m_VertexDeclarations.Remove(hVertexDeclaration, &pVertexDeclaration))
  {
    DestroyVertexDeclarationPlatform(pVertexDeclaration);
  }
  else
  {
    ezLog::Warning("DestroyVertexDeclaration called on invalid handle (double free?)");
  }
}


// No need to have a context to get query data:

void ezGALDevice::GetQueryData(ezGALQueryHandle hQuery, ezUInt64* puiRendererdPixels)
{
  /// \todo Assert on query support?

  GetQueryDataPlatform(m_Queries[hQuery], puiRendererdPixels);
}


// Swap chain functions

void ezGALDevice::Present(ezGALSwapChainHandle hSwapChain)
{
  EZ_ASSERT_DEV(m_bFrameBeginCalled, "You must have called ezGALDevice::Begin before you can call this function");

  ezGALSwapChain* pSwapChain = nullptr;

  if (m_SwapChains.TryGetValue(hSwapChain, pSwapChain))
  {
    PresentPlatform(pSwapChain);
  }
  else
  {
    EZ_REPORT_FAILURE("Swap chain handle invalid");
  }
}

ezGALTextureHandle ezGALDevice::GetBackBufferTextureFromSwapChain(ezGALSwapChainHandle hSwapChain)
{
  EZ_REPORT_FAILURE("Not implemented!");
  return ezGALTextureHandle();
}




// Misc functions

void ezGALDevice::BeginFrame()
{
  EZ_ASSERT_DEV(!m_bFrameBeginCalled, "You must call ezGALDevice::End before you can call ezGALDevice::BeginFrame again");
  m_bFrameBeginCalled = true;

  BeginFramePlatform();
  m_pPrimaryContext->ClearStatisticsCounters();
}

void ezGALDevice::EndFrame()
{
  EZ_ASSERT_DEV(m_bFrameBeginCalled, "You must have called ezGALDevice::Begin before you can call ezGALDevice::EndFrame");

  EndFramePlatform();

  m_bFrameBeginCalled = false;
}

void ezGALDevice::Flush()
{
  EZ_ASSERT_DEV(m_bFrameBeginCalled, "You must have called ezGALDevice::Begin before you can call this function");

  FlushPlatform();
}

void ezGALDevice::Finish()
{
  EZ_ASSERT_DEV(m_bFrameBeginCalled, "You must have called ezGALDevice::Begin before you can call this function");

  FinishPlatform();
}

void ezGALDevice::SetPrimarySwapChain(ezGALSwapChainHandle hSwapChain)
{
  ezGALSwapChain* pSwapChain = nullptr;

  if (m_SwapChains.TryGetValue(hSwapChain, pSwapChain))
  {
    SetPrimarySwapChainPlatform(pSwapChain); // Needs a return value?
    m_hPrimarySwapChain = hSwapChain;
  }
  else
  {
    ezLog::Error("Invalid swap chain handle given to SetPrimarySwapChain!");
  }
}


const ezGALDeviceCapabilities& ezGALDevice::GetCapabilities() const
{
  return m_Capabilities;
}

ezUInt64 ezGALDevice::GetMemoryConsumptionForTexture(const ezGALTextureCreationDescription& Description) const
{
  // This generic implementation is only an approximation, but it can be overridden by specific devices
  // to give an accurate memory consumption figure.
  ezUInt64 uiMemory = ezUInt64(Description.m_uiWidth) * ezUInt64(Description.m_uiHeight) * ezUInt64(Description.m_uiDepth);
  uiMemory *= Description.m_uiArraySize;
  uiMemory *= ezGALResourceFormat::GetBitsPerElement(Description.m_Format);
  uiMemory /= 8; // Bits per pixel
  uiMemory *= Description.m_SampleCount;
  
  // Also account for mip maps
  if (Description.m_uiMipSliceCount > 1)
  {
    uiMemory = static_cast<ezUInt64>(uiMemory * (1.0 / 3.0) * uiMemory);
  }

  return uiMemory;
}


ezUInt64 ezGALDevice::GetMemoryConsumptionForBuffer(const ezGALBufferCreationDescription& Description) const
{
  return Description.m_uiTotalSize;
}


EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_Device_Implementation_Device);

