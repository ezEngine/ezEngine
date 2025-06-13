#include <RendererFoundation/RendererFoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Utilities/Stats.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/SharedTextureSwapChain.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/DynamicBuffer.h>
#include <RendererFoundation/Resources/ProxyTexture.h>
#include <RendererFoundation/Resources/ReadbackTexture.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/ResourceView.h>
#include <RendererFoundation/Resources/UnorderedAccesView.h>
#include <RendererFoundation/Shader/Shader.h>
#include <RendererFoundation/Shader/VertexDeclaration.h>
#include <RendererFoundation/State/ComputePipeline.h>
#include <RendererFoundation/State/GraphicsPipeline.h>
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
      DynamicBuffer,
      Texture,
      ReadbackTexture,
      ReadbackBuffer,
      TextureResourceView,
      BufferResourceView,
      RenderTargetView,
      TextureUnorderedAccessView,
      BufferUnorderedAccessView,
      SwapChain,
      VertexDeclaration,
      GraphicsPipeline,
      ComputePipeline
    };
  };

  static_assert(sizeof(ezGALBlendStateHandle) == sizeof(ezUInt32));
  static_assert(sizeof(ezGALDepthStencilStateHandle) == sizeof(ezUInt32));
  static_assert(sizeof(ezGALRasterizerStateHandle) == sizeof(ezUInt32));
  static_assert(sizeof(ezGALSamplerStateHandle) == sizeof(ezUInt32));
  static_assert(sizeof(ezGALShaderHandle) == sizeof(ezUInt32));
  static_assert(sizeof(ezGALBufferHandle) == sizeof(ezUInt32));
  static_assert(sizeof(ezGALTextureHandle) == sizeof(ezUInt32));
  static_assert(sizeof(ezGALTextureResourceViewHandle) == sizeof(ezUInt32));
  static_assert(sizeof(ezGALBufferResourceViewHandle) == sizeof(ezUInt32));
  static_assert(sizeof(ezGALRenderTargetViewHandle) == sizeof(ezUInt32));
  static_assert(sizeof(ezGALTextureUnorderedAccessViewHandle) == sizeof(ezUInt32));
  static_assert(sizeof(ezGALBufferUnorderedAccessViewHandle) == sizeof(ezUInt32));
  static_assert(sizeof(ezGALSwapChainHandle) == sizeof(ezUInt32));
  static_assert(sizeof(ezGALVertexDeclarationHandle) == sizeof(ezUInt32));
  static_assert(sizeof(ezGALGraphicsPipelineHandle) == sizeof(ezUInt32));
  static_assert(sizeof(ezGALComputePipelineHandle) == sizeof(ezUInt32));
} // namespace

ezGALDevice* ezGALDevice::s_pDefaultDevice = nullptr;
ezEvent<const ezGALDeviceEvent&, ezMutex> ezGALDevice::s_Events;

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

    if (!m_TextureResourceViews.IsEmpty())
      ezLog::Warning("{0} texture resource views have not been cleaned up", m_TextureResourceViews.GetCount());

    if (!m_BufferResourceViews.IsEmpty())
      ezLog::Warning("{0} buffer resource views have not been cleaned up", m_BufferResourceViews.GetCount());

    if (!m_RenderTargetViews.IsEmpty())
      ezLog::Warning("{0} render target views have not been cleaned up", m_RenderTargetViews.GetCount());

    if (!m_TextureUnorderedAccessViews.IsEmpty())
      ezLog::Warning("{0} texture unordered access views have not been cleaned up", m_TextureUnorderedAccessViews.GetCount());

    if (!m_BufferUnorderedAccessViews.IsEmpty())
      ezLog::Warning("{0} buffer unordered access views have not been cleaned up", m_BufferUnorderedAccessViews.GetCount());

    if (!m_SwapChains.IsEmpty())
      ezLog::Warning("{0} swap chains have not been cleaned up", m_SwapChains.GetCount());

    if (!m_VertexDeclarations.IsEmpty())
      ezLog::Warning("{0} vertex declarations have not been cleaned up", m_VertexDeclarations.GetCount());

    if (!m_GraphicsPipelines.IsEmpty())
      ezLog::Warning("{0} graphics pipelines have not been cleaned up", m_GraphicsPipelines.GetCount());

    if (!m_ComputePipelines.IsEmpty())
      ezLog::Warning("{0} Compute pipelines have not been cleaned up", m_ComputePipelines.GetCount());
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

  ezGALSharedTextureSwapChain::SetFactoryMethod([this](const ezGALSharedTextureSwapChainCreationDescription& desc) -> ezGALSwapChainHandle
    { return CreateSwapChain([&desc](ezAllocator* pAllocator) -> ezGALSwapChain*
        { return EZ_NEW(pAllocator, ezGALSharedTextureSwapChain, desc); }); });

  // Fill the capabilities
  FillCapabilitiesPlatform();

  ezLog::Info("Adapter: '{}' - {} VRAM, {} Sys RAM, {} Shared RAM", m_Capabilities.m_sAdapterName, ezArgFileSize(m_Capabilities.m_uiDedicatedVRAM),
    ezArgFileSize(m_Capabilities.m_uiDedicatedSystemRAM), ezArgFileSize(m_Capabilities.m_uiSharedSystemRAM));

  if (!m_Capabilities.m_bHardwareAccelerated)
  {
    ezLog::Warning("Selected graphics adapter has no hardware acceleration.");
  }

  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezProfilingSystem::InitializeGPUData();

  {
    ezGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type = ezGALDeviceEvent::AfterInit;
    s_Events.Broadcast(e);
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
    s_Events.Broadcast(e);
  }

  DestroyDeadObjects();

  // make sure we are not listed as the default device anymore
  if (ezGALDevice::HasDefaultDevice() && ezGALDevice::GetDefaultDevice() == this)
  {
    ezGALDevice::SetDefaultDevice(nullptr);
  }

  EZ_ASSERT_DEBUG(m_uiShaders == 0, "Error in counting deduplicated GAL resources");
  EZ_ASSERT_DEBUG(m_uiVertexDeclarations == 0, "Error in counting deduplicated GAL resources");
  EZ_ASSERT_DEBUG(m_uiBlendStates == 0, "Error in counting deduplicated GAL resources");
  EZ_ASSERT_DEBUG(m_uiDepthStencilStates == 0, "Error in counting deduplicated GAL resources");
  EZ_ASSERT_DEBUG(m_uiRasterizerStates == 0, "Error in counting deduplicated GAL resources");
  EZ_ASSERT_DEBUG(m_uiGraphicsPipelines == 0, "Error in counting deduplicated GAL resources");
  EZ_ASSERT_DEBUG(m_uiComputePipelines == 0, "Error in counting deduplicated GAL resources");
  EZ_ASSERT_DEBUG(m_uiSamplerStates == 0, "Error in counting deduplicated GAL resources");

  return ShutdownPlatform();
}

ezStringView ezGALDevice::GetRenderer()
{
  return GetRendererPlatform();
}

ezGALCommandEncoder* ezGALDevice::BeginCommands(const char* szName)
{
  {
    EZ_PROFILE_SCOPE("BeforeBeginCommands");
    ezGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type = ezGALDeviceEvent::BeforeBeginCommands;
    s_Events.Broadcast(e, 1);
  }
  {
    EZ_GALDEVICE_LOCK_AND_CHECK();

    EZ_ASSERT_DEV(m_pCommandEncoder == nullptr, "Nested Passes are not allowed: You must call ezGALDevice::EndCommands before you can call ezGALDevice::BeginCommands again");
    m_pCommandEncoder = BeginCommandsPlatform(szName);
  }
  {
    EZ_PROFILE_SCOPE("AfterBeginCommands");
    ezGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type = ezGALDeviceEvent::AfterBeginCommands;
    e.m_pCommandEncoder = m_pCommandEncoder;
    s_Events.Broadcast(e, 1);
  }
  return m_pCommandEncoder;
}

void ezGALDevice::EndCommands(ezGALCommandEncoder* pCommandEncoder)
{
  {
    EZ_PROFILE_SCOPE("BeforeEndCommands");
    ezGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type = ezGALDeviceEvent::BeforeEndCommands;
    e.m_pCommandEncoder = pCommandEncoder;
    s_Events.Broadcast(e, 1);
  }
  {
    EZ_GALDEVICE_LOCK_AND_CHECK();
    EZ_ASSERT_DEV(m_pCommandEncoder != nullptr, "You must have called ezGALDevice::BeginCommands before you can call ezGALDevice::EndCommands");
    m_EncoderStats += m_pCommandEncoder->GetStats();
    m_pCommandEncoder->ResetStats();

    m_pCommandEncoder = nullptr;
    EndCommandsPlatform(pCommandEncoder);
  }
  {
    EZ_PROFILE_SCOPE("AfterEndCommands");
    ezGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type = ezGALDeviceEvent::AfterEndCommands;
    s_Events.Broadcast(e, 1);
  }
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
      m_uiBlendStates++;
      return hBlendState;
    }
  }

  ezGALBlendState* pBlendState = CreateBlendStatePlatform(desc);

  if (pBlendState != nullptr)
  {
    EZ_ASSERT_DEBUG(pBlendState->GetDescription().CalculateHash() == uiHash, "BlendState hash doesn't match");

    pBlendState->AddRef();
    m_uiBlendStates++;

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
    m_uiBlendStates--;

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
      m_uiDepthStencilStates++;
      return hDepthStencilState;
    }
  }

  ezGALDepthStencilState* pDepthStencilState = CreateDepthStencilStatePlatform(desc);

  if (pDepthStencilState != nullptr)
  {
    EZ_ASSERT_DEBUG(pDepthStencilState->GetDescription().CalculateHash() == uiHash, "DepthStencilState hash doesn't match");

    pDepthStencilState->AddRef();
    m_uiDepthStencilStates++;

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
    m_uiDepthStencilStates--;

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
      m_uiRasterizerStates++;
      return hRasterizerState;
    }
  }

  ezGALRasterizerState* pRasterizerState = CreateRasterizerStatePlatform(desc);

  if (pRasterizerState != nullptr)
  {
    EZ_ASSERT_DEBUG(pRasterizerState->GetDescription().CalculateHash() == uiHash, "RasterizerState hash doesn't match");

    pRasterizerState->AddRef();
    m_uiRasterizerStates++;

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
    m_uiRasterizerStates--;

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
      m_uiSamplerStates++;
      return hSamplerState;
    }
  }

  ezGALSamplerState* pSamplerState = CreateSamplerStatePlatform(desc);

  if (pSamplerState != nullptr)
  {
    EZ_ASSERT_DEBUG(pSamplerState->GetDescription().CalculateHash() == uiHash, "SamplerState hash doesn't match");

    pSamplerState->AddRef();
    m_uiSamplerStates++;

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
    m_uiSamplerStates--;

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

ezGALGraphicsPipelineHandle ezGALDevice::CreateGraphicsPipeline(const ezGALGraphicsPipelineCreationDescription& desc)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  auto IncreaseReference = [&](const auto& idTable, ezUInt32& ref_uiCount, auto handle, GALObjectType::Enum type)
  {
    auto pRes = idTable[handle];
    if (pRes->GetRefCount() == 0)
    {
      ReviveDeadObject(type, handle);
    }
    pRes->AddRef();
    ref_uiCount++;
  };

  // Hash desc and return potential existing one (including inc. refcount)
  ezUInt32 uiHash = desc.CalculateHash();
  {
    ezGALGraphicsPipelineHandle hGraphicsPipeline;
    if (m_GraphicsPipelineTable.TryGetValue(uiHash, hGraphicsPipeline))
    {
      ezGALGraphicsPipeline* pGraphicsPipeline = m_GraphicsPipelines[hGraphicsPipeline];
      if (pGraphicsPipeline->GetRefCount() == 0)
      {
        ReviveDeadObject(GALObjectType::GraphicsPipeline, hGraphicsPipeline);
        IncreaseReference(m_Shaders, m_uiShaders, desc.m_hShader, GALObjectType::Shader);
        IncreaseReference(m_RasterizerStates, m_uiRasterizerStates, desc.m_hRasterizerState, GALObjectType::RasterizerState);
        IncreaseReference(m_BlendStates, m_uiBlendStates, desc.m_hBlendState, GALObjectType::BlendState);
        IncreaseReference(m_DepthStencilStates, m_uiDepthStencilStates, desc.m_hDepthStencilState, GALObjectType::DepthStencilState);
        if (!desc.m_hVertexDeclaration.IsInvalidated())
        {
          IncreaseReference(m_VertexDeclarations, m_uiVertexDeclarations, desc.m_hVertexDeclaration, GALObjectType::VertexDeclaration);
        }
      }

      pGraphicsPipeline->AddRef();
      m_uiGraphicsPipelines++;
      return hGraphicsPipeline;
    }
  }

  if (desc.m_hBlendState.IsInvalidated() || desc.m_hDepthStencilState.IsInvalidated() || desc.m_hRasterizerState.IsInvalidated() || desc.m_hShader.IsInvalidated())
  {
    ezLog::Error("An essential handle was invalid. Only the m_hVertexDeclaration handle can be invalid.");
    return {};
  }

  ezGALGraphicsPipeline* pGraphicsPipeline = CreateGraphicsPipelinePlatform(desc);

  if (pGraphicsPipeline != nullptr)
  {
    EZ_ASSERT_DEBUG(pGraphicsPipeline->GetDescription().CalculateHash() == uiHash, "GraphicsPipeline hash doesn't match");

    pGraphicsPipeline->AddRef();
    m_uiGraphicsPipelines++;

    ezGALGraphicsPipelineHandle hGraphicsPipeline(m_GraphicsPipelines.Insert(pGraphicsPipeline));
    m_GraphicsPipelineTable.Insert(uiHash, hGraphicsPipeline);

    IncreaseReference(m_Shaders, m_uiShaders, desc.m_hShader, GALObjectType::Shader);
    IncreaseReference(m_RasterizerStates, m_uiRasterizerStates, desc.m_hRasterizerState, GALObjectType::RasterizerState);
    IncreaseReference(m_BlendStates, m_uiBlendStates, desc.m_hBlendState, GALObjectType::BlendState);
    IncreaseReference(m_DepthStencilStates, m_uiDepthStencilStates, desc.m_hDepthStencilState, GALObjectType::DepthStencilState);
    if (!desc.m_hVertexDeclaration.IsInvalidated())
    {
      IncreaseReference(m_VertexDeclarations, m_uiVertexDeclarations, desc.m_hVertexDeclaration, GALObjectType::VertexDeclaration);
    }

    return hGraphicsPipeline;
  }

  return ezGALGraphicsPipelineHandle();
}

void ezGALDevice::DestroyGraphicsPipeline(ezGALGraphicsPipelineHandle hGraphicsPipeline)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezGALGraphicsPipeline* pGraphicsPipeline = nullptr;

  if (m_GraphicsPipelines.TryGetValue(hGraphicsPipeline, pGraphicsPipeline))
  {
    pGraphicsPipeline->ReleaseRef();
    m_uiGraphicsPipelines--;

    if (pGraphicsPipeline->GetRefCount() == 0)
    {
      AddDeadObject(GALObjectType::GraphicsPipeline, hGraphicsPipeline);
      const ezGALGraphicsPipelineCreationDescription& desc = pGraphicsPipeline->GetDescription();

      DestroyShader(desc.m_hShader);
      DestroyRasterizerState(desc.m_hRasterizerState);
      DestroyBlendState(desc.m_hBlendState);
      DestroyDepthStencilState(desc.m_hDepthStencilState);
      if (!desc.m_hVertexDeclaration.IsInvalidated())
      {
        DestroyVertexDeclaration(desc.m_hVertexDeclaration);
      }
    }
  }
  else
  {
    ezLog::Warning("DestroyGraphicsPipeline called on invalid handle (double free?)");
  }
}


ezGALComputePipelineHandle ezGALDevice::CreateComputePipeline(const ezGALComputePipelineCreationDescription& desc)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  auto IncreaseReference = [&](const auto& idTable, ezUInt32& ref_uiCount, auto handle, GALObjectType::Enum type)
  {
    auto pRes = idTable[handle];
    if (pRes->GetRefCount() == 0)
    {
      ReviveDeadObject(type, handle);
    }
    pRes->AddRef();
    ref_uiCount++;
  };

  // Hash desc and return potential existing one (including inc. refcount)
  ezUInt32 uiHash = desc.CalculateHash();
  {
    ezGALComputePipelineHandle hComputePipeline;
    if (m_ComputePipelineTable.TryGetValue(uiHash, hComputePipeline))
    {
      ezGALComputePipeline* pComputePipeline = m_ComputePipelines[hComputePipeline];
      if (pComputePipeline->GetRefCount() == 0)
      {
        ReviveDeadObject(GALObjectType::ComputePipeline, hComputePipeline);
        IncreaseReference(m_Shaders, m_uiShaders, desc.m_hShader, GALObjectType::Shader);
      }

      pComputePipeline->AddRef();
      m_uiComputePipelines++;
      return hComputePipeline;
    }
  }

  if (desc.m_hShader.IsInvalidated())
  {
    ezLog::Error("Shader handle must be valid.");
    return {};
  }

  ezGALComputePipeline* pComputePipeline = CreateComputePipelinePlatform(desc);

  if (pComputePipeline != nullptr)
  {
    EZ_ASSERT_DEBUG(pComputePipeline->GetDescription().CalculateHash() == uiHash, "ComputePipeline hash doesn't match");

    pComputePipeline->AddRef();
    m_uiComputePipelines++;

    ezGALComputePipelineHandle hComputePipeline(m_ComputePipelines.Insert(pComputePipeline));
    m_ComputePipelineTable.Insert(uiHash, hComputePipeline);
    IncreaseReference(m_Shaders, m_uiShaders, desc.m_hShader, GALObjectType::Shader);

    return hComputePipeline;
  }

  return ezGALComputePipelineHandle();
}

void ezGALDevice::DestroyComputePipeline(ezGALComputePipelineHandle hComputePipeline)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezGALComputePipeline* pComputePipeline = nullptr;

  if (m_ComputePipelines.TryGetValue(hComputePipeline, pComputePipeline))
  {
    pComputePipeline->ReleaseRef();
    m_uiComputePipelines--;

    if (pComputePipeline->GetRefCount() == 0)
    {
      AddDeadObject(GALObjectType::ComputePipeline, hComputePipeline);
      const ezGALComputePipelineCreationDescription& desc = pComputePipeline->GetDescription();
      DestroyShader(desc.m_hShader);
    }
  }
  else
  {
    ezLog::Warning("DestroyComputePipeline called on invalid handle (double free?)");
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

  // Hash desc and return potential existing one (including inc. refcount)
  ezUInt32 uiHash = desc.CalculateHash();

  {
    ezGALShaderHandle hShader;
    if (m_ShaderTable.TryGetValue(uiHash, hShader))
    {
      ezGALShader* pShader = m_Shaders[hShader];
      if (pShader->GetRefCount() == 0)
      {
        ReviveDeadObject(GALObjectType::Shader, hShader);
      }

      pShader->AddRef();
      m_uiShaders++;
      return hShader;
    }
  }

  ezGALShader* pShader = CreateShaderPlatform(desc);

  if (pShader != nullptr)
  {
    EZ_ASSERT_DEBUG(pShader->GetDescription().CalculateHash() == uiHash, "Shader hash doesn't match");

    pShader->AddRef();
    m_uiShaders++;

    ezGALShaderHandle hShader(m_Shaders.Insert(pShader));
    m_ShaderTable.Insert(uiHash, hShader);

    return hShader;
  }

  return ezGALShaderHandle();
}

void ezGALDevice::DestroyShader(ezGALShaderHandle hShader)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezGALShader* pShader = nullptr;

  if (m_Shaders.TryGetValue(hShader, pShader))
  {
    pShader->ReleaseRef();
    m_uiShaders--;

    if (pShader->GetRefCount() == 0)
    {
      AddDeadObject(GALObjectType::Shader, hShader);
    }
  }
  else
  {
    ezLog::Warning("DestroyShader called on invalid handle (double free?)");
  }
}


ezGALBufferHandle ezGALDevice::CreateBuffer(const ezGALBufferCreationDescription& desc, ezArrayPtr<const ezUInt8> initialData)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  if (desc.m_uiTotalSize == 0)
  {
    ezLog::Error("Trying to create a buffer with size of 0 is not possible!");
    return ezGALBufferHandle();
  }

  if (desc.m_ResourceAccess.IsImmutable() && initialData.IsEmpty())
  {
    ezLog::Error("Trying to create an immutable buffer but not supplying initial data is not possible!");
    return ezGALBufferHandle();
  }

  ezUInt32 uiBufferSize = desc.m_uiTotalSize;
  if (initialData.GetCount() > 0 && uiBufferSize != initialData.GetCount())
  {
    ezLog::Error("Trying to create a buffer with invalid initial data.");
    return {};
  }

  if (desc.m_BufferFlags.IsSet(ezGALBufferUsageFlags::Transient) && !initialData.IsEmpty())
  {
    ezLog::Error("Transient buffers cannot have initial data.");
    return {};
  }
  if (desc.m_BufferFlags.IsSet(ezGALBufferUsageFlags::IndexBuffer) && desc.m_uiStructSize != 2 && desc.m_uiStructSize != 4)
  {
    ezLog::Error("IndexBuffer struct size must be either 2 or 4 but {} is set.", desc.m_uiStructSize);
  }
  if (desc.m_BufferFlags.IsSet(ezGALBufferUsageFlags::TexelBuffer) && desc.m_Format == ezGALResourceFormat::Invalid)
  {
    ezLog::Error("Texel buffers must have a valid m_Format set.");
    return {};
  }
  if (!desc.m_BufferFlags.IsSet(ezGALBufferUsageFlags::TexelBuffer) && desc.m_Format != ezGALResourceFormat::Invalid)
  {
    ezLog::Error("m_Format is only allowed if TexelBuffer flag is set.");
    return {};
  }
  if (desc.m_BufferFlags.IsSet(ezGALBufferUsageFlags::TexelBuffer) && (desc.m_uiTotalSize % (ezGALResourceFormat::GetBitsPerElement(desc.m_Format) / 8)) != 0)
  {
    ezLog::Error("TexelBuffer with format {} must have a size multiple of {}, but size is {}.", (ezUInt32)desc.m_Format, ezGALResourceFormat::GetBitsPerElement(desc.m_Format) / 8, desc.m_uiTotalSize);
    return {};
  }

  if (desc.m_BufferFlags.IsAnySet(ezGALBufferUsageFlags::StructuredBuffer | ezGALBufferUsageFlags::VertexBuffer | ezGALBufferUsageFlags::IndexBuffer) && desc.m_uiStructSize == 0)
  {
    ezLog::Error("m_uiStructSize must be != 0 if StructuredBuffer, IndexBuffer or VertexBuffer flag is set.");
    return {};
  }
  if (desc.m_BufferFlags.IsSet(ezGALBufferUsageFlags::StructuredBuffer) && (desc.m_uiTotalSize % desc.m_uiStructSize) != 0)
  {
    ezLog::Error("StructuredBuffer must have a size multiple of m_uiStructSize {}, but size is {}.", desc.m_uiStructSize, desc.m_uiTotalSize);
    return {};
  }
  if (!m_Capabilities.m_bSupportsTexelBuffer && desc.m_BufferFlags.IsSet(ezGALBufferUsageFlags::TexelBuffer))
  {
    ezLog::Error("TexelBuffer flag is not supported on this platform.");
    return {};
  }

  /// \todo Platform independent validation (buffer type supported)

  ezGALBuffer* pBuffer = CreateBufferPlatform(desc, initialData);

  return FinalizeBufferInternal(desc, pBuffer);
}

ezGALBufferHandle ezGALDevice::FinalizeBufferInternal(const ezGALBufferCreationDescription& desc, ezGALBuffer* pBuffer)
{
  EZ_ASSERT_DEBUG(m_Mutex.IsLocked(), "");

  if (pBuffer != nullptr)
  {
    ezGALBufferHandle hBuffer(m_Buffers.Insert(pBuffer));

    // Create default resource view
    if (desc.m_BufferFlags.IsSet(ezGALBufferUsageFlags::ShaderResource))
    {
      if (desc.m_BufferFlags.IsSet(ezGALBufferUsageFlags::TexelBuffer) && desc.m_Format != ezGALResourceFormat::Invalid)
      {
        ezGALBufferResourceViewCreationDescription viewDesc;
        viewDesc.m_hBuffer = hBuffer;
        viewDesc.m_Format = desc.m_Format;
        viewDesc.m_ResourceType = ezGALShaderResourceType::TexelBuffer;
        pBuffer->m_hDefaultTexelBufferView = CreateResourceView(viewDesc);
      }
      if (desc.m_BufferFlags.IsSet(ezGALBufferUsageFlags::StructuredBuffer))
      {
        ezGALBufferResourceViewCreationDescription viewDesc;
        viewDesc.m_hBuffer = hBuffer;
        viewDesc.m_ResourceType = ezGALShaderResourceType::StructuredBuffer;
        pBuffer->m_hDefaultStructuredBufferView = CreateResourceView(viewDesc);
      }
      if (desc.m_BufferFlags.IsSet(ezGALBufferUsageFlags::ByteAddressBuffer))
      {
        ezGALBufferResourceViewCreationDescription viewDesc;
        viewDesc.m_hBuffer = hBuffer;
        viewDesc.m_ResourceType = ezGALShaderResourceType::ByteAddressBuffer;
        pBuffer->m_hDefaultByteAddressBufferView = CreateResourceView(viewDesc);
      }
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

ezGALDynamicBufferHandle ezGALDevice::CreateDynamicBuffer(const ezGALBufferCreationDescription& description, ezStringView sDebugName)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  auto pBuffer = EZ_NEW(&m_Allocator, ezGALDynamicBuffer);
  pBuffer->Initialize(description, sDebugName);

  return ezGALDynamicBufferHandle(m_DynamicBuffers.Insert(pBuffer));
}

void ezGALDevice::DestroyDynamicBuffer(ezGALDynamicBufferHandle& inout_hBuffer)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezGALDynamicBuffer* pBuffer = nullptr;
  if (m_DynamicBuffers.TryGetValue(inout_hBuffer, pBuffer))
  {
    AddDeadObject(GALObjectType::DynamicBuffer, inout_hBuffer);
    inout_hBuffer.Invalidate();
  }
}

// Helper functions for buffers (for common, simple use cases)
ezGALBufferHandle ezGALDevice::CreateVertexBuffer(ezUInt32 uiVertexSize, ezUInt32 uiVertexCount, ezArrayPtr<const ezUInt8> initialData, bool bDataIsMutable /*= false */)
{
  ezGALBufferCreationDescription desc;
  desc.m_uiStructSize = uiVertexSize;
  desc.m_uiTotalSize = uiVertexSize * ezMath::Max(1u, uiVertexCount);
  desc.m_BufferFlags = ezGALBufferUsageFlags::VertexBuffer;
  desc.m_ResourceAccess.m_bImmutable = !initialData.IsEmpty() && !bDataIsMutable;

  return CreateBuffer(desc, initialData);
}

ezGALBufferHandle ezGALDevice::CreateIndexBuffer(ezGALIndexType::Enum indexType, ezUInt32 uiIndexCount, ezArrayPtr<const ezUInt8> initialData, bool bDataIsMutable /*= false*/)
{
  ezGALBufferCreationDescription desc;
  desc.m_uiStructSize = ezGALIndexType::GetSize(indexType);
  desc.m_uiTotalSize = desc.m_uiStructSize * ezMath::Max(1u, uiIndexCount);
  desc.m_BufferFlags = ezGALBufferUsageFlags::IndexBuffer;
  desc.m_ResourceAccess.m_bImmutable = !bDataIsMutable && !initialData.IsEmpty();

  return CreateBuffer(desc, initialData);
}

ezGALBufferHandle ezGALDevice::CreateConstantBuffer(ezUInt32 uiBufferSize)
{
  ezGALBufferCreationDescription desc;
  desc.m_uiStructSize = 0;
  desc.m_uiTotalSize = uiBufferSize;
  desc.m_BufferFlags = ezGALBufferUsageFlags::ConstantBuffer | ezGALBufferUsageFlags::Transient;
  desc.m_ResourceAccess.m_bImmutable = false;

  return CreateBuffer(desc);
}


ezGALTextureHandle ezGALDevice::CreateTexture(const ezGALTextureCreationDescription& desc, ezArrayPtr<ezGALSystemMemoryDescription> initialData)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  /// \todo Platform independent validation (desc width & height < platform maximum, format, etc.)

  if (desc.m_ResourceAccess.IsImmutable() && (initialData.IsEmpty() || initialData.GetCount() < desc.m_uiMipLevelCount) &&
      !desc.m_bAllowRenderTargetView)
  {
    ezLog::Error("Trying to create an immutable texture but not supplying initial data (or not enough data pointers) is not possible!");
    return ezGALTextureHandle();
  }

  if (desc.m_uiWidth == 0 || desc.m_uiHeight == 0)
  {
    ezLog::Error("Trying to create a texture with width or height == 0 is not possible!");
    return ezGALTextureHandle();
  }

  if (desc.m_Type != ezGALTextureType::Texture2DArray && desc.m_Type != ezGALTextureType::TextureCubeArray)
  {
    if (desc.m_uiArraySize != 1)
    {
      ezLog::Error("m_uiArraySize must be 1 for non array textures!");
      return ezGALTextureHandle();
    }
  }

  ezGALTexture* pTexture = CreateTexturePlatform(desc, initialData);

  return FinalizeTextureInternal(desc, pTexture);
}

ezGALTextureHandle ezGALDevice::FinalizeTextureInternal(const ezGALTextureCreationDescription& desc, ezGALTexture* pTexture)
{
  if (pTexture != nullptr)
  {
    ezGALTextureHandle hTexture(m_Textures.Insert(pTexture));

    // Create default resource view
    if (desc.m_bAllowShaderResourceView)
    {
      ezGALTextureResourceViewCreationDescription viewDesc;
      viewDesc.m_hTexture = hTexture;
      viewDesc.m_uiArraySize = desc.m_uiArraySize;
      pTexture->m_hDefaultResourceView = CreateResourceView(viewDesc);
    }

    // Create default render target view
    if (desc.m_bAllowRenderTargetView)
    {
      ezGALRenderTargetViewCreationDescription rtDesc;
      rtDesc.m_hTexture = hTexture;
      rtDesc.m_uiFirstSlice = 0;
      rtDesc.m_uiSliceCount = desc.m_uiArraySize;
      if (desc.m_Type == ezGALTextureType::TextureCube || desc.m_Type == ezGALTextureType::TextureCubeArray)
      {
        rtDesc.m_OverrideViewType = ezGALTextureType::Texture2DArray;
      }

      pTexture->m_hDefaultRenderTargetView = CreateRenderTargetView(rtDesc);
    }

    return hTexture;
  }

  return ezGALTextureHandle();
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
  EZ_IGNORE_UNUSED(parentDesc);
  EZ_ASSERT_DEV(parentDesc.m_Type == ezGALTextureType::TextureCube || parentDesc.m_Type == ezGALTextureType::Texture2DArray || parentDesc.m_Type == ezGALTextureType::TextureCubeArray, "Proxy textures can only be created for cubemaps or array textures.");

  ezGALProxyTexture* pProxyTexture = EZ_NEW(&m_Allocator, ezGALProxyTexture, *pParentTexture);
  ezGALTextureHandle hProxyTexture(m_Textures.Insert(pProxyTexture));

  const auto& desc = pProxyTexture->GetDescription();

  // Create default resource view
  if (desc.m_bAllowShaderResourceView)
  {
    ezGALTextureResourceViewCreationDescription viewDesc;
    viewDesc.m_hTexture = hProxyTexture;
    viewDesc.m_uiFirstArraySlice = uiSlice;
    viewDesc.m_uiArraySize = 1;

    pProxyTexture->m_hDefaultResourceView = CreateResourceView(viewDesc);
  }

  // Create default render target view
  // if (desc.m_bAllowRenderTargetView)
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

ezGALTextureHandle ezGALDevice::CreateSharedTexture(const ezGALTextureCreationDescription& desc, ezArrayPtr<ezGALSystemMemoryDescription> initialData)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  /// \todo Platform independent validation (desc width & height < platform maximum, format, etc.)

  if (desc.m_ResourceAccess.IsImmutable() && (initialData.IsEmpty() || initialData.GetCount() < desc.m_uiMipLevelCount) &&
      !desc.m_bAllowRenderTargetView)
  {
    ezLog::Error("Trying to create an immutable texture but not supplying initial data (or not enough data pointers) is not possible!");
    return ezGALTextureHandle();
  }

  if (desc.m_uiWidth == 0 || desc.m_uiHeight == 0)
  {
    ezLog::Error("Trying to create a texture with width or height == 0 is not possible!");
    return ezGALTextureHandle();
  }

  if (desc.m_pExisitingNativeObject != nullptr)
  {
    ezLog::Error("Shared textures cannot be created on exiting native objects!");
    return ezGALTextureHandle();
  }

  if (desc.m_Type != ezGALTextureType::Texture2DShared)
  {
    ezLog::Error("Only ezGALTextureType::Texture2DShared is supported for shared textures!");
    return ezGALTextureHandle();
  }

  ezGALTexture* pTexture = CreateSharedTexturePlatform(desc, initialData, ezGALSharedTextureType::Exported, {});

  return FinalizeTextureInternal(desc, pTexture);
}

ezGALTextureHandle ezGALDevice::OpenSharedTexture(const ezGALTextureCreationDescription& desc, ezGALPlatformSharedHandle hSharedHandle)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  if (desc.m_pExisitingNativeObject != nullptr)
  {
    ezLog::Error("Shared textures cannot be created on exiting native objects!");
    return ezGALTextureHandle();
  }

  if (desc.m_Type != ezGALTextureType::Texture2DShared)
  {
    ezLog::Error("Only ezGALTextureType::Texture2DShared is supported for shared textures!");
    return ezGALTextureHandle();
  }

  if (desc.m_uiWidth == 0 || desc.m_uiHeight == 0)
  {
    ezLog::Error("Trying to create a texture with width or height == 0 is not possible!");
    return ezGALTextureHandle();
  }

  ezGALTexture* pTexture = CreateSharedTexturePlatform(desc, {}, ezGALSharedTextureType::Imported, hSharedHandle);

  return FinalizeTextureInternal(desc, pTexture);
}

void ezGALDevice::DestroySharedTexture(ezGALTextureHandle hSharedTexture)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezGALTexture* pTexture = nullptr;
  if (m_Textures.TryGetValue(hSharedTexture, pTexture))
  {
    EZ_ASSERT_DEV(pTexture->GetDescription().m_Type == ezGALTextureType::Texture2DShared, "Given texture is not a shared texture texture");

    AddDeadObject(GALObjectType::Texture, hSharedTexture);
  }
  else
  {
    ezLog::Warning("DestroySharedTexture called on invalid handle (double free?)");
  }
}

ezGALReadbackTextureHandle ezGALDevice::CreateReadbackTexture(const ezGALTextureCreationDescription& description)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  if (description.m_uiWidth == 0 || description.m_uiHeight == 0)
  {
    ezLog::Error("Trying to create a texture with width or height == 0 is not possible!");
    return ezGALReadbackTextureHandle();
  }

  ezGALReadbackTexture* pReadbackTexture = CreateReadbackTexturePlatform(description);
  ezGALReadbackTextureHandle hTexture(m_ReadbackTextures.Insert(pReadbackTexture));
  return hTexture;
}

void ezGALDevice::DestroyReadbackTexture(ezGALReadbackTextureHandle hTexture)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezGALReadbackTexture* pTexture = nullptr;
  if (m_ReadbackTextures.TryGetValue(hTexture, pTexture))
  {
    AddDeadObject(GALObjectType::ReadbackTexture, hTexture);
  }
  else
  {
    ezLog::Warning("DestroyReadbackTexture called on invalid handle (double free?)");
  }
}

ezGALReadbackBufferHandle ezGALDevice::CreateReadbackBuffer(const ezGALBufferCreationDescription& description)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezGALReadbackBuffer* pReadbackBuffer = CreateReadbackBufferPlatform(description);
  ezGALReadbackBufferHandle hBuffer(m_ReadbackBuffers.Insert(pReadbackBuffer));
  return hBuffer;
}

void ezGALDevice::DestroyReadbackBuffer(ezGALReadbackBufferHandle hBuffer)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezGALReadbackBuffer* pBuffer = nullptr;
  if (m_ReadbackBuffers.TryGetValue(hBuffer, pBuffer))
  {
    AddDeadObject(GALObjectType::ReadbackBuffer, hBuffer);
  }
  else
  {
    ezLog::Warning("DestroyReadbackBuffer called on invalid handle (double free?)");
  }
}

void ezGALDevice::UpdateBufferForNextFrame(ezGALBufferHandle hBuffer, ezConstByteArrayPtr sourceData, ezUInt32 uiDestOffset /*= 0*/)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  if (const ezGALBuffer* pBuffer = GetBuffer(hBuffer))
  {
    if (uiDestOffset + sourceData.GetCount() > pBuffer->GetDescription().m_uiTotalSize)
    {
      ezLog::Error("Trying to update buffer outside of its bounds!");
      return;
    }

    UpdateBufferForNextFramePlatform(pBuffer, sourceData, uiDestOffset);
  }
  else
  {
    ezLog::Error("No valid buffer handle given to update!");
  }
}

void ezGALDevice::UpdateTextureForNextFrame(ezGALTextureHandle hTexture, const ezGALSystemMemoryDescription& sourceData, const ezGALTextureSubresource& destinationSubResource /*= {}*/, const ezBoundingBoxu32& destinationBox /*= ezBoundingBoxu32::MakeInvalid()*/)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  if (const ezGALTexture* pTexture = GetTexture(hTexture))
  {
    auto& desc = pTexture->GetDescription();

    const bool bDestBoxIsValid = destinationBox.IsValid() && destinationBox.GetExtents().IsZero() == false;
    if (bDestBoxIsValid && (destinationBox.m_vMax.x > desc.m_uiWidth || destinationBox.m_vMax.y > desc.m_uiHeight || destinationBox.m_vMax.z > desc.m_uiDepth))
    {
      ezLog::Error("Trying to update texture outside of its bounds!");
      return;
    }

    const ezUInt32 uiWidth = bDestBoxIsValid ? ezMath::Max(destinationBox.m_vMax.x - destinationBox.m_vMin.x, 1u) : desc.m_uiWidth;
    const ezUInt32 uiHeight = bDestBoxIsValid ? ezMath::Max(destinationBox.m_vMax.y - destinationBox.m_vMin.y, 1u) : desc.m_uiHeight;
    const ezUInt32 uiDepth = bDestBoxIsValid ? ezMath::Max(destinationBox.m_vMax.z - destinationBox.m_vMin.z, 1u) : desc.m_uiDepth;

    const ezUInt32 uiRowPitch = uiWidth * ezGALResourceFormat::GetBitsPerElement(desc.m_Format) / 8;
    const ezUInt32 uiSlicePitch = uiRowPitch * uiHeight;
    if (sourceData.m_uiRowPitch != uiRowPitch)
    {
      ezLog::Error("Invalid row pitch. Expected {0} got {1}", uiRowPitch, sourceData.m_uiRowPitch);
      return;
    }

    if (sourceData.m_uiSlicePitch != 0 && sourceData.m_uiSlicePitch != uiSlicePitch)
    {
      ezLog::Error("Invalid slice pitch. Expected {0} got {1}", uiSlicePitch, sourceData.m_uiSlicePitch);
      return;
    }

    if (sourceData.m_pData.GetCount() < uiSlicePitch * uiDepth)
    {
      ezLog::Error("Not enough data provided to update texture");
      return;
    }

    ezGALSystemMemoryDescription finalSourceData = sourceData;
    if (finalSourceData.m_uiSlicePitch == 0)
    {
      finalSourceData.m_uiSlicePitch = uiSlicePitch;
    }

    ezBoundingBoxu32 finalDestBox = destinationBox;
    if (bDestBoxIsValid)
    {
      finalDestBox.m_vMax = finalDestBox.m_vMin + ezVec3U32(uiWidth, uiHeight, uiDepth);
    }
    else
    {
      finalDestBox.m_vMin = ezVec3U32(0, 0, 0);
      finalDestBox.m_vMax = ezVec3U32(desc.m_uiWidth, desc.m_uiHeight, desc.m_uiDepth);
    }

    UpdateTextureForNextFramePlatform(pTexture, finalSourceData, destinationSubResource, finalDestBox);
  }
  else
  {
    ezLog::Error("No valid texture handle given to update!");
  }
}

ezGALTextureResourceViewHandle ezGALDevice::GetDefaultResourceView(ezGALTextureHandle hTexture)
{
  if (const ezGALTexture* pTexture = GetTexture(hTexture))
  {
    return pTexture->m_hDefaultResourceView;
  }

  return ezGALTextureResourceViewHandle();
}

ezGALBufferResourceViewHandle ezGALDevice::GetDefaultResourceView(ezGALBufferHandle hBuffer, ezEnum<ezGALShaderResourceType> slotType)
{
  if (const ezGALBuffer* pBuffer = GetBuffer(hBuffer))
  {
    if (slotType == ezGALShaderResourceType::Unknown)
    {
      const ezBitflags<ezGALBufferUsageFlags> srvFlags = pBuffer->GetDescription().m_BufferFlags & (ezGALBufferUsageFlags::TexelBuffer | ezGALBufferUsageFlags::StructuredBuffer | ezGALBufferUsageFlags::ByteAddressBuffer);
      const ezUInt32 uiSrvFlagsCount = ezMath::CountBits(static_cast<ezUInt32>(srvFlags.GetValue()));
      if (uiSrvFlagsCount != 1)
      {
        ezLog::Error("GetDefaultResourceView failed: slotType is ambiguous as there are {} SRV types possible on the buffer. Explicitly set the slotType.", uiSrvFlagsCount);
        return {};
      }
      if (srvFlags.IsSet(ezGALBufferUsageFlags::TexelBuffer))
        slotType = ezGALShaderResourceType::TexelBuffer;
      else if (srvFlags.IsSet(ezGALBufferUsageFlags::StructuredBuffer))
        slotType = ezGALShaderResourceType::StructuredBuffer;
      else if (srvFlags.IsSet(ezGALBufferUsageFlags::ByteAddressBuffer))
        slotType = ezGALShaderResourceType::ByteAddressBuffer;
    }

    ezGALBufferResourceViewHandle hView;
    switch (slotType)
    {
      case ezGALShaderResourceType::TexelBuffer:
        hView = pBuffer->m_hDefaultTexelBufferView;
        break;
      case ezGALShaderResourceType::StructuredBuffer:
        hView = pBuffer->m_hDefaultStructuredBufferView;
        break;
      case ezGALShaderResourceType::ByteAddressBuffer:
        hView = pBuffer->m_hDefaultByteAddressBufferView;
        break;
      default:
        ezLog::Error("GetDefaultResourceView failed: slotType of {} is invalid. Must be TexelBuffer, StructuredBuffer, ByteAddressBuffer.", (ezUInt32)slotType);
        return {};
    }
    if (hView.IsInvalidated())
    {
      ezLog::Error("GetDefaultResourceView failed: handle is invalid.");
    }
    return hView;
  }

  return {};
}

ezGALTextureResourceViewHandle ezGALDevice::CreateResourceView(const ezGALTextureResourceViewCreationDescription& desc)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezGALTexture* pResource = nullptr;

  if (!desc.m_hTexture.IsInvalidated())
    pResource = Get<TextureTable, ezGALTexture>(desc.m_hTexture, m_Textures);

  if (pResource == nullptr)
  {
    ezLog::Error("No valid texture handle given for resource view creation!");
    return ezGALTextureResourceViewHandle();
  }

  const ezEnum<ezGALTextureType> type = desc.m_OverrideViewType != ezGALTextureType::Invalid ? desc.m_OverrideViewType : pResource->GetDescription().m_Type;
  if (type != ezGALTextureType::Texture2DArray && type != ezGALTextureType::TextureCubeArray)
  {
    if (desc.m_uiArraySize != 1)
    {
      ezLog::Error("m_uiArraySize must be 1 for non array textures!");
      return ezGALTextureResourceViewHandle();
    }
  }

  // Hash desc and return potential existing one
  const ezUInt32 uiHash = desc.CalculateHash();
  if (auto hResourceView = TryGetView<ezGALTextureResourceViewHandle, ezGALTextureResourceView>(uiHash, m_TextureResourceViews, pResource->m_ResourceViews); !hResourceView.IsInvalidated())
    return hResourceView;

  ezGALTextureResourceView* pResourceView = CreateResourceViewPlatform(pResource, desc);
  return InsertView<ezGALTextureResourceViewHandle, ezGALTextureResourceView>(uiHash, pResourceView, m_TextureResourceViews, pResource->m_ResourceViews);
}

ezGALBufferResourceViewHandle ezGALDevice::CreateResourceView(ezGALBufferResourceViewCreationDescription desc)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezGALBuffer* pResource = nullptr;
  if (!desc.m_hBuffer.IsInvalidated())
    pResource = Get<BufferTable, ezGALBuffer>(desc.m_hBuffer, m_Buffers);

  if (pResource == nullptr)
  {
    ezLog::Error("No valid buffer handle given for resource view creation.");
    return {};
  }
  const ezGALBufferCreationDescription& bufferDesc = pResource->GetDescription();
  const ezBitflags<ezGALBufferUsageFlags> flags = bufferDesc.m_BufferFlags;
  if (!flags.IsSet(ezGALBufferUsageFlags::ShaderResource))
  {
    ezLog::Error("Resource view creation failed as buffer does not have ShaderResource flag set.");
    return {};
  }
  if (desc.m_uiByteCount == EZ_GAL_WHOLE_SIZE)
  {
    desc.m_uiByteCount = bufferDesc.m_uiTotalSize - desc.m_uiByteOffset;
  }
  if (desc.m_ResourceType == ezGALShaderResourceType::Unknown)
  {
    const ezBitflags<ezGALBufferUsageFlags> srvFlags = flags & (ezGALBufferUsageFlags::TexelBuffer | ezGALBufferUsageFlags::StructuredBuffer | ezGALBufferUsageFlags::ByteAddressBuffer);
    const ezUInt32 uiSrvFlagsCount = ezMath::CountBits(static_cast<ezUInt32>(srvFlags.GetValue()));
    if (uiSrvFlagsCount != 1)
    {
      ezLog::Error("Resource view creation failed as m_ResourceType == Unknown but there are {} SRV flags set, making the SRV ambiguous. Explicitly set the desired resource type to TexelBuffer, StructuredBuffer or ByteAddressBuffer.", uiSrvFlagsCount);
      return {};
    }
    if (srvFlags.IsSet(ezGALBufferUsageFlags::TexelBuffer))
      desc.m_ResourceType = ezGALShaderResourceType::TexelBuffer;
    else if (srvFlags.IsSet(ezGALBufferUsageFlags::StructuredBuffer))
      desc.m_ResourceType = ezGALShaderResourceType::StructuredBuffer;
    else if (srvFlags.IsSet(ezGALBufferUsageFlags::ByteAddressBuffer))
      desc.m_ResourceType = ezGALShaderResourceType::ByteAddressBuffer;
  }
  switch (desc.m_ResourceType)
  {
    case ezGALShaderResourceType::TexelBuffer:
    case ezGALShaderResourceType::StructuredBuffer:
    case ezGALShaderResourceType::ByteAddressBuffer:
      break;
    default:
    {
      ezLog::Error("Resource view creation failed as the resource type of {} is not supported. It must be TexelBuffer, StructuredBuffer or ByteAddressBuffer.", (ezUInt32)desc.m_ResourceType.GetValue());
      return {};
    }
  }

  ezUInt32 uiBytesPerElement = 4; // ByteAddress must be multiple of 4
  if (desc.m_ResourceType == ezGALShaderResourceType::StructuredBuffer)
  {
    uiBytesPerElement = bufferDesc.m_uiStructSize;
  }
  else if (desc.m_ResourceType == ezGALShaderResourceType::TexelBuffer)
  {
    desc.m_Format = desc.m_Format == ezGALResourceFormat::Invalid ? bufferDesc.m_Format : desc.m_Format;
    if (desc.m_Format == ezGALResourceFormat::Invalid)
    {
      ezLog::Error("m_Format must be set if m_ResourceType == TexelBuffer");
      return {};
    }
    uiBytesPerElement = ezGALResourceFormat::GetBitsPerElement(desc.m_Format) / 8;
  }
  else if (desc.m_Format != ezGALResourceFormat::Invalid)
  {
    ezLog::Error("m_Format must not be set if m_ResourceType != TexelBuffer");
    return {};
  }

  if ((desc.m_uiByteOffset % uiBytesPerElement) != 0)
  {
    ezLog::Error("m_uiByteOffset {} is not a multiple of the element size {}", desc.m_uiByteOffset, uiBytesPerElement);
    return {};
  }
  if ((desc.m_uiByteCount % uiBytesPerElement) != 0)
  {
    ezLog::Error("m_uiByteCount {} is not a multiple of the element size {}", desc.m_uiByteCount, uiBytesPerElement);
    return {};
  }


  if (desc.m_uiByteOffset >= bufferDesc.m_uiTotalSize)
  {
    ezLog::Error("m_uiByteOffset {} is too big for the buffer of size {}", desc.m_uiByteOffset, bufferDesc.m_uiTotalSize);
    return {};
  }
  if (desc.m_uiByteOffset + desc.m_uiByteCount > bufferDesc.m_uiTotalSize)
  {
    ezLog::Error("m_uiByteOffset {} + m_uiByteCount {} = {} is too big for the buffer of size {}", desc.m_uiByteOffset, desc.m_uiByteCount, desc.m_uiByteOffset + desc.m_uiByteCount, bufferDesc.m_uiTotalSize);
    return {};
  }

  // Hash desc and return potential existing one
  const ezUInt32 uiHash = desc.CalculateHash();
  if (auto hResourceView = TryGetView<ezGALBufferResourceViewHandle, ezGALBufferResourceView>(uiHash, m_BufferResourceViews, pResource->m_ResourceViews); !hResourceView.IsInvalidated())
    return hResourceView;

  ezGALBufferResourceView* pResourceView = CreateResourceViewPlatform(pResource, desc);
  return InsertView<ezGALBufferResourceViewHandle, ezGALBufferResourceView>(uiHash, pResourceView, m_BufferResourceViews, pResource->m_ResourceViews);
}

template <typename Handle, typename View, typename ViewTable, typename CacheTable>
Handle ezGALDevice::TryGetView(ezUInt32 uiHash, ViewTable& viewTable, CacheTable& cacheTable)
{
  Handle hView;
  if (cacheTable.TryGetValue(uiHash, hView))
  {
    View* pView = nullptr;
    EZ_VERIFY(viewTable.TryGetValue(hView, pView), "Implementation error");
    pView->AddRef();
    return hView;
  }
  return {};
}

template <typename Handle, typename View, typename ViewTable, typename CacheTable>
Handle ezGALDevice::InsertView(ezUInt32 uiHash, View* pView, ViewTable& viewTable, CacheTable& cacheTable)
{
  if (pView != nullptr)
  {
    Handle hResourceView(viewTable.Insert(pView));
    cacheTable.Insert(uiHash, hResourceView);

    return hResourceView;
  }

  return Handle();
}

template <typename View, typename Handle, typename ViewTable>
void ezGALDevice::DestroyView(Handle hView, ViewTable& table, ezUInt32 galObjectType)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();
  View* pView = nullptr;
  if (table.TryGetValue(hView, pView))
  {
    if (pView->ReleaseRef() == 0)
    {
      AddDeadObject((GALObjectType::Enum)galObjectType, hView);
    }
  }
  else
  {
    ezLog::Warning("DestroyView called on invalid handle (double free?)");
  }
}

void ezGALDevice::DestroyResourceView(ezGALTextureResourceViewHandle hResourceView)
{
  DestroyView<ezGALTextureResourceView>(hResourceView, m_TextureResourceViews, GALObjectType::TextureResourceView);
}

void ezGALDevice::DestroyResourceView(ezGALBufferResourceViewHandle hResourceView)
{
  DestroyView<ezGALBufferResourceView>(hResourceView, m_BufferResourceViews, GALObjectType::BufferResourceView);
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

  const ezEnum<ezGALTextureType> type = desc.m_OverrideViewType != ezGALTextureType::Invalid ? desc.m_OverrideViewType : pTexture->GetDescription().m_Type;
  if (type != ezGALTextureType::Texture2DArray && type != ezGALTextureType::TextureCubeArray)
  {
    if (desc.m_uiSliceCount != 1)
    {
      EZ_REPORT_FAILURE("m_uiSliceCount must be 1 for non array textures!");
      return ezGALRenderTargetViewHandle();
    }
  }
  if (type == ezGALTextureType::TextureCube || type == ezGALTextureType::TextureCubeArray)
  {
    EZ_REPORT_FAILURE("Render targets cannot be created on cube maps, use 2DArrays instead.");
    return ezGALRenderTargetViewHandle();
  }

  // Hash desc and return potential existing one
  const ezUInt32 uiHash = desc.CalculateHash();
  if (auto hRenderTarget = TryGetView<ezGALRenderTargetViewHandle, ezGALRenderTargetView>(uiHash, m_RenderTargetViews, pTexture->m_RenderTargetViews); !hRenderTarget.IsInvalidated())
    return hRenderTarget;

  ezGALRenderTargetView* pRenderTargetView = CreateRenderTargetViewPlatform(pTexture, desc);
  return InsertView<ezGALRenderTargetViewHandle, ezGALRenderTargetView>(uiHash, pRenderTargetView, m_RenderTargetViews, pTexture->m_RenderTargetViews);
}

void ezGALDevice::DestroyRenderTargetView(ezGALRenderTargetViewHandle hRenderTargetView)
{
  DestroyView<ezGALRenderTargetView>(hRenderTargetView, m_RenderTargetViews, GALObjectType::RenderTargetView);
}

ezGALTextureUnorderedAccessViewHandle ezGALDevice::CreateUnorderedAccessView(const ezGALTextureUnorderedAccessViewCreationDescription& desc)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezGALTexture* pTexture = nullptr;

  if (!desc.m_hTexture.IsInvalidated())
  {
    pTexture = Get<TextureTable, ezGALTexture>(desc.m_hTexture, m_Textures);
  }

  if (pTexture == nullptr)
  {
    ezLog::Error("No valid texture handle given for unordered access view creation!");
    return ezGALTextureUnorderedAccessViewHandle();
  }

  // Some platform independent validation.
  {
    // Is this really platform independent?
    if (pTexture->GetDescription().m_SampleCount != ezGALMSAASampleCount::None)
    {
      ezLog::Error("Can't create unordered access view on textures with multisampling.");
      return ezGALTextureUnorderedAccessViewHandle();
    }
  }

  const ezEnum<ezGALTextureType> type = desc.m_OverrideViewType != ezGALTextureType::Invalid ? desc.m_OverrideViewType : pTexture->GetDescription().m_Type;
  if (type != ezGALTextureType::Texture2DArray && type != ezGALTextureType::TextureCubeArray)
  {
    if (desc.m_uiArraySize != 1)
    {
      EZ_REPORT_FAILURE("m_uiArraySize must be 1 for non array textures!");
      return ezGALTextureUnorderedAccessViewHandle();
    }
  }
  if (type == ezGALTextureType::TextureCube || type == ezGALTextureType::TextureCubeArray)
  {
    EZ_REPORT_FAILURE("UAVs cannot be created on cube maps, use 2DArrays instead.");
    return ezGALTextureUnorderedAccessViewHandle();
  }

  // Hash desc and return potential existing one
  const ezUInt32 uiHash = desc.CalculateHash();
  if (auto hUnorderedAccessView = TryGetView<ezGALTextureUnorderedAccessViewHandle, ezGALTextureUnorderedAccessView>(uiHash, m_TextureUnorderedAccessViews, pTexture->m_UnorderedAccessViews); !hUnorderedAccessView.IsInvalidated())
    return hUnorderedAccessView;

  ezGALTextureUnorderedAccessView* pUnorderedAccessView = CreateUnorderedAccessViewPlatform(pTexture, desc);
  return InsertView<ezGALTextureUnorderedAccessViewHandle, ezGALTextureUnorderedAccessView>(uiHash, pUnorderedAccessView, m_TextureUnorderedAccessViews, pTexture->m_UnorderedAccessViews);
}

ezGALBufferUnorderedAccessViewHandle ezGALDevice::CreateUnorderedAccessView(ezGALBufferUnorderedAccessViewCreationDescription desc)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezGALBuffer* pBuffer = nullptr;

  if (!desc.m_hBuffer.IsInvalidated())
  {
    pBuffer = Get<BufferTable, ezGALBuffer>(desc.m_hBuffer, m_Buffers);
  }

  if (pBuffer == nullptr)
  {
    ezLog::Error("No valid buffer handle given for unordered access view creation!");
    return {};
  }

  const ezGALBufferCreationDescription& bufferDesc = pBuffer->GetDescription();
  const ezBitflags<ezGALBufferUsageFlags> flags = bufferDesc.m_BufferFlags;
  if (!flags.IsSet(ezGALBufferUsageFlags::UnorderedAccess))
  {
    ezLog::Error("UAV creation failed as buffer does not have UnorderedAccess flag set.");
    return {};
  }
  if (desc.m_uiByteCount == 0)
  {
    desc.m_uiByteCount = bufferDesc.m_uiTotalSize - desc.m_uiByteOffset;
  }
  if (desc.m_ResourceType == ezGALShaderResourceType::Unknown)
  {
    const ezBitflags<ezGALBufferUsageFlags> srvFlags = flags & (ezGALBufferUsageFlags::TexelBuffer | ezGALBufferUsageFlags::StructuredBuffer | ezGALBufferUsageFlags::ByteAddressBuffer);
    const ezUInt32 uiSrvFlagsCount = ezMath::CountBits(static_cast<ezUInt32>(srvFlags.GetValue()));
    if (uiSrvFlagsCount != 1)
    {
      ezLog::Error("UAV creation failed as m_ResourceType == Unknown but there are {} UAV flags set, making the UAV ambiguous. Explicitly set the desired resource type to TexelBufferRW, StructuredBufferRW or ByteAddressBufferRW.", uiSrvFlagsCount);
      return {};
    }
    if (srvFlags.IsSet(ezGALBufferUsageFlags::TexelBuffer))
      desc.m_ResourceType = ezGALShaderResourceType::TexelBufferRW;
    else if (srvFlags.IsSet(ezGALBufferUsageFlags::StructuredBuffer))
      desc.m_ResourceType = ezGALShaderResourceType::StructuredBufferRW;
    else if (srvFlags.IsSet(ezGALBufferUsageFlags::ByteAddressBuffer))
      desc.m_ResourceType = ezGALShaderResourceType::ByteAddressBufferRW;
  }
  switch (desc.m_ResourceType)
  {
    case ezGALShaderResourceType::TexelBufferRW:
    case ezGALShaderResourceType::StructuredBufferRW:
    case ezGALShaderResourceType::ByteAddressBufferRW:
      break;
    default:
    {
      ezLog::Error("Resource view creation failed as the resource type of {} is not supported. It must be TexelBufferRW, StructuredBufferRW or ByteAddressBufferRW.", (ezUInt32)desc.m_ResourceType.GetValue());
      return {};
    }
  }

  ezUInt32 uiBytesPerElement = 4; // ByteAddress must be multiple of 4
  if (desc.m_ResourceType == ezGALShaderResourceType::StructuredBufferRW)
  {
    uiBytesPerElement = bufferDesc.m_uiStructSize;
  }
  else if (desc.m_ResourceType == ezGALShaderResourceType::TexelBufferRW)
  {
    desc.m_Format = desc.m_Format == ezGALResourceFormat::Invalid ? bufferDesc.m_Format : desc.m_Format;
    if (desc.m_Format == ezGALResourceFormat::Invalid)
    {
      ezLog::Error("m_Format must be set if m_ResourceType == TexelBuffer");
      return {};
    }
    uiBytesPerElement = ezGALResourceFormat::GetBitsPerElement(desc.m_Format) / 8;
  }
  else if (desc.m_Format != ezGALResourceFormat::Invalid)
  {
    ezLog::Error("m_Format must not be set if m_ResourceType != TexelBuffer");
    return {};
  }

  if ((desc.m_uiByteOffset % uiBytesPerElement) != 0)
  {
    ezLog::Error("m_uiByteOffset {} is not a multiple of the element size {}", desc.m_uiByteOffset, uiBytesPerElement);
    return {};
  }
  if ((desc.m_uiByteCount % uiBytesPerElement) != 0)
  {
    ezLog::Error("m_uiByteCount {} is not a multiple of the element size {}", desc.m_uiByteCount, uiBytesPerElement);
    return {};
  }

  if (desc.m_uiByteOffset >= bufferDesc.m_uiTotalSize)
  {
    ezLog::Error("m_uiByteOffset {} is too big for the buffer of size {}", desc.m_uiByteOffset, bufferDesc.m_uiTotalSize);
    return {};
  }
  if (desc.m_uiByteOffset + desc.m_uiByteCount > bufferDesc.m_uiTotalSize)
  {
    ezLog::Error("m_uiByteOffset {} + m_uiByteCount {} = {} is too big for the buffer of size {}", desc.m_uiByteOffset, desc.m_uiByteCount, desc.m_uiByteOffset + desc.m_uiByteCount, bufferDesc.m_uiTotalSize);
    return {};
  }
  // Hash desc and return potential existing one
  const ezUInt32 uiHash = desc.CalculateHash();
  if (auto hUnorderedAccessView = TryGetView<ezGALBufferUnorderedAccessViewHandle, ezGALBufferUnorderedAccessView>(uiHash, m_BufferUnorderedAccessViews, pBuffer->m_UnorderedAccessViews); !hUnorderedAccessView.IsInvalidated())
    return hUnorderedAccessView;

  ezGALBufferUnorderedAccessView* pUnorderedAccessView = CreateUnorderedAccessViewPlatform(pBuffer, desc);
  return InsertView<ezGALBufferUnorderedAccessViewHandle, ezGALBufferUnorderedAccessView>(uiHash, pUnorderedAccessView, m_BufferUnorderedAccessViews, pBuffer->m_UnorderedAccessViews);
}

void ezGALDevice::DestroyUnorderedAccessView(ezGALTextureUnorderedAccessViewHandle hUnorderedAccessViewHandle)
{
  DestroyView<ezGALTextureUnorderedAccessView>(hUnorderedAccessViewHandle, m_TextureUnorderedAccessViews, GALObjectType::TextureUnorderedAccessView);
}

void ezGALDevice::DestroyUnorderedAccessView(ezGALBufferUnorderedAccessViewHandle hUnorderedAccessViewHandle)
{
  DestroyView<ezGALBufferUnorderedAccessView>(hUnorderedAccessViewHandle, m_BufferUnorderedAccessViews, GALObjectType::BufferUnorderedAccessView);
}

ezGALSwapChainHandle ezGALDevice::CreateSwapChain(const SwapChainFactoryFunction& func)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ///// \todo Platform independent validation
  // if (desc.m_pWindow == nullptr)
  //{
  //   ezLog::Error("The desc for the swap chain creation contained an invalid (nullptr) window handle!");
  //   return ezGALSwapChainHandle();
  // }

  ezGALSwapChain* pSwapChain = func(&m_Allocator);
  // ezGALSwapChainDX11* pSwapChain = EZ_NEW(&m_Allocator, ezGALSwapChainDX11, Description);

  if (!pSwapChain->InitPlatform(this).Succeeded())
  {
    EZ_DELETE(&m_Allocator, pSwapChain);
    return ezGALSwapChainHandle();
  }

  return ezGALSwapChainHandle(m_SwapChains.Insert(pSwapChain));
}

ezResult ezGALDevice::UpdateSwapChain(ezGALSwapChainHandle hSwapChain, ezEnum<ezGALPresentMode> newPresentMode)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezGALSwapChain* pSwapChain = nullptr;

  if (m_SwapChains.TryGetValue(hSwapChain, pSwapChain))
  {
    return pSwapChain->UpdateSwapChain(this, newPresentMode);
  }
  else
  {
    ezLog::Warning("UpdateSwapChain called on invalid handle.");
    return EZ_FAILURE;
  }
}

void ezGALDevice::DestroySwapChain(ezGALSwapChainHandle hSwapChain)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezGALSwapChain* pSwapChain = nullptr;

  if (m_SwapChains.TryGetValue(hSwapChain, pSwapChain))
  {
    AddDeadObject(GALObjectType::SwapChain, hSwapChain);
  }
  else
  {
    ezLog::Warning("DestroySwapChain called on invalid handle (double free?)");
  }
}

ezGALVertexDeclarationHandle ezGALDevice::CreateVertexDeclaration(const ezGALVertexDeclarationCreationDescription& desc)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezInt32 iHighestUsedBinding = -1;
  for (ezUInt32 slot = 0; slot < desc.m_VertexAttributes.GetCount(); ++slot)
  {
    iHighestUsedBinding = ezMath::Max(iHighestUsedBinding, static_cast<ezInt32>(desc.m_VertexAttributes[slot].m_uiVertexBufferSlot));
  }
  if (desc.m_VertexBindings.GetCount() != static_cast<ezUInt32>(iHighestUsedBinding + 1))
  {
    ezLog::Error("Not enough vertex bindings ({}) to support the maximum used vertex buffer index ({}) used by the vertex attributes.", desc.m_VertexBindings.GetCount(), iHighestUsedBinding);
    return {};
  }
  if (desc.m_VertexBindings.GetCount() > EZ_GAL_MAX_VERTEX_BUFFER_COUNT)
  {
    ezLog::Error("Too many vertex bindings ({}), only up to {} are supported.", desc.m_VertexBindings.GetCount(), EZ_GAL_MAX_VERTEX_BUFFER_COUNT);
    return {};
  }

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
      m_uiVertexDeclarations++;
      return hVertexDeclaration;
    }
  }

  ezGALVertexDeclaration* pVertexDeclaration = CreateVertexDeclarationPlatform(desc);

  if (pVertexDeclaration != nullptr)
  {
    pVertexDeclaration->AddRef();
    m_uiVertexDeclarations++;

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
    m_uiVertexDeclarations--;

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

ezEnum<ezGALAsyncResult> ezGALDevice::GetFenceResult(ezGALFenceHandle hFence, ezTime timeout)
{
  if (hFence == 0)
    return ezGALAsyncResult::Expired;

  EZ_ASSERT_DEBUG(timeout.IsZero() || m_pCommandEncoder == nullptr || !m_pCommandEncoder->IsInRenderingScope(), "Waiting for a fence is only allowed outside of a rendering scope");

  ezEnum<ezGALAsyncResult> res = GetFenceResultPlatform(hFence, timeout);

  return res;
}

ezReadbackBufferLock ezGALDevice::LockBuffer(ezGALReadbackBufferHandle hReadbackBuffer, ezArrayPtr<const ezUInt8>& out_memory)
{
  if (hReadbackBuffer.IsInvalidated())
    return {};
  const ezGALReadbackBuffer* pReadbackBuffer = GetReadbackBuffer(hReadbackBuffer);
  if (pReadbackBuffer == nullptr)
    return {};

  return ezReadbackBufferLock(this, pReadbackBuffer, out_memory);
}

ezReadbackTextureLock ezGALDevice::LockTexture(ezGALReadbackTextureHandle hReadbackTexture, const ezArrayPtr<const ezGALTextureSubresource>& subResources, ezDynamicArray<ezGALSystemMemoryDescription>& out_memory)
{
  if (hReadbackTexture.IsInvalidated())
    return {};
  const ezGALReadbackTexture* pReadbackTexture = GetReadbackTexture(hReadbackTexture);
  if (pReadbackTexture == nullptr)
    return {};

  return ezReadbackTextureLock(this, pReadbackTexture, subResources, out_memory);
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

void ezGALDevice::EnqueueFrameSwapChain(ezGALSwapChainHandle hSwapChain)
{
  EZ_ASSERT_DEV(!m_bBeginFrameCalled, "EnqueueFrameSwapChain must be called before or during ezGALDeviceEvent::BeforeBeginFrame");

  ezGALSwapChain* pSwapChain = nullptr;
  if (m_SwapChains.TryGetValue(hSwapChain, pSwapChain))
    m_FrameSwapChains.PushBack(pSwapChain);
}

void ezGALDevice::BeginFrame(const ezUInt64 uiAppFrame)
{
  {
    EZ_PROFILE_SCOPE("BeforeBeginFrame");
    ezGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type = ezGALDeviceEvent::BeforeBeginFrame;
    s_Events.Broadcast(e);
  }

  {
    EZ_GALDEVICE_LOCK_AND_CHECK();
    EZ_ASSERT_DEV(!m_bBeginFrameCalled, "You must call ezGALDevice::EndFrame before you can call ezGALDevice::BeginFrame again");
    m_bBeginFrameCalled = true;
    BeginFramePlatform(m_FrameSwapChains, uiAppFrame);
  }

  for (auto it = m_DynamicBuffers.GetIterator(); it.IsValid(); ++it)
  {
    it.Value()->SwapBuffers();
  }

  {
    ezGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type = ezGALDeviceEvent::AfterBeginFrame;
    s_Events.Broadcast(e);
  }
}

void ezGALDevice::EndFrame()
{
  EZ_PROFILE_SCOPE("ezGALDevice::EndFrame");

  {
    ezGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type = ezGALDeviceEvent::BeforeEndFrame;
    s_Events.Broadcast(e);
  }

  {
    EZ_GALDEVICE_LOCK_AND_CHECK();
    EZ_ASSERT_DEV(m_bBeginFrameCalled, "You must have called ezGALDevice::Begin before you can call ezGALDevice::EndFrame");

    DestroyDeadObjects();

    EndFramePlatform(m_FrameSwapChains);
    m_FrameSwapChains.Clear();
    m_bBeginFrameCalled = false;
    if (m_pCommandEncoder)
      m_pCommandEncoder->ResetStats();
  }

  {
    ezGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type = ezGALDeviceEvent::AfterEndFrame;
    s_Events.Broadcast(e);
  }

  {
    m_EncoderStats.SetStatistics();
    m_EncoderStats.Reset();
    ezStats::SetStat("GalDevice/ShaderReferences", m_uiShaders);
    ezStats::SetStat("GalDevice/VertexDeclarationReferences", m_uiVertexDeclarations);
    ezStats::SetStat("GalDevice/BlendStateReferences", m_uiBlendStates);
    ezStats::SetStat("GalDevice/DepthStencilStateReferences", m_uiDepthStencilStates);
    ezStats::SetStat("GalDevice/RasterizerStateReferences", m_uiRasterizerStates);
    ezStats::SetStat("GalDevice/GraphicsPipelineReferences", m_uiGraphicsPipelines);
    ezStats::SetStat("GalDevice/ComputePipelineReferences", m_uiComputePipelines);
    ezStats::SetStat("GalDevice/SamplerStateReferences", m_uiSamplerStates);

    ezStats::SetStat("GalDevice/Shaders", m_Shaders.GetCount());
    ezStats::SetStat("GalDevice/VertexDeclarations", m_VertexDeclarations.GetCount());
    ezStats::SetStat("GalDevice/BlendStates", m_BlendStates.GetCount());
    ezStats::SetStat("GalDevice/DepthStencilStates", m_DepthStencilStates.GetCount());
    ezStats::SetStat("GalDevice/RasterizerStates", m_RasterizerStates.GetCount());
    ezStats::SetStat("GalDevice/GraphicsPipelines", m_GraphicsPipelines.GetCount());
    ezStats::SetStat("GalDevice/ComputePipelines", m_ComputePipelines.GetCount());
    ezStats::SetStat("GalDevice/SamplerStates", m_SamplerStates.GetCount());

    ezStats::SetStat("GalDevice/Buffers", m_Buffers.GetCount());
    ezStats::SetStat("GalDevice/DynamicBuffers", m_DynamicBuffers.GetCount());
    ezStats::SetStat("GalDevice/Textures", m_Textures.GetCount());
    ezStats::SetStat("GalDevice/ReadbackBuffers", m_ReadbackBuffers.GetCount());
    ezStats::SetStat("GalDevice/ReadbackTextures", m_ReadbackTextures.GetCount());
    ezStats::SetStat("GalDevice/TextureResourceViews", m_TextureResourceViews.GetCount());
    ezStats::SetStat("GalDevice/BufferResourceViews", m_BufferResourceViews.GetCount());
    ezStats::SetStat("GalDevice/RenderTargetViews", m_RenderTargetViews.GetCount());
    ezStats::SetStat("GalDevice/TextureUnorderedAccessViews", m_TextureUnorderedAccessViews.GetCount());
    ezStats::SetStat("GalDevice/BufferUnorderedAccessViews", m_BufferUnorderedAccessViews.GetCount());
    ezStats::SetStat("GalDevice/SwapChains", m_SwapChains.GetCount());
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

void ezGALDevice::Flush()
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  FlushPlatform();
}

void ezGALDevice::WaitIdle()
{
  WaitIdlePlatform();
}

void ezGALDevice::DestroyViews(ezGALTexture* pResource)
{
  EZ_ASSERT_DEBUG(pResource != nullptr, "Must provide valid resource");

  EZ_GALDEVICE_LOCK_AND_CHECK();

  for (auto it = pResource->m_ResourceViews.GetIterator(); it.IsValid(); ++it)
  {
    ezGALTextureResourceViewHandle hResourceView = it.Value();
    ezGALTextureResourceView* pResourceView = m_TextureResourceViews[hResourceView];

    m_TextureResourceViews.Remove(hResourceView);

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
    ezGALTextureUnorderedAccessViewHandle hUnorderedAccessView = it.Value();
    ezGALTextureUnorderedAccessView* pUnorderedAccessView = m_TextureUnorderedAccessViews[hUnorderedAccessView];

    m_TextureUnorderedAccessViews.Remove(hUnorderedAccessView);

    DestroyUnorderedAccessViewPlatform(pUnorderedAccessView);
  }
  pResource->m_UnorderedAccessViews.Clear();
}

void ezGALDevice::DestroyViews(ezGALBuffer* pResource)
{
  EZ_ASSERT_DEBUG(pResource != nullptr, "Must provide valid resource");

  EZ_GALDEVICE_LOCK_AND_CHECK();

  for (auto it = pResource->m_ResourceViews.GetIterator(); it.IsValid(); ++it)
  {
    ezGALBufferResourceViewHandle hResourceView = it.Value();
    ezGALBufferResourceView* pResourceView = m_BufferResourceViews[hResourceView];

    m_BufferResourceViews.Remove(hResourceView);

    DestroyResourceViewPlatform(pResourceView);
  }
  pResource->m_ResourceViews.Clear();

  pResource->m_hDefaultTexelBufferView.Invalidate();
  pResource->m_hDefaultStructuredBufferView.Invalidate();
  pResource->m_hDefaultByteAddressBufferView.Invalidate();

  for (auto it = pResource->m_UnorderedAccessViews.GetIterator(); it.IsValid(); ++it)
  {
    ezGALBufferUnorderedAccessViewHandle hUnorderedAccessView = it.Value();
    ezGALBufferUnorderedAccessView* pUnorderedAccessView = m_BufferUnorderedAccessViews[hUnorderedAccessView];

    m_BufferUnorderedAccessViews.Remove(hUnorderedAccessView);

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

        EZ_VERIFY(m_Shaders.Remove(hShader, &pShader), "");
        m_ShaderTable.Remove(pShader->GetDescription().CalculateHash());

        DestroyShaderPlatform(pShader);

        break;
      }
      case GALObjectType::Buffer:
      {
        ezGALBufferHandle hBuffer(ezGAL::ez18_14Id(deadObject.m_uiHandle));
        ezGALBuffer* pBuffer = nullptr;

        EZ_VERIFY(m_Buffers.Remove(hBuffer, &pBuffer), "");

        DestroyViews(pBuffer);
        DestroyBufferPlatform(pBuffer);

        break;
      }
      case GALObjectType::DynamicBuffer:
      {
        ezGALDynamicBufferHandle hDynamicBuffer(ezGAL::ez18_14Id(deadObject.m_uiHandle));
        ezGALDynamicBuffer* pDynamicBuffer = nullptr;

        EZ_VERIFY(m_DynamicBuffers.Remove(hDynamicBuffer, &pDynamicBuffer), "");

        EZ_DELETE(&m_Allocator, pDynamicBuffer);

        break;
      }
      case GALObjectType::Texture:
      {
        ezGALTextureHandle hTexture(ezGAL::ez18_14Id(deadObject.m_uiHandle));
        ezGALTexture* pTexture = nullptr;

        EZ_VERIFY(m_Textures.Remove(hTexture, &pTexture), "Unexpected invalild texture handle");

        DestroyViews(pTexture);

        switch (pTexture->GetDescription().m_Type)
        {
          case ezGALTextureType::Texture2DShared:
            DestroySharedTexturePlatform(pTexture);
            break;
          default:
            DestroyTexturePlatform(pTexture);
            break;
        }
        break;
      }
      case GALObjectType::ReadbackBuffer:
      {
        ezGALReadbackBufferHandle hBuffer(ezGAL::ez18_14Id(deadObject.m_uiHandle));
        ezGALReadbackBuffer* pBuffer = nullptr;
        EZ_VERIFY(m_ReadbackBuffers.Remove(hBuffer, &pBuffer), "");
        DestroyReadbackBufferPlatform(pBuffer);
        break;
      }
      case GALObjectType::ReadbackTexture:
      {
        ezGALReadbackTextureHandle hTexture(ezGAL::ez18_14Id(deadObject.m_uiHandle));
        ezGALReadbackTexture* pTexture = nullptr;
        EZ_VERIFY(m_ReadbackTextures.Remove(hTexture, &pTexture), "");
        DestroyReadbackTexturePlatform(pTexture);
        break;
      }
      case GALObjectType::TextureResourceView:
      {
        ezGALTextureResourceViewHandle hResourceView(ezGAL::ez18_14Id(deadObject.m_uiHandle));
        ezGALTextureResourceView* pResourceView = nullptr;

        EZ_VERIFY(m_TextureResourceViews.Remove(hResourceView, &pResourceView), "");

        ezGALTexture* pResource = pResourceView->m_pResource;
        EZ_ASSERT_DEBUG(pResource != nullptr, "");

        EZ_VERIFY(pResource->m_ResourceViews.Remove(pResourceView->GetDescription().CalculateHash()), "");
        pResourceView->m_pResource = nullptr;

        DestroyResourceViewPlatform(pResourceView);

        break;
      }
      case GALObjectType::BufferResourceView:
      {
        ezGALBufferResourceViewHandle hResourceView(ezGAL::ez18_14Id(deadObject.m_uiHandle));
        ezGALBufferResourceView* pResourceView = nullptr;

        EZ_VERIFY(m_BufferResourceViews.Remove(hResourceView, &pResourceView), "");

        ezGALBuffer* pResource = pResourceView->m_pResource;
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

        EZ_VERIFY(m_RenderTargetViews.Remove(hRenderTargetView, &pRenderTargetView), "");

        ezGALTexture* pTexture = pRenderTargetView->m_pTexture;
        EZ_ASSERT_DEBUG(pTexture != nullptr, "");
        EZ_VERIFY(pTexture->m_RenderTargetViews.Remove(pRenderTargetView->GetDescription().CalculateHash()), "");
        pRenderTargetView->m_pTexture = nullptr;

        DestroyRenderTargetViewPlatform(pRenderTargetView);

        break;
      }
      case GALObjectType::TextureUnorderedAccessView:
      {
        ezGALTextureUnorderedAccessViewHandle hUnorderedAccessViewHandle(ezGAL::ez18_14Id(deadObject.m_uiHandle));
        ezGALTextureUnorderedAccessView* pUnorderedAccesssView = nullptr;

        EZ_VERIFY(m_TextureUnorderedAccessViews.Remove(hUnorderedAccessViewHandle, &pUnorderedAccesssView), "");

        ezGALTexture* pResource = pUnorderedAccesssView->m_pResource;
        EZ_ASSERT_DEBUG(pResource != nullptr, "");

        EZ_VERIFY(pResource->m_UnorderedAccessViews.Remove(pUnorderedAccesssView->GetDescription().CalculateHash()), "");
        pUnorderedAccesssView->m_pResource = nullptr;

        DestroyUnorderedAccessViewPlatform(pUnorderedAccesssView);
        break;
      }
      case GALObjectType::BufferUnorderedAccessView:
      {
        ezGALBufferUnorderedAccessViewHandle hUnorderedAccessViewHandle(ezGAL::ez18_14Id(deadObject.m_uiHandle));
        ezGALBufferUnorderedAccessView* pUnorderedAccesssView = nullptr;

        EZ_VERIFY(m_BufferUnorderedAccessViews.Remove(hUnorderedAccessViewHandle, &pUnorderedAccesssView), "");

        ezGALBuffer* pResource = pUnorderedAccesssView->m_pResource;
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

        EZ_VERIFY(m_SwapChains.Remove(hSwapChain, &pSwapChain), "");

        if (pSwapChain != nullptr)
        {
          pSwapChain->DeInitPlatform(this).IgnoreResult();
          EZ_DELETE(&m_Allocator, pSwapChain);
        }

        break;
      }
      case GALObjectType::VertexDeclaration:
      {
        ezGALVertexDeclarationHandle hVertexDeclaration(ezGAL::ez18_14Id(deadObject.m_uiHandle));
        ezGALVertexDeclaration* pVertexDeclaration = nullptr;

        EZ_VERIFY(m_VertexDeclarations.Remove(hVertexDeclaration, &pVertexDeclaration), "Unexpected invalid handle");
        m_VertexDeclarationTable.Remove(pVertexDeclaration->GetDescription().CalculateHash());

        DestroyVertexDeclarationPlatform(pVertexDeclaration);

        break;
      }
      case GALObjectType::GraphicsPipeline:
      {
        ezGALGraphicsPipelineHandle hGraphicsPipeline(ezGAL::ez18_14Id(deadObject.m_uiHandle));
        ezGALGraphicsPipeline* pGraphicsPipeline = nullptr;

        EZ_VERIFY(m_GraphicsPipelines.Remove(hGraphicsPipeline, &pGraphicsPipeline), "Unexpected invalid handle");
        m_GraphicsPipelineTable.Remove(pGraphicsPipeline->GetDescription().CalculateHash());

        DestroyGraphicsPipelinePlatform(pGraphicsPipeline);
        break;
      }
      case GALObjectType::ComputePipeline:
      {
        ezGALComputePipelineHandle hComputePipeline(ezGAL::ez18_14Id(deadObject.m_uiHandle));
        ezGALComputePipeline* pComputePipeline = nullptr;

        EZ_VERIFY(m_ComputePipelines.Remove(hComputePipeline, &pComputePipeline), "Unexpected invalid handle");
        m_ComputePipelineTable.Remove(pComputePipeline->GetDescription().CalculateHash());

        DestroyComputePipelinePlatform(pComputePipeline);
        break;
      }
      default:
        EZ_ASSERT_NOT_IMPLEMENTED;
    }
  }

  m_DeadObjects.Clear();
}

const ezGALSwapChain* ezGALDevice::GetSwapChainInternal(ezGALSwapChainHandle hSwapChain, const ezRTTI* pRequestedType) const
{
  const ezGALSwapChain* pSwapChain = GetSwapChain(hSwapChain);
  if (pSwapChain)
  {
    if (!pSwapChain->GetDescription().m_pSwapChainType->IsDerivedFrom(pRequestedType))
      return nullptr;
  }
  return pSwapChain;
}
