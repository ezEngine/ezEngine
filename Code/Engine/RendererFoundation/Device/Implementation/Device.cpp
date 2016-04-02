
#include <RendererFoundation/PCH.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/State/State.h>
#include <RendererFoundation/Shader/Shader.h>
#include <RendererFoundation/Shader/VertexDeclaration.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/Texture.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/ResourceView.h>
#include <RendererFoundation/Context/Context.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Logging/Log.h>

ezGALDevice* ezGALDevice::s_pDefaultDevice = nullptr;

ezGALDevice::ezGALDevice(const ezGALDeviceCreationDescription& desc)
  : m_Allocator("GALDevice", ezFoundation::GetDefaultAllocator())
  , m_AllocatorWrapper(&m_Allocator)
  , m_Description(desc)
  , m_pPrimaryContext(nullptr)
  , m_bFrameBeginCalled(false)
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

ezGALBlendStateHandle ezGALDevice::CreateBlendState(const ezGALBlendStateCreationDescription& desc)
{
  // Hash desc and return potential existing one (including inc. refcount)
  ezUInt32 uiHash = desc.CalculateHash();

  ezGALBlendStateHandle hBlendState;
  if (m_BlendStateTable.TryGetValue(uiHash, hBlendState))
  {
    ezGALBlendState* pBlendState = m_BlendStates[hBlendState];
    pBlendState->AddRef();
    return hBlendState;
  }

  ezGALBlendState* pBlendState = CreateBlendStatePlatform(desc);

  if (pBlendState != nullptr)
  {
    pBlendState->AddRef();

    ezGALBlendStateHandle hBlendState(m_BlendStates.Insert(pBlendState));
    m_BlendStateTable.Insert(uiHash, hBlendState);

    return hBlendState;
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
      m_BlendStateTable.Remove(pBlendState->GetDescription().CalculateHash());

      DestroyBlendStatePlatform(pBlendState);
    }
  }
  else
  {
    ezLog::Warning("DestroyBlendState called on invalid handle (double free?)");
  }
}

ezGALDepthStencilStateHandle ezGALDevice::CreateDepthStencilState(const ezGALDepthStencilStateCreationDescription& desc)
{
  // Hash desc and return potential existing one (including inc. refcount)
  ezUInt32 uiHash = desc.CalculateHash();

  ezGALDepthStencilStateHandle hDepthStencilState;
  if (m_DepthStencilStateTable.TryGetValue(uiHash, hDepthStencilState))
  {
    ezGALDepthStencilState* pDepthStencilState = m_DepthStencilStates[hDepthStencilState];
    pDepthStencilState->AddRef();
    return hDepthStencilState;
  }

  ezGALDepthStencilState* pDepthStencilState = CreateDepthStencilStatePlatform(desc);

  if (pDepthStencilState != nullptr)
  {
    pDepthStencilState->AddRef();

    ezGALDepthStencilStateHandle hDepthStencilState(m_DepthStencilStates.Insert(pDepthStencilState));
    m_DepthStencilStateTable.Insert(uiHash, hDepthStencilState);

    return hDepthStencilState;
  }

  return ezGALDepthStencilStateHandle();
}

void ezGALDevice::DestroyDepthStencilState(ezGALDepthStencilStateHandle hDepthStencilState)
{
  ezGALDepthStencilState* pDepthStencilState = nullptr;

  if (m_DepthStencilStates.TryGetValue(hDepthStencilState, pDepthStencilState))
  {
    pDepthStencilState->ReleaseRef();

    if (pDepthStencilState->GetRefCount() == 0)
    {
      m_DepthStencilStates.Remove(hDepthStencilState);
      m_DepthStencilStateTable.Remove(pDepthStencilState->GetDescription().CalculateHash());

      DestroyDepthStencilStatePlatform(pDepthStencilState);
    }
  }
  else
  {
    ezLog::Warning("DestroyDepthStencilState called on invalid handle (double free?)");
  }
}

ezGALRasterizerStateHandle ezGALDevice::CreateRasterizerState(const ezGALRasterizerStateCreationDescription& desc)
{
  // Hash desc and return potential existing one (including inc. refcount)
  ezUInt32 uiHash = desc.CalculateHash();

  ezGALRasterizerStateHandle hRasterizerState;
  if (m_RasterizerStateTable.TryGetValue(uiHash, hRasterizerState))
  {
    ezGALRasterizerState* pRasterizerState = m_RasterizerStates[hRasterizerState];
    pRasterizerState->AddRef();
    return hRasterizerState;
  }

  ezGALRasterizerState* pRasterizerState = CreateRasterizerStatePlatform(desc);

  if (pRasterizerState != nullptr)
  {
    pRasterizerState->AddRef();

    ezGALRasterizerStateHandle hRasterizerState(m_RasterizerStates.Insert(pRasterizerState));
    m_RasterizerStateTable.Insert(uiHash, hRasterizerState);

    return hRasterizerState;
  }

  return ezGALRasterizerStateHandle();
}

void ezGALDevice::DestroyRasterizerState(ezGALRasterizerStateHandle hRasterizerState)
{
  ezGALRasterizerState* pRasterizerState = nullptr;

  if (m_RasterizerStates.TryGetValue(hRasterizerState, pRasterizerState))
  {
    pRasterizerState->ReleaseRef();

    if (pRasterizerState->GetRefCount() == 0)
    {
      m_RasterizerStates.Remove(hRasterizerState);
      m_RasterizerStateTable.Remove(pRasterizerState->GetDescription().CalculateHash());

      DestroyRasterizerStatePlatform(pRasterizerState);
    }
  }
  else
  {
    ezLog::Warning("DestroyRasterizerState called on invalid handle (double free?)");
  }
}

ezGALSamplerStateHandle ezGALDevice::CreateSamplerState(const ezGALSamplerStateCreationDescription& desc)
{
  /// \todo Platform independent validation

  // Hash desc and return potential existing one (including inc. refcount)
  ezUInt32 uiHash = desc.CalculateHash();

  ezGALSamplerStateHandle hSamplerState;
  if (m_SamplerStateTable.TryGetValue(uiHash, hSamplerState))
  {
    ezGALSamplerState* pSamplerState = m_SamplerStates[hSamplerState];
    pSamplerState->AddRef();
    return hSamplerState;
  }

  ezGALSamplerState* pSamplerState = CreateSamplerStatePlatform(desc);

  if (pSamplerState != nullptr)
  {
    pSamplerState->AddRef();

    ezGALSamplerStateHandle hSamplerState(m_SamplerStates.Insert(pSamplerState));
    m_SamplerStateTable.Insert(uiHash, hSamplerState);

    return hSamplerState;
  }

  return ezGALSamplerStateHandle();
}

void ezGALDevice::DestroySamplerState(ezGALSamplerStateHandle hSamplerState)
{
  ezGALSamplerState* pSamplerState = nullptr;

  if (m_SamplerStates.TryGetValue(hSamplerState, pSamplerState))
  {
    pSamplerState->ReleaseRef();

    if (pSamplerState->GetRefCount() == 0)
    {
      m_SamplerStates.Remove(hSamplerState);
      m_SamplerStateTable.Remove(pSamplerState->GetDescription().CalculateHash());

      DestroySamplerStatePlatform(pSamplerState);
    }
  }
  else
  {
    ezLog::Warning("DestroySamplerState called on invalid handle (double free?)");
  }
}



ezGALShaderHandle ezGALDevice::CreateShader(const ezGALShaderCreationDescription& desc)
{
  bool bHasByteCodes = false;

  for (ezUInt32 uiStage = 0; uiStage < ezGALShaderStage::ENUM_COUNT; uiStage++)
  {
    if (desc.HasByteCodeForStage((ezGALShaderStage::Enum)uiStage))
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

  ezGALShader* pShader = CreateShaderPlatform(desc);

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


ezGALBufferHandle ezGALDevice::CreateBuffer(const ezGALBufferCreationDescription& desc, const void* pInitialData)
{
  if (desc.m_uiTotalSize == 0)
  {
    ezLog::Error("Trying to create a buffer with size of 0 is not possible!");
    return ezGALBufferHandle();
  }

  if (desc.m_ResourceAccess.IsImmutable() && pInitialData == nullptr)
  {
    ezLog::Error("Trying to create an immutable buffer but not supplying initial data is not possible!");
    return ezGALBufferHandle();
  }

  /// \todo Platform independent validation (buffer type supported)

  ezGALBuffer* pBuffer = CreateBufferPlatform(desc, pInitialData);

  if (pBuffer != nullptr)
  {
    ezGALBufferHandle hBuffer(m_Buffers.Insert(pBuffer));

    // Create default resource view
    if (desc.m_BufferType == ezGALBufferType::Generic)
    {
      ezGALResourceViewCreationDescription viewDesc;
      viewDesc.m_hBuffer = hBuffer;
      viewDesc.m_uiFirstElement = 0;
      viewDesc.m_uiNumElements = (desc.m_uiStructSize != 0) ? (desc.m_uiTotalSize / desc.m_uiStructSize) : desc.m_uiTotalSize;

      pBuffer->m_hDefaultResourceView = CreateResourceView(viewDesc);
    }

    return hBuffer;
  }

  return ezGALBufferHandle();
}

void ezGALDevice::DestroyBuffer(ezGALBufferHandle hBuffer)
{
  ezGALBuffer* pBuffer = nullptr;

  if (m_Buffers.Remove(hBuffer, &pBuffer))
  {
    DestroyViews(pBuffer);

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
  ezGALBufferCreationDescription desc;
  desc.m_uiStructSize = uiVertexSize;
  desc.m_uiTotalSize = uiVertexSize * uiVertexCount;
  desc.m_BufferType = ezGALBufferType::VertexBuffer;
  desc.m_ResourceAccess.m_bImmutable = pInitialData != nullptr;

  return CreateBuffer(desc, pInitialData);
}

ezGALBufferHandle ezGALDevice::CreateIndexBuffer(ezGALIndexType::Enum IndexType, ezUInt32 uiIndexCount, const void* pInitialData /*= nullptr*/)
{
  ezGALBufferCreationDescription desc;
  desc.m_uiStructSize = ezGALIndexType::GetSize(IndexType);
  desc.m_uiTotalSize = desc.m_uiStructSize * uiIndexCount;
  desc.m_BufferType = ezGALBufferType::IndexBuffer;
  desc.m_ResourceAccess.m_bImmutable = pInitialData != nullptr;

  return CreateBuffer(desc, pInitialData);
}

ezGALBufferHandle ezGALDevice::CreateConstantBuffer(ezUInt32 uiBufferSize)
{
  ezGALBufferCreationDescription desc;
  desc.m_uiStructSize = 0;
  desc.m_uiTotalSize = uiBufferSize;
  desc.m_BufferType = ezGALBufferType::ConstantBuffer;
  desc.m_ResourceAccess.m_bImmutable = false;

  return CreateBuffer(desc, nullptr);
}



ezGALTextureHandle ezGALDevice::CreateTexture(const ezGALTextureCreationDescription& desc, const ezArrayPtr<ezGALSystemMemoryDescription>* pInitialData)
{
  /// \todo Platform independent validation (desc width & height < platform maximum, format, etc.)

  if (desc.m_ResourceAccess.IsImmutable() && (pInitialData == nullptr || pInitialData->GetCount() < desc.m_uiMipLevelCount) && !desc.m_bCreateRenderTarget)
  {
    ezLog::Error("Trying to create an immutable texture but not supplying initial data (or not enough data pointers) is not possible!");
    return ezGALTextureHandle();
  }

  if (desc.m_uiWidth == 0 || desc.m_uiHeight == 0)
  {
    ezLog::Error("Trying to create a texture with width or height == 0 is not possible!");
    return ezGALTextureHandle();
  }

  ezGALTexture* pTexture = CreateTexturePlatform(desc, pInitialData);

  if (pTexture != nullptr)
  {
    ezGALTextureHandle hTexture(m_Textures.Insert(pTexture));

    // Create default resource view
    if (desc.m_bAllowShaderResourceView)
    {
      ezGALResourceViewCreationDescription viewDesc;
      viewDesc.m_hTexture = hTexture;

      pTexture->m_hDefaultResourceView = CreateResourceView(viewDesc);
    }

    // Create default render target view
    if (desc.m_bCreateRenderTarget)
    {
      ezGALRenderTargetViewCreationDescription rtDesc;
      rtDesc.m_hTexture = hTexture;
      rtDesc.m_uiFirstSlice = 0;
      rtDesc.m_uiSliceCount = desc.m_uiArraySize;

      pTexture->m_hDefaultRenderTargetView = CreateRenderTargetView(rtDesc);
    }

    return hTexture;
  }

  return ezGALTextureHandle();
}

void ezGALDevice::DestroyTexture(ezGALTextureHandle hTexture)
{
  ezGALTexture* pTexture = nullptr;

  if (m_Textures.Remove(hTexture, &pTexture))
  {
    DestroyViews(pTexture);

    DestroyTexturePlatform(pTexture);
  }
  else
  {
    ezLog::Warning("DestroyTexture called on invalid handle (double free?)");
  }
}

ezGALResourceViewHandle ezGALDevice::GetDefaultResourceView(ezGALTextureHandle hTexture)
{
  if (const ezGALTexture* pTexture = GetTexture(hTexture))
  {
    return pTexture->m_hDefaultResourceView;
  }

  return ezGALResourceViewHandle();
}

ezGALResourceViewHandle ezGALDevice::GetDefaultResourceView(ezGALBufferHandle hBuffer)
{
  if (const ezGALBuffer* pBuffer = GetBuffer(hBuffer))
  {
    return pBuffer->m_hDefaultResourceView;
  }

  return ezGALResourceViewHandle();
}

ezGALResourceViewHandle ezGALDevice::CreateResourceView(const ezGALResourceViewCreationDescription& desc)
{
  ezGALResourceBase* pResource = nullptr;

  if (!desc.m_hTexture.IsInvalidated())
    pResource = Get<TextureTable, ezGALTexture>(desc.m_hTexture, m_Textures);

  if (!desc.m_hBuffer.IsInvalidated())
    pResource = Get<BufferTable, ezGALBuffer>(desc.m_hBuffer, m_Buffers);

  if (pResource == nullptr)
  {
    ezLog::Error("No valid texture handle or buffer handle given for resource view creation!");
    return ezGALResourceViewHandle();
  }

  // Hash desc and return potential existing one
  ezUInt32 uiHash = desc.CalculateHash();

  ezGALResourceViewHandle hResourceView;
  if (pResource->m_ResourceViews.TryGetValue(uiHash, hResourceView))
  {
    return hResourceView;
  }

  ezGALResourceView* pResourceView = CreateResourceViewPlatform(pResource, desc);

  if (pResourceView != nullptr)
  {
    ezGALResourceViewHandle hResourceView(m_ResourceViews.Insert(pResourceView));
    pResource->m_ResourceViews.Insert(uiHash, hResourceView);

    return hResourceView;
  }

  return ezGALResourceViewHandle();
}

void ezGALDevice::DestroyResourceView(ezGALResourceViewHandle hResourceView)
{
  ezGALResourceView* pResourceView = nullptr;

  if (m_ResourceViews.TryGetValue(hResourceView, pResourceView))
  {
    m_ResourceViews.Remove(hResourceView);

    ezGALResourceBase* pResource = pResourceView->m_pResource;
    EZ_VERIFY(pResource->m_ResourceViews.Remove(pResourceView->GetDescription().CalculateHash()), "");
    pResourceView->m_pResource = nullptr;

    DestroyResourceViewPlatform(pResourceView);
  }
  else
  {
    ezLog::Warning("DestroyResourceView called on invalid handle (double free?)");
  }
}

ezGALRenderTargetViewHandle ezGALDevice::GetDefaultRenderTargetView(ezGALTextureHandle hTexture)
{
  if (const ezGALTexture* pTexture = GetTexture(hTexture))
  {
    return pTexture->m_hDefaultRenderTargetView;
  }

  return ezGALRenderTargetViewHandle();
}

ezGALRenderTargetViewHandle ezGALDevice::CreateRenderTargetView(const ezGALRenderTargetViewCreationDescription& desc)
{
  ezGALResourceBase* pResource = nullptr;

  if (!desc.m_hTexture.IsInvalidated())
    pResource = Get<TextureTable, ezGALTexture>(desc.m_hTexture, m_Textures);

  if (pResource == nullptr)
  {
    return ezGALRenderTargetViewHandle();
  }

  /// \todo Platform independent validation

  // Hash desc and return potential existing one
  ezUInt32 uiHash = desc.CalculateHash();

  ezGALRenderTargetViewHandle hRenderTargetView;
  if (pResource->m_RenderTargetViews.TryGetValue(uiHash, hRenderTargetView))
  {
    return hRenderTargetView;
  }  

  ezGALRenderTargetView* pRenderTargetView = CreateRenderTargetViewPlatform(pResource, desc);

  if (pRenderTargetView != nullptr)
  {
    ezGALRenderTargetViewHandle hRenderTargetView(m_RenderTargetViews.Insert(pRenderTargetView));
    pResource->m_RenderTargetViews.Insert(uiHash, hRenderTargetView);

    return hRenderTargetView;
  }

  return ezGALRenderTargetViewHandle();
}

void ezGALDevice::DestroyRenderTargetView(ezGALRenderTargetViewHandle hRenderTargetView)
{
  ezGALRenderTargetView* pRenderTargetView = nullptr;

  if (m_RenderTargetViews.TryGetValue(hRenderTargetView, pRenderTargetView))
  {
    m_RenderTargetViews.Remove(hRenderTargetView);

    ezGALResourceBase* pResource = pRenderTargetView->m_pResource;
    EZ_VERIFY(pResource->m_RenderTargetViews.Remove(pRenderTargetView->GetDescription().CalculateHash()), "");
    pRenderTargetView->m_pResource = nullptr;

    DestroyRenderTargetViewPlatform(pRenderTargetView);
  }
  else
  {
    ezLog::Warning("DestroyRenderTargetView called on invalid handle (double free?)");
  }
}

void ezGALDevice::DestroyViews(ezGALResourceBase* pResource)
{
  for (auto it = pResource->m_ResourceViews.GetIterator(); it.IsValid(); ++it)
  {
    ezGALResourceViewHandle hResourceView = it.Value();
    ezGALResourceView* pResourceView = m_ResourceViews[hResourceView];

    m_ResourceViews.Remove(hResourceView);
    
    DestroyResourceViewPlatform(pResourceView);
  }
  pResource->m_ResourceViews.Clear();
  pResource->m_hDefaultResourceView.Invalidate();

  for (auto it = pResource->m_RenderTargetViews.GetIterator(); it.IsValid(); ++it)
  {
    ezGALRenderTargetViewHandle hRenderTargetView = it.Value();
    ezGALRenderTargetView* pRenderTargetView = m_RenderTargetViews[hRenderTargetView];

    m_RenderTargetViews.Remove(hRenderTargetView);

    DestroyRenderTargetViewPlatform(pRenderTargetView);
  }
  pResource->m_RenderTargetViews.Clear();
  pResource->m_hDefaultRenderTargetView.Invalidate();
}



ezGALSwapChainHandle ezGALDevice::CreateSwapChain(const ezGALSwapChainCreationDescription& desc)
{
  /// \todo Platform independent validation
  if (desc.m_pWindow == nullptr)
  {
    ezLog::Error("The desc for the swap chain creation contained an invalid (nullptr) window handle!");
    return ezGALSwapChainHandle();
  }


  ezGALSwapChain* pSwapChain = CreateSwapChainPlatform(desc);

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

ezGALQueryHandle ezGALDevice::CreateQuery(const ezGALQueryCreationDescription& desc)
{
  /// \todo Platform independent validation

  ezGALQuery* pQuery = CreateQueryPlatform(desc);

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

ezGALVertexDeclarationHandle ezGALDevice::CreateVertexDeclaration(const ezGALVertexDeclarationCreationDescription& desc)
{
  /// \todo Platform independent validation
  
  // Hash desc and return potential existing one (including inc. refcount)
  ezUInt32 uiHash = desc.CalculateHash();

  ezGALVertexDeclarationHandle hVertexDeclaration;
  if (m_VertexDeclarationTable.TryGetValue(uiHash, hVertexDeclaration))
  {
    ezGALVertexDeclaration* pVertexDeclaration = m_VertexDeclarations[hVertexDeclaration];
    pVertexDeclaration->AddRef();
    return hVertexDeclaration;
  }

  ezGALVertexDeclaration* pVertexDeclaration = CreateVertexDeclarationPlatform(desc);

  if (pVertexDeclaration != nullptr)
  {
    pVertexDeclaration->AddRef();

    ezGALVertexDeclarationHandle hVertexDeclaration(m_VertexDeclarations.Insert(pVertexDeclaration));
    m_VertexDeclarationTable.Insert(uiHash, hVertexDeclaration);

    return hVertexDeclaration;
  }

  return ezGALVertexDeclarationHandle();
}

void ezGALDevice::DestroyVertexDeclaration(ezGALVertexDeclarationHandle hVertexDeclaration)
{
  ezGALVertexDeclaration* pVertexDeclaration = nullptr;

  if (m_VertexDeclarations.TryGetValue(hVertexDeclaration, pVertexDeclaration))
  {
    pVertexDeclaration->ReleaseRef();

    if (pVertexDeclaration->GetRefCount() == 0)
    {
      m_VertexDeclarations.Remove(hVertexDeclaration);
      m_VertexDeclarationTable.Remove(pVertexDeclaration->GetDescription().CalculateHash());

      DestroyVertexDeclarationPlatform(pVertexDeclaration);
    }
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

ezUInt64 ezGALDevice::GetMemoryConsumptionForTexture(const ezGALTextureCreationDescription& desc) const
{
  // This generic implementation is only an approximation, but it can be overridden by specific devices
  // to give an accurate memory consumption figure.
  ezUInt64 uiMemory = ezUInt64(desc.m_uiWidth) * ezUInt64(desc.m_uiHeight) * ezUInt64(desc.m_uiDepth);
  uiMemory *= desc.m_uiArraySize;
  uiMemory *= ezGALResourceFormat::GetBitsPerElement(desc.m_Format);
  uiMemory /= 8; // Bits per pixel
  uiMemory *= desc.m_SampleCount;
  
  // Also account for mip maps
  if (desc.m_uiMipLevelCount > 1)
  {
    uiMemory = static_cast<ezUInt64>(uiMemory * (1.0 / 3.0) * uiMemory);
  }

  return uiMemory;
}


ezUInt64 ezGALDevice::GetMemoryConsumptionForBuffer(const ezGALBufferCreationDescription& desc) const
{
  return desc.m_uiTotalSize;
}


EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_Device_Implementation_Device);

