#include <RendererFoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Profiling/Profiling.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/ProxyTexture.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/ResourceView.h>
#include <RendererFoundation/Resources/UnorderedAccesView.h>
#include <RendererFoundation/Shader/VertexDeclaration.h>
#include <RendererFoundation/State/State.h>

namespace
{
  struct GALObjectType
  {
    enum Enum
    {
      BlendState,
      DepthStencilState,
      RasterizerState,
      SamplerState,
      Shader,
      Buffer,
      Texture,
      ResourceView,
      RenderTargetView,
      UnorderedAccessView,
      SwapChain,
      Fence,
      Query,
      VertexDeclaration
    };
  };

  EZ_CHECK_AT_COMPILETIME(sizeof(ezGALBlendStateHandle) == sizeof(ezUInt32));
  EZ_CHECK_AT_COMPILETIME(sizeof(ezGALDepthStencilStateHandle) == sizeof(ezUInt32));
  EZ_CHECK_AT_COMPILETIME(sizeof(ezGALRasterizerStateHandle) == sizeof(ezUInt32));
  EZ_CHECK_AT_COMPILETIME(sizeof(ezGALSamplerStateHandle) == sizeof(ezUInt32));
  EZ_CHECK_AT_COMPILETIME(sizeof(ezGALShaderHandle) == sizeof(ezUInt32));
  EZ_CHECK_AT_COMPILETIME(sizeof(ezGALBufferHandle) == sizeof(ezUInt32));
  EZ_CHECK_AT_COMPILETIME(sizeof(ezGALTextureHandle) == sizeof(ezUInt32));
  EZ_CHECK_AT_COMPILETIME(sizeof(ezGALResourceViewHandle) == sizeof(ezUInt32));
  EZ_CHECK_AT_COMPILETIME(sizeof(ezGALRenderTargetViewHandle) == sizeof(ezUInt32));
  EZ_CHECK_AT_COMPILETIME(sizeof(ezGALUnorderedAccessViewHandle) == sizeof(ezUInt32));
  EZ_CHECK_AT_COMPILETIME(sizeof(ezGALSwapChainHandle) == sizeof(ezUInt32));
  EZ_CHECK_AT_COMPILETIME(sizeof(ezGALFenceHandle) == sizeof(ezUInt32));
  EZ_CHECK_AT_COMPILETIME(sizeof(ezGALQueryHandle) == sizeof(ezUInt32));
  EZ_CHECK_AT_COMPILETIME(sizeof(ezGALVertexDeclarationHandle) == sizeof(ezUInt32));
} // namespace

ezGALDevice* ezGALDevice::s_pDefaultDevice = nullptr;


ezGALDevice::ezGALDevice(const ezGALDeviceCreationDescription& desc)
  : m_Allocator("GALDevice", ezFoundation::GetDefaultAllocator())
  , m_AllocatorWrapper(&m_Allocator)
  , m_Description(desc)
{
}

ezGALDevice::~ezGALDevice()
{
  // Check for object leaks
  {
    EZ_LOG_BLOCK("ezGALDevice object leak report");

    if (!m_Shaders.IsEmpty())
      ezLog::Warning("{0} shaders have not been cleaned up", m_Shaders.GetCount());

    if (!m_BlendStates.IsEmpty())
      ezLog::Warning("{0} blend states have not been cleaned up", m_BlendStates.GetCount());

    if (!m_DepthStencilStates.IsEmpty())
      ezLog::Warning("{0} depth stencil states have not been cleaned up", m_DepthStencilStates.GetCount());

    if (!m_RasterizerStates.IsEmpty())
      ezLog::Warning("{0} rasterizer states have not been cleaned up", m_RasterizerStates.GetCount());

    if (!m_Buffers.IsEmpty())
      ezLog::Warning("{0} buffers have not been cleaned up", m_Buffers.GetCount());

    if (!m_Textures.IsEmpty())
      ezLog::Warning("{0} textures have not been cleaned up", m_Textures.GetCount());

    if (!m_ResourceViews.IsEmpty())
      ezLog::Warning("{0} resource views have not been cleaned up", m_ResourceViews.GetCount());

    if (!m_RenderTargetViews.IsEmpty())
      ezLog::Warning("{0} render target views have not been cleaned up", m_RenderTargetViews.GetCount());

    if (!m_UnorderedAccessViews.IsEmpty())
      ezLog::Warning("{0} unordered access views have not been cleaned up", m_UnorderedAccessViews.GetCount());

    if (!m_SwapChains.IsEmpty())
      ezLog::Warning("{0} swap chains have not been cleaned up", m_SwapChains.GetCount());

    if (!m_Fences.IsEmpty())
      ezLog::Warning("{0} fences have not been cleaned up", m_Fences.GetCount());

    if (!m_Queries.IsEmpty())
      ezLog::Warning("{0} queries have not been cleaned up", m_Queries.GetCount());

    if (!m_VertexDeclarations.IsEmpty())
      ezLog::Warning("{0} vertex declarations have not been cleaned up", m_VertexDeclarations.GetCount());
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

  ezLog::Info("Adapter: '{}' - {} VRAM, {} Sys RAM, {} Shared RAM", m_Capabilities.m_sAdapterName, ezArgFileSize(m_Capabilities.m_uiDedicatedVRAM),
    ezArgFileSize(m_Capabilities.m_uiDedicatedSystemRAM), ezArgFileSize(m_Capabilities.m_uiSharedSystemRAM));

  if (!m_Capabilities.m_bHardwareAccelerated)
  {
    ezLog::Warning("Selected graphics adapter has no hardware acceleration.");
  }

  EZ_GALDEVICE_LOCK_AND_CHECK();

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

  ezProfilingSystem::InitializeGPUData();

  {
    ezGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type = ezGALDeviceEvent::AfterInit;
    m_Events.Broadcast(e);
  }

  return EZ_SUCCESS;
}

ezResult ezGALDevice::Shutdown()
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  EZ_LOG_BLOCK("ezGALDevice::Shutdown");

  {
    ezGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type = ezGALDeviceEvent::BeforeShutdown;
    m_Events.Broadcast(e);
  }

  // If we created a primary swap chain, release it
  if (!m_hPrimarySwapChain.IsInvalidated())
  {
    // DestroySwapChain usually warns for destroying the primary swap chain.
    auto handle = m_hPrimarySwapChain;
    m_hPrimarySwapChain.Invalidate();
    DestroySwapChain(handle);
  }

  DestroyDeadObjects();

  // make sure we are not listed as the default device anymore
  if (ezGALDevice::HasDefaultDevice() && ezGALDevice::GetDefaultDevice() == this)
  {
    ezGALDevice::SetDefaultDevice(nullptr);
  }

  return ShutdownPlatform();
}

ezGALPass* ezGALDevice::BeginPass(const char* szName)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  return BeginPassPlatform(szName);
}

void ezGALDevice::EndPass(ezGALPass* pPass)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  EndPassPlatform(pPass);
}

ezGALBlendStateHandle ezGALDevice::CreateBlendState(const ezGALBlendStateCreationDescription& desc)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  // Hash desc and return potential existing one (including inc. refcount)
  ezUInt32 uiHash = desc.CalculateHash();

  {
    ezGALBlendStateHandle hBlendState;
    if (m_BlendStateTable.TryGetValue(uiHash, hBlendState))
    {
      ezGALBlendState* pBlendState = m_BlendStates[hBlendState];
      if (pBlendState->GetRefCount() == 0)
      {
        ReviveDeadObject(GALObjectType::BlendState, hBlendState);
      }

      pBlendState->AddRef();
      return hBlendState;
    }
  }

  ezGALBlendState* pBlendState = CreateBlendStatePlatform(desc);

  if (pBlendState != nullptr)
  {
    EZ_ASSERT_DEBUG(pBlendState->GetDescription().CalculateHash() == uiHash, "BlendState hash doesn't match");

    pBlendState->AddRef();

    ezGALBlendStateHandle hBlendState(m_BlendStates.Insert(pBlendState));
    m_BlendStateTable.Insert(uiHash, hBlendState);

    return hBlendState;
  }

  return ezGALBlendStateHandle();
}

void ezGALDevice::DestroyBlendState(ezGALBlendStateHandle hBlendState)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezGALBlendState* pBlendState = nullptr;

  if (m_BlendStates.TryGetValue(hBlendState, pBlendState))
  {
    pBlendState->ReleaseRef();

    if (pBlendState->GetRefCount() == 0)
    {
      AddDeadObject(GALObjectType::BlendState, hBlendState);
    }
  }
  else
  {
    ezLog::Warning("DestroyBlendState called on invalid handle (double free?)");
  }
}

ezGALDepthStencilStateHandle ezGALDevice::CreateDepthStencilState(const ezGALDepthStencilStateCreationDescription& desc)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  // Hash desc and return potential existing one (including inc. refcount)
  ezUInt32 uiHash = desc.CalculateHash();

  {
    ezGALDepthStencilStateHandle hDepthStencilState;
    if (m_DepthStencilStateTable.TryGetValue(uiHash, hDepthStencilState))
    {
      ezGALDepthStencilState* pDepthStencilState = m_DepthStencilStates[hDepthStencilState];
      if (pDepthStencilState->GetRefCount() == 0)
      {
        ReviveDeadObject(GALObjectType::DepthStencilState, hDepthStencilState);
      }

      pDepthStencilState->AddRef();
      return hDepthStencilState;
    }
  }

  ezGALDepthStencilState* pDepthStencilState = CreateDepthStencilStatePlatform(desc);

  if (pDepthStencilState != nullptr)
  {
    EZ_ASSERT_DEBUG(pDepthStencilState->GetDescription().CalculateHash() == uiHash, "DepthStencilState hash doesn't match");

    pDepthStencilState->AddRef();

    ezGALDepthStencilStateHandle hDepthStencilState(m_DepthStencilStates.Insert(pDepthStencilState));
    m_DepthStencilStateTable.Insert(uiHash, hDepthStencilState);

    return hDepthStencilState;
  }

  return ezGALDepthStencilStateHandle();
}

void ezGALDevice::DestroyDepthStencilState(ezGALDepthStencilStateHandle hDepthStencilState)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezGALDepthStencilState* pDepthStencilState = nullptr;

  if (m_DepthStencilStates.TryGetValue(hDepthStencilState, pDepthStencilState))
  {
    pDepthStencilState->ReleaseRef();

    if (pDepthStencilState->GetRefCount() == 0)
    {
      AddDeadObject(GALObjectType::DepthStencilState, hDepthStencilState);
    }
  }
  else
  {
    ezLog::Warning("DestroyDepthStencilState called on invalid handle (double free?)");
  }
}

ezGALRasterizerStateHandle ezGALDevice::CreateRasterizerState(const ezGALRasterizerStateCreationDescription& desc)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  // Hash desc and return potential existing one (including inc. refcount)
  ezUInt32 uiHash = desc.CalculateHash();

  {
    ezGALRasterizerStateHandle hRasterizerState;
    if (m_RasterizerStateTable.TryGetValue(uiHash, hRasterizerState))
    {
      ezGALRasterizerState* pRasterizerState = m_RasterizerStates[hRasterizerState];
      if (pRasterizerState->GetRefCount() == 0)
      {
        ReviveDeadObject(GALObjectType::RasterizerState, hRasterizerState);
      }

      pRasterizerState->AddRef();
      return hRasterizerState;
    }
  }

  ezGALRasterizerState* pRasterizerState = CreateRasterizerStatePlatform(desc);

  if (pRasterizerState != nullptr)
  {
    EZ_ASSERT_DEBUG(pRasterizerState->GetDescription().CalculateHash() == uiHash, "RasterizerState hash doesn't match");

    pRasterizerState->AddRef();

    ezGALRasterizerStateHandle hRasterizerState(m_RasterizerStates.Insert(pRasterizerState));
    m_RasterizerStateTable.Insert(uiHash, hRasterizerState);

    return hRasterizerState;
  }

  return ezGALRasterizerStateHandle();
}

void ezGALDevice::DestroyRasterizerState(ezGALRasterizerStateHandle hRasterizerState)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezGALRasterizerState* pRasterizerState = nullptr;

  if (m_RasterizerStates.TryGetValue(hRasterizerState, pRasterizerState))
  {
    pRasterizerState->ReleaseRef();

    if (pRasterizerState->GetRefCount() == 0)
    {
      AddDeadObject(GALObjectType::RasterizerState, hRasterizerState);
    }
  }
  else
  {
    ezLog::Warning("DestroyRasterizerState called on invalid handle (double free?)");
  }
}

ezGALSamplerStateHandle ezGALDevice::CreateSamplerState(const ezGALSamplerStateCreationDescription& desc)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  /// \todo Platform independent validation

  // Hash desc and return potential existing one (including inc. refcount)
  ezUInt32 uiHash = desc.CalculateHash();

  {
    ezGALSamplerStateHandle hSamplerState;
    if (m_SamplerStateTable.TryGetValue(uiHash, hSamplerState))
    {
      ezGALSamplerState* pSamplerState = m_SamplerStates[hSamplerState];
      if (pSamplerState->GetRefCount() == 0)
      {
        ReviveDeadObject(GALObjectType::SamplerState, hSamplerState);
      }

      pSamplerState->AddRef();
      return hSamplerState;
    }
  }

  ezGALSamplerState* pSamplerState = CreateSamplerStatePlatform(desc);

  if (pSamplerState != nullptr)
  {
    EZ_ASSERT_DEBUG(pSamplerState->GetDescription().CalculateHash() == uiHash, "SamplerState hash doesn't match");

    pSamplerState->AddRef();

    ezGALSamplerStateHandle hSamplerState(m_SamplerStates.Insert(pSamplerState));
    m_SamplerStateTable.Insert(uiHash, hSamplerState);

    return hSamplerState;
  }

  return ezGALSamplerStateHandle();
}

void ezGALDevice::DestroySamplerState(ezGALSamplerStateHandle hSamplerState)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezGALSamplerState* pSamplerState = nullptr;

  if (m_SamplerStates.TryGetValue(hSamplerState, pSamplerState))
  {
    pSamplerState->ReleaseRef();

    if (pSamplerState->GetRefCount() == 0)
    {
      AddDeadObject(GALObjectType::SamplerState, hSamplerState);
    }
  }
  else
  {
    ezLog::Warning("DestroySamplerState called on invalid handle (double free?)");
  }
}



ezGALShaderHandle ezGALDevice::CreateShader(const ezGALShaderCreationDescription& desc)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

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
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezGALShader* pShader = nullptr;

  if (m_Shaders.TryGetValue(hShader, pShader))
  {
    AddDeadObject(GALObjectType::Shader, hShader);
  }
  else
  {
    ezLog::Warning("DestroyShader called on invalid handle (double free?)");
  }
}


ezGALBufferHandle ezGALDevice::CreateBuffer(const ezGALBufferCreationDescription& desc, ezArrayPtr<const ezUInt8> pInitialData)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  if (desc.m_uiTotalSize == 0)
  {
    ezLog::Error("Trying to create a buffer with size of 0 is not possible!");
    return ezGALBufferHandle();
  }

  if (desc.m_ResourceAccess.IsImmutable())
  {
    if (pInitialData.IsEmpty())
    {
      ezLog::Error("Trying to create an immutable buffer but not supplying initial data is not possible!");
      return ezGALBufferHandle();
    }

    ezUInt32 uiBufferSize = desc.m_uiTotalSize;
    if (uiBufferSize != pInitialData.GetCount())
    {
      ezLog::Error("Trying to create a buffer with invalid initial data!");
      return ezGALBufferHandle();
    }
  }

  /// \todo Platform independent validation (buffer type supported)

  ezGALBuffer* pBuffer = CreateBufferPlatform(desc, pInitialData);

  if (pBuffer != nullptr)
  {
    ezGALBufferHandle hBuffer(m_Buffers.Insert(pBuffer));

    // Create default resource view
    if (desc.m_bAllowShaderResourceView && desc.m_BufferType == ezGALBufferType::Generic)
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
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezGALBuffer* pBuffer = nullptr;

  if (m_Buffers.TryGetValue(hBuffer, pBuffer))
  {
    AddDeadObject(GALObjectType::Buffer, hBuffer);
  }
  else
  {
    ezLog::Warning("DestroyBuffer called on invalid handle (double free?)");
  }
}


// Helper functions for buffers (for common, simple use cases)
ezGALBufferHandle ezGALDevice::CreateVertexBuffer(ezUInt32 uiVertexSize, ezUInt32 uiVertexCount, ezArrayPtr<const ezUInt8> pInitialData)
{
  ezGALBufferCreationDescription desc;
  desc.m_uiStructSize = uiVertexSize;
  desc.m_uiTotalSize = uiVertexSize * uiVertexCount;
  desc.m_BufferType = ezGALBufferType::VertexBuffer;
  desc.m_ResourceAccess.m_bImmutable = !pInitialData.IsEmpty();

  return CreateBuffer(desc, pInitialData);
}

ezGALBufferHandle ezGALDevice::CreateIndexBuffer(ezGALIndexType::Enum IndexType, ezUInt32 uiIndexCount, ezArrayPtr<const ezUInt8> pInitialData)
{
  ezGALBufferCreationDescription desc;
  desc.m_uiStructSize = ezGALIndexType::GetSize(IndexType);
  desc.m_uiTotalSize = desc.m_uiStructSize * uiIndexCount;
  desc.m_BufferType = ezGALBufferType::IndexBuffer;
  desc.m_ResourceAccess.m_bImmutable = !pInitialData.IsEmpty();

  return CreateBuffer(desc, pInitialData);
}

ezGALBufferHandle ezGALDevice::CreateConstantBuffer(ezUInt32 uiBufferSize)
{
  ezGALBufferCreationDescription desc;
  desc.m_uiStructSize = 0;
  desc.m_uiTotalSize = uiBufferSize;
  desc.m_BufferType = ezGALBufferType::ConstantBuffer;
  desc.m_ResourceAccess.m_bImmutable = false;

  return CreateBuffer(desc);
}


ezGALTextureHandle ezGALDevice::CreateTexture(const ezGALTextureCreationDescription& desc, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  /// \todo Platform independent validation (desc width & height < platform maximum, format, etc.)

  if (desc.m_ResourceAccess.IsImmutable() && (pInitialData.IsEmpty() || pInitialData.GetCount() < desc.m_uiMipLevelCount) &&
      !desc.m_bCreateRenderTarget)
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
      viewDesc.m_uiArraySize = desc.m_uiArraySize;
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

ezResult ezGALDevice::ReplaceExisitingNativeObject(ezGALTextureHandle hTexture, void* pExisitingNativeObject)
{
  ezGALTexture* pTexture = nullptr;
  if (m_Textures.TryGetValue(hTexture, pTexture))
  {
    for (auto it = pTexture->m_ResourceViews.GetIterator(); it.IsValid(); ++it)
    {
      ezGALResourceView* pResourceView = nullptr;

      if (m_ResourceViews.TryGetValue(it.Value(), pResourceView))
      {
        EZ_VERIFY(pResourceView->DeInitPlatform(this).Succeeded(), "DeInitPlatform should never fail.");
      }
    }
    for (auto it = pTexture->m_RenderTargetViews.GetIterator(); it.IsValid(); ++it)
    {
      ezGALRenderTargetView* pRenderTargetView = nullptr;

      if (m_RenderTargetViews.TryGetValue(it.Value(), pRenderTargetView))
      {
        EZ_VERIFY(pRenderTargetView->DeInitPlatform(this).Succeeded(), "DeInitPlatform should never fail.");
      }
    }
    for (auto it = pTexture->m_UnorderedAccessViews.GetIterator(); it.IsValid(); ++it)
    {
      ezGALUnorderedAccessView* pUnorderedAccessView = nullptr;

      if (m_UnorderedAccessViews.TryGetValue(it.Value(), pUnorderedAccessView))
      {
        EZ_VERIFY(pUnorderedAccessView->DeInitPlatform(this).Succeeded(), "DeInitPlatform should never fail.");
      }
    }

    EZ_VERIFY(pTexture->DeInitPlatform(this).Succeeded(), "DeInitPlatform should never fail.");
    EZ_VERIFY(
      pTexture->ReplaceExisitingNativeObject(pExisitingNativeObject).Succeeded(), "Failed to replace native texture, make sure the input is valid.");
    EZ_VERIFY(pTexture->InitPlatform(this, {}).Succeeded(),
      "InitPlatform failed on a texture the previously succeded in the same call, is the new native object valid?");

    for (auto it = pTexture->m_ResourceViews.GetIterator(); it.IsValid(); ++it)
    {
      ezGALResourceView* pResourceView = nullptr;

      if (m_ResourceViews.TryGetValue(it.Value(), pResourceView))
      {
        EZ_VERIFY(pResourceView->InitPlatform(this).Succeeded(),
          "InitPlatform failed on a resource view that previously succeded in the same call, is the new native object valid?");
      }
    }
    for (auto it = pTexture->m_RenderTargetViews.GetIterator(); it.IsValid(); ++it)
    {
      ezGALRenderTargetView* pRenderTargetView = nullptr;

      if (m_RenderTargetViews.TryGetValue(it.Value(), pRenderTargetView))
      {
        EZ_VERIFY(pRenderTargetView->InitPlatform(this).Succeeded(),
          "InitPlatform failed on a render target view that previously succeded in the same call, is the new native object valid?");
      }
    }
    for (auto it = pTexture->m_UnorderedAccessViews.GetIterator(); it.IsValid(); ++it)
    {
      ezGALUnorderedAccessView* pUnorderedAccessView = nullptr;

      if (m_UnorderedAccessViews.TryGetValue(it.Value(), pUnorderedAccessView))
      {
        EZ_VERIFY(pUnorderedAccessView->InitPlatform(this).Succeeded(),
          "InitPlatform failed on a unordered access view that previously succeded in the same call, is the new native object valid?");
      }
    }
    return EZ_SUCCESS;
  }
  else
  {
    ezLog::Warning("ReplaceExisitingNativeObject called on invalid handle");
    return EZ_FAILURE;
  }
}

void ezGALDevice::DestroyTexture(ezGALTextureHandle hTexture)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezGALTexture* pTexture = nullptr;
  if (m_Textures.TryGetValue(hTexture, pTexture))
  {
    AddDeadObject(GALObjectType::Texture, hTexture);
  }
  else
  {
    ezLog::Warning("DestroyTexture called on invalid handle (double free?)");
  }
}

ezGALTextureHandle ezGALDevice::CreateProxyTexture(ezGALTextureHandle hParentTexture, ezUInt32 uiSlice)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezGALTexture* pParentTexture = nullptr;

  if (!hParentTexture.IsInvalidated())
  {
    pParentTexture = Get<TextureTable, ezGALTexture>(hParentTexture, m_Textures);
  }

  if (pParentTexture == nullptr)
  {
    ezLog::Error("No valid texture handle given for proxy texture creation!");
    return ezGALTextureHandle();
  }

  const auto& parentDesc = pParentTexture->GetDescription();
  EZ_ASSERT_DEV(parentDesc.m_Type != ezGALTextureType::Texture2DProxy, "Can't create a proxy texture of a proxy texture.");
  EZ_ASSERT_DEV(parentDesc.m_Type == ezGALTextureType::TextureCube || parentDesc.m_uiArraySize > 1,
    "Proxy textures can only be created for cubemaps or array textures.");

  ezGALProxyTexture* pProxyTexture = EZ_NEW(&m_Allocator, ezGALProxyTexture, *pParentTexture);
  ezGALTextureHandle hProxyTexture(m_Textures.Insert(pProxyTexture));

  const auto& desc = pProxyTexture->GetDescription();

  // Create default resource view
  if (desc.m_bAllowShaderResourceView)
  {
    ezGALResourceViewCreationDescription viewDesc;
    viewDesc.m_hTexture = hProxyTexture;
    viewDesc.m_uiFirstArraySlice = uiSlice;
    viewDesc.m_uiArraySize = 1;

    pProxyTexture->m_hDefaultResourceView = CreateResourceView(viewDesc);
  }

  // Create default render target view
  if (desc.m_bCreateRenderTarget)
  {
    ezGALRenderTargetViewCreationDescription rtDesc;
    rtDesc.m_hTexture = hProxyTexture;
    rtDesc.m_uiFirstSlice = uiSlice;
    rtDesc.m_uiSliceCount = 1;

    pProxyTexture->m_hDefaultRenderTargetView = CreateRenderTargetView(rtDesc);
  }

  return hProxyTexture;
}

void ezGALDevice::DestroyProxyTexture(ezGALTextureHandle hProxyTexture)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezGALTexture* pTexture = nullptr;
  if (m_Textures.TryGetValue(hProxyTexture, pTexture))
  {
    EZ_ASSERT_DEV(pTexture->GetDescription().m_Type == ezGALTextureType::Texture2DProxy, "Given texture is not a proxy texture");

    AddDeadObject(GALObjectType::Texture, hProxyTexture);
  }
  else
  {
    ezLog::Warning("DestroyProxyTexture called on invalid handle (double free?)");
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
  EZ_GALDEVICE_LOCK_AND_CHECK();

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

  {
    ezGALResourceViewHandle hResourceView;
    if (pResource->m_ResourceViews.TryGetValue(uiHash, hResourceView))
    {
      return hResourceView;
    }
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
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezGALResourceView* pResourceView = nullptr;

  if (m_ResourceViews.TryGetValue(hResourceView, pResourceView))
  {
    AddDeadObject(GALObjectType::ResourceView, hResourceView);
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
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezGALTexture* pTexture = nullptr;

  if (!desc.m_hTexture.IsInvalidated())
    pTexture = Get<TextureTable, ezGALTexture>(desc.m_hTexture, m_Textures);

  if (pTexture == nullptr)
  {
    ezLog::Error("No valid texture handle given for render target view creation!");
    return ezGALRenderTargetViewHandle();
  }

  /// \todo Platform independent validation

  // Hash desc and return potential existing one
  ezUInt32 uiHash = desc.CalculateHash();

  {
    ezGALRenderTargetViewHandle hRenderTargetView;
    if (pTexture->m_RenderTargetViews.TryGetValue(uiHash, hRenderTargetView))
    {
      return hRenderTargetView;
    }
  }

  ezGALRenderTargetView* pRenderTargetView = CreateRenderTargetViewPlatform(pTexture, desc);

  if (pRenderTargetView != nullptr)
  {
    ezGALRenderTargetViewHandle hRenderTargetView(m_RenderTargetViews.Insert(pRenderTargetView));
    pTexture->m_RenderTargetViews.Insert(uiHash, hRenderTargetView);

    return hRenderTargetView;
  }

  return ezGALRenderTargetViewHandle();
}

void ezGALDevice::DestroyRenderTargetView(ezGALRenderTargetViewHandle hRenderTargetView)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezGALRenderTargetView* pRenderTargetView = nullptr;

  if (m_RenderTargetViews.TryGetValue(hRenderTargetView, pRenderTargetView))
  {
    AddDeadObject(GALObjectType::RenderTargetView, hRenderTargetView);
  }
  else
  {
    ezLog::Warning("DestroyRenderTargetView called on invalid handle (double free?)");
  }
}

ezGALUnorderedAccessViewHandle ezGALDevice::CreateUnorderedAccessView(const ezGALUnorderedAccessViewCreationDescription& desc)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  if (!desc.m_hTexture.IsInvalidated() && !desc.m_hBuffer.IsInvalidated())
  {
    ezLog::Error("Can't pass both a texture and buffer to a ezGALUnorderedAccessViewCreationDescription.");
    return ezGALUnorderedAccessViewHandle();
  }

  ezGALResourceBase* pResource = nullptr;
  ezGALTexture* pTexture = nullptr;
  ezGALBuffer* pBuffer = nullptr;

  if (!desc.m_hTexture.IsInvalidated())
  {
    pResource = pTexture = Get<TextureTable, ezGALTexture>(desc.m_hTexture, m_Textures);
  }
  else if (!desc.m_hBuffer.IsInvalidated())
  {
    pResource = pBuffer = Get<BufferTable, ezGALBuffer>(desc.m_hBuffer, m_Buffers);
  }

  if (pResource == nullptr)
  {
    ezLog::Error("No valid texture handle or buffer handle given for unordered access view creation!");
    return ezGALUnorderedAccessViewHandle();
  }

  // Some platform independent validation.
  {
    if (pTexture)
    {
      // Is this really platform independent?
      if (pTexture->GetDescription().m_Type == ezGALTextureType::TextureCube)
      {
        ezLog::Error("Can't create unordered access view from cube textures.");
        return ezGALUnorderedAccessViewHandle();
      }

      // Is this really platform independent?
      if (pTexture->GetDescription().m_SampleCount != ezGALMSAASampleCount::None)
      {
        ezLog::Error("Can't create unordered access view on textures with multisampling.");
        return ezGALUnorderedAccessViewHandle();
      }
    }
    else
    {
      if (desc.m_OverrideViewFormat == ezGALResourceFormat::Invalid)
      {
        ezLog::Error("Invalid resource format is not allowed for buffer unordered access views!");
        return ezGALUnorderedAccessViewHandle();
      }

      if (!pBuffer->GetDescription().m_bAllowRawViews && desc.m_bRawView)
      {
        ezLog::Error("Trying to create a raw view for a buffer with no raw view flag is invalid!");
        return ezGALUnorderedAccessViewHandle();
      }
    }
  }

  // Hash desc and return potential existing one
  ezUInt32 uiHash = desc.CalculateHash();

  {
    ezGALUnorderedAccessViewHandle hUnorderedAccessView;
    if (pResource->m_UnorderedAccessViews.TryGetValue(uiHash, hUnorderedAccessView))
    {
      return hUnorderedAccessView;
    }
  }

  ezGALUnorderedAccessView* pUnorderedAccessViewView = CreateUnorderedAccessViewPlatform(pResource, desc);

  if (pUnorderedAccessViewView != nullptr)
  {
    ezGALUnorderedAccessViewHandle hUnorderedAccessView(m_UnorderedAccessViews.Insert(pUnorderedAccessViewView));
    pResource->m_UnorderedAccessViews.Insert(uiHash, hUnorderedAccessView);

    return hUnorderedAccessView;
  }

  return ezGALUnorderedAccessViewHandle();
}

void ezGALDevice::DestroyUnorderedAccessView(ezGALUnorderedAccessViewHandle hUnorderedAccessViewHandle)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezGALUnorderedAccessView* pUnorderedAccesssView = nullptr;

  if (m_UnorderedAccessViews.TryGetValue(hUnorderedAccessViewHandle, pUnorderedAccesssView))
  {
    AddDeadObject(GALObjectType::UnorderedAccessView, hUnorderedAccessViewHandle);
  }
  else
  {
    ezLog::Warning("DestroyUnorderedAccessView called on invalid handle (double free?)");
  }
}

ezGALSwapChainHandle ezGALDevice::CreateSwapChain(const ezGALSwapChainCreationDescription& desc)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

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
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezGALSwapChain* pSwapChain = nullptr;

  if (hSwapChain == m_hPrimarySwapChain)
  {
    ezLog::Warning("DestroySwapChain called on primary swap chain!");
    m_hPrimarySwapChain.Invalidate();
  }

  if (m_SwapChains.TryGetValue(hSwapChain, pSwapChain))
  {
    AddDeadObject(GALObjectType::SwapChain, hSwapChain);
  }
  else
  {
    ezLog::Warning("DestroySwapChain called on invalid handle (double free?)");
  }
}

ezGALFenceHandle ezGALDevice::CreateFence()
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

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
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezGALFence* pFence = nullptr;

  if (m_Fences.TryGetValue(hFence, pFence))
  {
    AddDeadObject(GALObjectType::Fence, hFence);
  }
  else
  {
    ezLog::Warning("DestroyFence called on invalid handle (double free?)");
  }
}

ezGALQueryHandle ezGALDevice::CreateQuery(const ezGALQueryCreationDescription& desc)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

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
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezGALQuery* pQuery = nullptr;

  if (m_Queries.TryGetValue(hQuery, pQuery))
  {
    AddDeadObject(GALObjectType::Query, hQuery);
  }
  else
  {
    ezLog::Warning("DestroyQuery called on invalid handle (double free?)");
  }
}

ezGALVertexDeclarationHandle ezGALDevice::CreateVertexDeclaration(const ezGALVertexDeclarationCreationDescription& desc)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  /// \todo Platform independent validation

  // Hash desc and return potential existing one (including inc. refcount)
  ezUInt32 uiHash = desc.CalculateHash();

  {
    ezGALVertexDeclarationHandle hVertexDeclaration;
    if (m_VertexDeclarationTable.TryGetValue(uiHash, hVertexDeclaration))
    {
      ezGALVertexDeclaration* pVertexDeclaration = m_VertexDeclarations[hVertexDeclaration];
      if (pVertexDeclaration->GetRefCount() == 0)
      {
        ReviveDeadObject(GALObjectType::VertexDeclaration, hVertexDeclaration);
      }

      pVertexDeclaration->AddRef();
      return hVertexDeclaration;
    }
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
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezGALVertexDeclaration* pVertexDeclaration = nullptr;

  if (m_VertexDeclarations.TryGetValue(hVertexDeclaration, pVertexDeclaration))
  {
    pVertexDeclaration->ReleaseRef();

    if (pVertexDeclaration->GetRefCount() == 0)
    {
      AddDeadObject(GALObjectType::VertexDeclaration, hVertexDeclaration);
    }
  }
  else
  {
    ezLog::Warning("DestroyVertexDeclaration called on invalid handle (double free?)");
  }
}

// Swap chain functions

void ezGALDevice::Present(ezGALSwapChainHandle hSwapChain, bool bVSync)
{
  EZ_ASSERT_DEV(m_bFrameBeginCalled, "You must have called ezGALDevice::Begin before you can call this function");

  ezGALSwapChain* pSwapChain = nullptr;

  if (m_SwapChains.TryGetValue(hSwapChain, pSwapChain))
  {
    PresentPlatform(pSwapChain, bVSync);
  }
  else
  {
    EZ_REPORT_FAILURE("Swap chain handle invalid");
  }
}

ezGALTextureHandle ezGALDevice::GetBackBufferTextureFromSwapChain(ezGALSwapChainHandle hSwapChain)
{
  ezGALSwapChain* pSwapChain = nullptr;

  if (m_SwapChains.TryGetValue(hSwapChain, pSwapChain))
  {
    return pSwapChain->GetBackBufferTexture();
  }
  else
  {
    EZ_REPORT_FAILURE("Swap chain handle invalid");
    return ezGALTextureHandle();
  }
}



// Misc functions

void ezGALDevice::BeginFrame()
{
  {
    ezGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type = ezGALDeviceEvent::BeforeBeginFrame;
    m_Events.Broadcast(e);
  }

  {
    EZ_GALDEVICE_LOCK_AND_CHECK();
    EZ_ASSERT_DEV(!m_bFrameBeginCalled, "You must call ezGALDevice::End before you can call ezGALDevice::BeginFrame again");
    m_bFrameBeginCalled = true;

    BeginFramePlatform();
  }

  // TODO: move to beginrendering/compute calls
  //m_pPrimaryContext->ClearStatisticsCounters();

  {
    ezGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type = ezGALDeviceEvent::AfterBeginFrame;
    m_Events.Broadcast(e);
  }
}

void ezGALDevice::EndFrame()
{
  {
    ezGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type = ezGALDeviceEvent::BeforeEndFrame;
    m_Events.Broadcast(e);
  }

  {
    EZ_GALDEVICE_LOCK_AND_CHECK();
    EZ_ASSERT_DEV(m_bFrameBeginCalled, "You must have called ezGALDevice::Begin before you can call ezGALDevice::EndFrame");

    DestroyDeadObjects();

    EndFramePlatform();

    m_bFrameBeginCalled = false;
  }

  {
    ezGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type = ezGALDeviceEvent::AfterEndFrame;
    m_Events.Broadcast(e);
  }
}

void ezGALDevice::SetPrimarySwapChain(ezGALSwapChainHandle hSwapChain)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

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
    uiMemory += static_cast<ezUInt64>((1.0 / 3.0) * uiMemory);
  }

  return uiMemory;
}


ezUInt64 ezGALDevice::GetMemoryConsumptionForBuffer(const ezGALBufferCreationDescription& desc) const
{
  return desc.m_uiTotalSize;
}


void ezGALDevice::DestroyViews(ezGALResourceBase* pResource)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

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

  for (auto it = pResource->m_UnorderedAccessViews.GetIterator(); it.IsValid(); ++it)
  {
    ezGALUnorderedAccessViewHandle hUnorderedAccessView = it.Value();
    ezGALUnorderedAccessView* pUnorderedAccessView = m_UnorderedAccessViews[hUnorderedAccessView];

    m_UnorderedAccessViews.Remove(hUnorderedAccessView);

    DestroyUnorderedAccessViewPlatform(pUnorderedAccessView);
  }
  pResource->m_UnorderedAccessViews.Clear();
}

void ezGALDevice::DestroyDeadObjects()
{
  // Can't use range based for here since new objects might be added during iteration
  for (ezUInt32 i = 0; i < m_DeadObjects.GetCount(); ++i)
  {
    const auto& deadObject = m_DeadObjects[i];

    switch (deadObject.m_uiType)
    {
      case GALObjectType::BlendState:
      {
        ezGALBlendStateHandle hBlendState(ezGAL::ez16_16Id(deadObject.m_uiHandle));
        ezGALBlendState* pBlendState = nullptr;

        EZ_VERIFY(m_BlendStates.Remove(hBlendState, &pBlendState), "BlendState not found in idTable");
        EZ_VERIFY(m_BlendStateTable.Remove(pBlendState->GetDescription().CalculateHash()), "BlendState not found in de-duplication table");

        DestroyBlendStatePlatform(pBlendState);

        break;
      }
      case GALObjectType::DepthStencilState:
      {
        ezGALDepthStencilStateHandle hDepthStencilState(ezGAL::ez16_16Id(deadObject.m_uiHandle));
        ezGALDepthStencilState* pDepthStencilState = nullptr;

        EZ_VERIFY(m_DepthStencilStates.Remove(hDepthStencilState, &pDepthStencilState), "DepthStencilState not found in idTable");
        EZ_VERIFY(m_DepthStencilStateTable.Remove(pDepthStencilState->GetDescription().CalculateHash()),
          "DepthStencilState not found in de-duplication table");

        DestroyDepthStencilStatePlatform(pDepthStencilState);

        break;
      }
      case GALObjectType::RasterizerState:
      {
        ezGALRasterizerStateHandle hRasterizerState(ezGAL::ez16_16Id(deadObject.m_uiHandle));
        ezGALRasterizerState* pRasterizerState = nullptr;

        EZ_VERIFY(m_RasterizerStates.Remove(hRasterizerState, &pRasterizerState), "RasterizerState not found in idTable");
        EZ_VERIFY(
          m_RasterizerStateTable.Remove(pRasterizerState->GetDescription().CalculateHash()), "RasterizerState not found in de-duplication table");

        DestroyRasterizerStatePlatform(pRasterizerState);

        break;
      }
      case GALObjectType::SamplerState:
      {
        ezGALSamplerStateHandle hSamplerState(ezGAL::ez16_16Id(deadObject.m_uiHandle));
        ezGALSamplerState* pSamplerState = nullptr;

        EZ_VERIFY(m_SamplerStates.Remove(hSamplerState, &pSamplerState), "SamplerState not found in idTable");
        EZ_VERIFY(m_SamplerStateTable.Remove(pSamplerState->GetDescription().CalculateHash()), "SamplerState not found in de-duplication table");

        DestroySamplerStatePlatform(pSamplerState);

        break;
      }
      case GALObjectType::Shader:
      {
        ezGALShaderHandle hShader(ezGAL::ez18_14Id(deadObject.m_uiHandle));
        ezGALShader* pShader = nullptr;

        m_Shaders.Remove(hShader, &pShader);

        DestroyShaderPlatform(pShader);

        break;
      }
      case GALObjectType::Buffer:
      {
        ezGALBufferHandle hBuffer(ezGAL::ez18_14Id(deadObject.m_uiHandle));
        ezGALBuffer* pBuffer = nullptr;

        m_Buffers.Remove(hBuffer, &pBuffer);

        DestroyViews(pBuffer);
        DestroyBufferPlatform(pBuffer);

        break;
      }
      case GALObjectType::Texture:
      {
        ezGALTextureHandle hTexture(ezGAL::ez18_14Id(deadObject.m_uiHandle));
        ezGALTexture* pTexture = nullptr;

        m_Textures.Remove(hTexture, &pTexture);

        DestroyViews(pTexture);
        DestroyTexturePlatform(pTexture);

        break;
      }
      case GALObjectType::ResourceView:
      {
        ezGALResourceViewHandle hResourceView(ezGAL::ez18_14Id(deadObject.m_uiHandle));
        ezGALResourceView* pResourceView = nullptr;

        m_ResourceViews.Remove(hResourceView, &pResourceView);

        ezGALResourceBase* pResource = pResourceView->m_pResource;
        EZ_ASSERT_DEBUG(pResource != nullptr, "");

        EZ_VERIFY(pResource->m_ResourceViews.Remove(pResourceView->GetDescription().CalculateHash()), "");
        pResourceView->m_pResource = nullptr;

        DestroyResourceViewPlatform(pResourceView);

        break;
      }
      case GALObjectType::RenderTargetView:
      {
        ezGALRenderTargetViewHandle hRenderTargetView(ezGAL::ez18_14Id(deadObject.m_uiHandle));
        ezGALRenderTargetView* pRenderTargetView = nullptr;

        m_RenderTargetViews.Remove(hRenderTargetView, &pRenderTargetView);

        ezGALTexture* pTexture = pRenderTargetView->m_pTexture;
        EZ_ASSERT_DEBUG(pTexture != nullptr, "");
        EZ_VERIFY(pTexture->m_RenderTargetViews.Remove(pRenderTargetView->GetDescription().CalculateHash()), "");
        pRenderTargetView->m_pTexture = nullptr;

        DestroyRenderTargetViewPlatform(pRenderTargetView);

        break;
      }
      case GALObjectType::UnorderedAccessView:
      {
        ezGALUnorderedAccessViewHandle hUnorderedAccessViewHandle(ezGAL::ez18_14Id(deadObject.m_uiHandle));
        ezGALUnorderedAccessView* pUnorderedAccesssView = nullptr;

        m_UnorderedAccessViews.Remove(hUnorderedAccessViewHandle, &pUnorderedAccesssView);

        ezGALResourceBase* pResource = pUnorderedAccesssView->m_pResource;
        EZ_ASSERT_DEBUG(pResource != nullptr, "");

        EZ_VERIFY(pResource->m_UnorderedAccessViews.Remove(pUnorderedAccesssView->GetDescription().CalculateHash()), "");
        pUnorderedAccesssView->m_pResource = nullptr;

        DestroyUnorderedAccessViewPlatform(pUnorderedAccesssView);

        break;
      }
      case GALObjectType::SwapChain:
      {
        ezGALSwapChainHandle hSwapChain(ezGAL::ez16_16Id(deadObject.m_uiHandle));
        ezGALSwapChain* pSwapChain = nullptr;

        m_SwapChains.Remove(hSwapChain, &pSwapChain);

        if (pSwapChain != nullptr)
        {
          DestroySwapChainPlatform(pSwapChain);
        }

        break;
      }
      case GALObjectType::Fence:
      {
        ezGALFenceHandle hFence(ezGAL::ez20_12Id(deadObject.m_uiHandle));
        ezGALFence* pFence = nullptr;

        m_Fences.Remove(hFence, &pFence);

        DestroyFencePlatform(pFence);

        break;
      }
      case GALObjectType::Query:
      {
        ezGALQueryHandle hQuery(ezGAL::ez20_12Id(deadObject.m_uiHandle));
        ezGALQuery* pQuery = nullptr;

        m_Queries.Remove(hQuery, &pQuery);

        DestroyQueryPlatform(pQuery);

        break;
      }
      case GALObjectType::VertexDeclaration:
      {
        ezGALVertexDeclarationHandle hVertexDeclaration(ezGAL::ez18_14Id(deadObject.m_uiHandle));
        ezGALVertexDeclaration* pVertexDeclaration = nullptr;

        m_VertexDeclarations.Remove(hVertexDeclaration, &pVertexDeclaration);
        m_VertexDeclarationTable.Remove(pVertexDeclaration->GetDescription().CalculateHash());

        DestroyVertexDeclarationPlatform(pVertexDeclaration);

        break;
      }
      default:
        EZ_ASSERT_NOT_IMPLEMENTED;
    }
  }

  m_DeadObjects.Clear();
}

EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_Device_Implementation_Device);
