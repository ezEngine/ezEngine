#include <RendererFoundation/RendererFoundationPCH.h>

#include <Foundation/Configuration/CVar.h>
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
#include <RendererFoundation/Shader/BindGroup.h>
#include <RendererFoundation/Shader/BindGroupLayout.h>
#include <RendererFoundation/Shader/PipelineLayout.h>
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
      RenderTargetView,
      SwapChain,
      VertexDeclaration,
      BindGroupLayout,
      BindGroup,
      PipelineLayout,
      GraphicsPipeline,
      ComputePipeline,
    };
  };

  static_assert(sizeof(ezGALBlendStateHandle) == sizeof(ezUInt32));
  static_assert(sizeof(ezGALDepthStencilStateHandle) == sizeof(ezUInt32));
  static_assert(sizeof(ezGALRasterizerStateHandle) == sizeof(ezUInt32));
  static_assert(sizeof(ezGALSamplerStateHandle) == sizeof(ezUInt32));
  static_assert(sizeof(ezGALShaderHandle) == sizeof(ezUInt32));
  static_assert(sizeof(ezGALBufferHandle) == sizeof(ezUInt32));
  static_assert(sizeof(ezGALTextureHandle) == sizeof(ezUInt32));
  static_assert(sizeof(ezGALRenderTargetViewHandle) == sizeof(ezUInt32));
  static_assert(sizeof(ezGALSwapChainHandle) == sizeof(ezUInt32));
  static_assert(sizeof(ezGALVertexDeclarationHandle) == sizeof(ezUInt32));
  static_assert(sizeof(ezGALBindGroupLayoutHandle) == sizeof(ezUInt32));
  static_assert(sizeof(ezGALBindGroupHandle) == sizeof(ezUInt32));
  static_assert(sizeof(ezGALPipelineLayoutHandle) == sizeof(ezUInt32));
  static_assert(sizeof(ezGALGraphicsPipelineHandle) == sizeof(ezUInt32));
  static_assert(sizeof(ezGALComputePipelineHandle) == sizeof(ezUInt32));
} // namespace

ezGALDevice* ezGALDevice::s_pDefaultDevice = nullptr;
ezEvent<const ezGALDeviceEvent&, ezMutex> ezGALDevice::s_Events;
ezEvent<const ezGALSwapChain*, ezMutex> ezGALDevice::s_SwapChainUpdatedEvent;

ezGALDevice::ezGALDevice(const ezGALDeviceCreationDescription& desc)
  : m_Allocator("GALDevice", ezFoundation::GetDefaultAllocator())
  , m_AllocatorWrapper(&m_Allocator)
  , m_Description(desc)
{
  m_BindGroupTracker.m_ResourceInvalidatedEvent.AddEventHandler(ezMakeDelegate(&ezGALDevice::OnBindGroupInvalidatedEventHandler, this));
}

ezGALDevice::~ezGALDevice()
{
  m_BindGroupTracker.m_ResourceInvalidatedEvent.RemoveEventHandler(ezMakeDelegate(&ezGALDevice::OnBindGroupInvalidatedEventHandler, this));
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

    if (!m_RenderTargetViews.IsEmpty())
      ezLog::Warning("{0} render target views have not been cleaned up", m_RenderTargetViews.GetCount());

    if (!m_SwapChains.IsEmpty())
      ezLog::Warning("{0} swap chains have not been cleaned up", m_SwapChains.GetCount());

    if (!m_VertexDeclarations.IsEmpty())
      ezLog::Warning("{0} vertex declarations have not been cleaned up", m_VertexDeclarations.GetCount());

    if (!m_BindGroupLayouts.IsEmpty())
      ezLog::Warning("{0} bind group layouts have not been cleaned up", m_BindGroupLayouts.GetCount());

    if (!m_BindGroups.IsEmpty())
      ezLog::Warning("{0} bind groups have not been cleaned up", m_BindGroups.GetCount());

    if (!m_PipelineLayouts.IsEmpty())
      ezLog::Warning("{0} pipeline layouts have not been cleaned up", m_PipelineLayouts.GetCount());

    if (!m_GraphicsPipelines.IsEmpty())
      ezLog::Warning("{0} graphics pipelines have not been cleaned up", m_GraphicsPipelines.GetCount());

    if (!m_ComputePipelines.IsEmpty())
      ezLog::Warning("{0} Compute pipelines have not been cleaned up", m_ComputePipelines.GetCount());
  }
}

ezResult ezGALDevice::Init()
{
  EZ_LOG_BLOCK("ezGALDevice::Init");

  {
    ezGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type = ezGALDeviceEvent::BeforeInit;
    s_Events.Broadcast(e);
  }

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
  EZ_ASSERT_DEBUG(m_uiSamplerStates == 0, "Error in counting deduplicated GAL resources");
  EZ_ASSERT_DEBUG(m_uiBindGroupLayouts == 0, "Error in counting deduplicated GAL resources");
  EZ_ASSERT_DEBUG(m_uiBindGroups == 0, "Error in counting deduplicated GAL resources");
  EZ_ASSERT_DEBUG(m_uiPipelineLayouts == 0, "Error in counting deduplicated GAL resources");
  EZ_ASSERT_DEBUG(m_uiGraphicsPipelines == 0, "Error in counting deduplicated GAL resources");
  EZ_ASSERT_DEBUG(m_uiComputePipelines == 0, "Error in counting deduplicated GAL resources");

  ezResult res = ShutdownPlatform();

  {
    ezGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type = ezGALDeviceEvent::AfterShutdown;
    s_Events.Broadcast(e);
  }

  return res;
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
    EZ_ASSERT_DEV(m_bBeginFrameCalled, "BeginCommands is only allowed to be called within a BeginFrame / EndFrame scope");
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

template <typename Handle, typename Resource, typename Table, typename CacheTable, typename HashType>
Handle ezGALDevice::TryGetHashedResource(HashType uiHash, Table& table, CacheTable& cacheTable, ezUInt32 galObjectType, ezUInt32& ref_uiCounter)
{
  Handle hResource;
  if (cacheTable.TryGetValue(uiHash, hResource))
  {
    Resource* pResource = table[hResource];
    if (pResource->GetRefCount() == 0)
    {
      ReviveDeadObject(galObjectType, hResource);
    }

    pResource->AddRef();
    ref_uiCounter++;
    return hResource;
  }
  return {};
}

template <typename Handle, typename Resource, typename Table, typename CacheTable, typename HashType>
Handle ezGALDevice::InsertHashedResource(HashType uiHash, Resource* pResource, Table& table, CacheTable& cacheTable, ezUInt32& ref_uiCounter)
{
  if (pResource != nullptr)
  {
    EZ_ASSERT_DEBUG(pResource->GetDescription().CalculateHash() == uiHash, "Resource hash doesn't match");

    pResource->AddRef();
    ref_uiCounter++;

    Handle hResource(table.Insert(pResource));
    cacheTable.Insert(uiHash, hResource);

    return hResource;
  }

  return Handle();
}

template <typename Resource, typename Handle, typename Table>
void ezGALDevice::DestroyHashedResource(Handle& inout_hResource, Table& table, ezUInt32 galObjectType, ezUInt32& ref_uiCounter)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  Resource* pResource = nullptr;
  if (table.TryGetValue(inout_hResource, pResource))
  {
    pResource->ReleaseRef();
    ref_uiCounter--;

    if (pResource->GetRefCount() == 0)
    {
      AddDeadObject(galObjectType, inout_hResource);
    }
  }

  inout_hResource.Invalidate();
}

void ezGALDevice::SetTextureQualityMode(ezGALTextureQualitySlot::Enum slot, ezGALTextureQuality::Enum quality)
{
  EZ_ASSERT_DEV(slot < 5, "Invalid ezGALTextureQualitySlot value used: {}", (int)slot);
  m_QualityModes[slot] = quality;
}

void ezGALDevice::AdjustSamplerStateDescription(ezGALSamplerStateCreationDescription& inout_desc)
{
  if (inout_desc.m_useTextureQualitySlot == ezGALTextureQualitySlot::None)
    return;

  switch (m_QualityModes[inout_desc.m_useTextureQualitySlot])
  {
    case ezGALTextureQuality::Nearest:
      inout_desc.m_MinFilter = ezGALTextureFilterMode::Point;
      inout_desc.m_MagFilter = ezGALTextureFilterMode::Point;
      inout_desc.m_MipFilter = ezGALTextureFilterMode::Point;
      inout_desc.m_uiMaxAnisotropy = 1;
      break;

    case ezGALTextureQuality::Bilinear:
      inout_desc.m_MinFilter = ezGALTextureFilterMode::Linear;
      inout_desc.m_MagFilter = ezGALTextureFilterMode::Linear;
      inout_desc.m_MipFilter = ezGALTextureFilterMode::Point;
      inout_desc.m_uiMaxAnisotropy = 1;
      break;

    case ezGALTextureQuality::Trilinear:
      inout_desc.m_MinFilter = ezGALTextureFilterMode::Linear;
      inout_desc.m_MagFilter = ezGALTextureFilterMode::Linear;
      inout_desc.m_MipFilter = ezGALTextureFilterMode::Linear;
      inout_desc.m_uiMaxAnisotropy = 1;
      break;

    case ezGALTextureQuality::Anisotropic2x:
      inout_desc.m_MinFilter = ezGALTextureFilterMode::Anisotropic;
      inout_desc.m_MagFilter = ezGALTextureFilterMode::Anisotropic;
      inout_desc.m_MipFilter = ezGALTextureFilterMode::Anisotropic;
      inout_desc.m_uiMaxAnisotropy = 2;
      break;

    case ezGALTextureQuality::Anisotropic4x:
      inout_desc.m_MinFilter = ezGALTextureFilterMode::Anisotropic;
      inout_desc.m_MagFilter = ezGALTextureFilterMode::Anisotropic;
      inout_desc.m_MipFilter = ezGALTextureFilterMode::Anisotropic;
      inout_desc.m_uiMaxAnisotropy = 4;
      break;

    case ezGALTextureQuality::Anisotropic8x:
      inout_desc.m_MinFilter = ezGALTextureFilterMode::Anisotropic;
      inout_desc.m_MagFilter = ezGALTextureFilterMode::Anisotropic;
      inout_desc.m_MipFilter = ezGALTextureFilterMode::Anisotropic;
      inout_desc.m_uiMaxAnisotropy = 8;
      break;

    case ezGALTextureQuality::Anisotropic16x:
      inout_desc.m_MinFilter = ezGALTextureFilterMode::Anisotropic;
      inout_desc.m_MagFilter = ezGALTextureFilterMode::Anisotropic;
      inout_desc.m_MipFilter = ezGALTextureFilterMode::Anisotropic;
      inout_desc.m_uiMaxAnisotropy = 16;
      break;
  }
}

void ezGALDevice::UpdateTextureQuality()
{
  for (auto it = m_SamplerStates.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value()->GetDescription().m_useTextureQualitySlot != ezGALTextureQualitySlot::None)
    {
      RecreateSamplerStatePlatform(it.Value());
    }
  }
}

ezGALBlendStateHandle ezGALDevice::CreateBlendState(const ezGALBlendStateCreationDescription& desc)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();
  // Hash desc and return potential existing one (including inc. refcount)
  const ezUInt32 uiHash = desc.CalculateHash();

  if (ezGALBlendStateHandle hBlendState = TryGetHashedResource<ezGALBlendStateHandle, ezGALBlendState>(uiHash, m_BlendStates, m_BlendStateTable, GALObjectType::BlendState, m_uiBlendStates); !hBlendState.IsInvalidated())
    return hBlendState;

  ezGALBlendState* pBlendState = CreateBlendStatePlatform(desc);
  return InsertHashedResource<ezGALBlendStateHandle>(uiHash, pBlendState, m_BlendStates, m_BlendStateTable, m_uiBlendStates);
}

void ezGALDevice::DestroyBlendState(ezGALBlendStateHandle& inout_hBlendState)
{
  DestroyHashedResource<ezGALBlendState>(inout_hBlendState, m_BlendStates, GALObjectType::BlendState, m_uiBlendStates);
}

ezGALDepthStencilStateHandle ezGALDevice::CreateDepthStencilState(const ezGALDepthStencilStateCreationDescription& desc)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();
  // Hash desc and return potential existing one (including inc. refcount)
  const ezUInt32 uiHash = desc.CalculateHash();

  if (ezGALDepthStencilStateHandle hDepthStencilState = TryGetHashedResource<ezGALDepthStencilStateHandle, ezGALDepthStencilState>(uiHash, m_DepthStencilStates, m_DepthStencilStateTable, GALObjectType::DepthStencilState, m_uiDepthStencilStates); !hDepthStencilState.IsInvalidated())
    return hDepthStencilState;

  ezGALDepthStencilState* pDepthStencilState = CreateDepthStencilStatePlatform(desc);
  return InsertHashedResource<ezGALDepthStencilStateHandle>(uiHash, pDepthStencilState, m_DepthStencilStates, m_DepthStencilStateTable, m_uiDepthStencilStates);
}

void ezGALDevice::DestroyDepthStencilState(ezGALDepthStencilStateHandle& inout_hDepthStencilState)
{
  DestroyHashedResource<ezGALDepthStencilState>(inout_hDepthStencilState, m_DepthStencilStates, GALObjectType::DepthStencilState, m_uiDepthStencilStates);
}

ezGALRasterizerStateHandle ezGALDevice::CreateRasterizerState(const ezGALRasterizerStateCreationDescription& desc)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();
  // Hash desc and return potential existing one (including inc. refcount)
  const ezUInt32 uiHash = desc.CalculateHash();

  if (ezGALRasterizerStateHandle hRasterizerState = TryGetHashedResource<ezGALRasterizerStateHandle, ezGALRasterizerState>(uiHash, m_RasterizerStates, m_RasterizerStateTable, GALObjectType::RasterizerState, m_uiRasterizerStates); !hRasterizerState.IsInvalidated())
    return hRasterizerState;

  ezGALRasterizerState* pRasterizerState = CreateRasterizerStatePlatform(desc);
  return InsertHashedResource<ezGALRasterizerStateHandle>(uiHash, pRasterizerState, m_RasterizerStates, m_RasterizerStateTable, m_uiRasterizerStates);
}

void ezGALDevice::DestroyRasterizerState(ezGALRasterizerStateHandle& inout_hRasterizerState)
{
  DestroyHashedResource<ezGALRasterizerState>(inout_hRasterizerState, m_RasterizerStates, GALObjectType::RasterizerState, m_uiRasterizerStates);
}

ezGALSamplerStateHandle ezGALDevice::CreateSamplerState(const ezGALSamplerStateCreationDescription& desc)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();
  // Hash desc and return potential existing one (including inc. refcount)
  const ezUInt32 uiHash = desc.CalculateHash();

  if (ezGALSamplerStateHandle hSamplerState = TryGetHashedResource<ezGALSamplerStateHandle, ezGALSamplerState>(uiHash, m_SamplerStates, m_SamplerStateTable, GALObjectType::SamplerState, m_uiSamplerStates); !hSamplerState.IsInvalidated())
    return hSamplerState;

  ezGALSamplerState* pSamplerState = CreateSamplerStatePlatform(desc);
  return InsertHashedResource<ezGALSamplerStateHandle>(uiHash, pSamplerState, m_SamplerStates, m_SamplerStateTable, m_uiSamplerStates);
}

void ezGALDevice::DestroySamplerState(ezGALSamplerStateHandle& inout_hSamplerState)
{
  DestroyHashedResource<ezGALSamplerState>(inout_hSamplerState, m_SamplerStates, GALObjectType::SamplerState, m_uiSamplerStates);
}

ezGALBindGroupLayoutHandle ezGALDevice::CreateBindGroupLayout(const ezGALBindGroupLayoutCreationDescription& desc)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();
  // Hash desc and return potential existing one (including inc. refcount)
  const ezUInt32 uiHash = desc.CalculateHash();

  if (ezGALBindGroupLayoutHandle hBindGroupLayout = TryGetHashedResource<ezGALBindGroupLayoutHandle, ezGALBindGroupLayout>(uiHash, m_BindGroupLayouts, m_BindGroupLayoutTable, GALObjectType::BindGroupLayout, m_uiBindGroupLayouts); !hBindGroupLayout.IsInvalidated())
    return hBindGroupLayout;

  ezGALBindGroupLayout* pBindGroupLayout = CreateBindGroupLayoutPlatform(desc);
  return InsertHashedResource<ezGALBindGroupLayoutHandle>(uiHash, pBindGroupLayout, m_BindGroupLayouts, m_BindGroupLayoutTable, m_uiBindGroupLayouts);
}

void ezGALDevice::DestroyBindGroupLayout(ezGALBindGroupLayoutHandle& inout_hBindGroupLayout)
{
  DestroyHashedResource<ezGALBindGroupLayout>(inout_hBindGroupLayout, m_BindGroupLayouts, GALObjectType::BindGroupLayout, m_uiBindGroupLayouts);
}

ezGALBindGroupHandle ezGALDevice::CreateBindGroup(const ezGALBindGroupCreationDescription& desc)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();
  // Hash desc and return potential existing one (including inc. refcount)
  const ezUInt64 uiHash = desc.CalculateHash();

  if (ezGALBindGroupHandle hBindGroup = TryGetHashedResource<ezGALBindGroupHandle, ezGALBindGroup>(uiHash, m_BindGroups, m_BindGroupTable, GALObjectType::BindGroup, m_uiBindGroups); !hBindGroup.IsInvalidated())
    return hBindGroup;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  {
    EZ_LOG_BLOCK("CreateBindGroup");
    desc.AssertValidDescription(*this);
  }
#endif

  ezGALBindGroup* pBindGroup = CreateBindGroupPlatform(desc);

  const ezGALBindGroupCreationDescription& desc2 = pBindGroup->GetDescription();
  EZ_ASSERT_DEBUG(desc.m_hBindGroupLayout == desc2.m_hBindGroupLayout, "");
  EZ_ASSERT_DEBUG(desc.m_BindGroupItems.GetCount() == desc2.m_BindGroupItems.GetCount(), "");

  EZ_ASSERT_DEBUG(desc.CalculateHash() == desc2.CalculateHash(), "");
  for (ezUInt32 i = 0; i < desc2.m_BindGroupItems.GetCount(); ++i)
  {
    EZ_ASSERT_DEBUG(desc.m_BindGroupItems[i] == desc2.m_BindGroupItems[i], "");
  }


  {
    ezSet<const ezGALResourceBase*> dependencies;
    const ezGALBindGroupLayout* pLayout = GetBindGroupLayout(desc.m_hBindGroupLayout);
    ezArrayPtr<const ezShaderResourceBinding> bindings = pLayout->GetDescription().m_ResourceBindings;
    ezArrayPtr<const ezGALBindGroupItem> items = desc.m_BindGroupItems;
    const ezUInt32 uiBindings = bindings.GetCount();
    for (ezUInt32 i = 0; i < uiBindings; ++i)
    {
      const ezShaderResourceBinding& binding = bindings[i];
      const ezGALBindGroupItem& item = items[i];
      switch (binding.m_ResourceType)
      {
        case ezGALShaderResourceType::Sampler:
        {
          const ezGALSamplerState* pSampler = GetSamplerState(item.m_Sampler.m_hSampler);
          dependencies.Insert(pSampler);
        }
        break;
        case ezGALShaderResourceType::ConstantBuffer:
        case ezGALShaderResourceType::TexelBuffer:
        case ezGALShaderResourceType::StructuredBuffer:
        case ezGALShaderResourceType::ByteAddressBuffer:
        case ezGALShaderResourceType::TexelBufferRW:
        case ezGALShaderResourceType::StructuredBufferRW:
        case ezGALShaderResourceType::ByteAddressBufferRW:
        {
          const ezGALBuffer* pBuffer = GetBuffer(item.m_Buffer.m_hBuffer);
          dependencies.Insert(pBuffer);
        }
        break;
        case ezGALShaderResourceType::TextureAndSampler:
        {
          const ezGALSamplerState* pSampler = GetSamplerState(item.m_Texture.m_hSampler);
          dependencies.Insert(pSampler);
        }
          [[fallthrough]];
        case ezGALShaderResourceType::Texture:
        case ezGALShaderResourceType::TextureRW:
        {
          const ezGALTexture* pTexture = GetTexture(item.m_Texture.m_hTexture);
          dependencies.Insert(pTexture);
        }
        break;

        default:
          EZ_REPORT_FAILURE("Unsupported shader resource type in bind group");
          break;
      }
    }

    m_BindGroupTracker.AddResource(pBindGroup, dependencies);
  }


  return InsertHashedResource<ezGALBindGroupHandle>(uiHash, pBindGroup, m_BindGroups, m_BindGroupTable, m_uiBindGroups);
}

void ezGALDevice::DestroyBindGroup(ezGALBindGroupHandle& inout_hBindGroup)
{
  DestroyHashedResource<ezGALBindGroup>(inout_hBindGroup, m_BindGroups, GALObjectType::BindGroup, m_uiBindGroups);
}

ezGALPipelineLayoutHandle ezGALDevice::CreatePipelineLayout(const ezGALPipelineLayoutCreationDescription& desc)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();
  // Hash desc and return potential existing one (including inc. refcount)
  const ezUInt32 uiHash = desc.CalculateHash();

  if (ezGALPipelineLayoutHandle hPipelineLayout = TryGetHashedResource<ezGALPipelineLayoutHandle, ezGALPipelineLayout>(uiHash, m_PipelineLayouts, m_PipelineLayoutTable, GALObjectType::PipelineLayout, m_uiPipelineLayouts); !hPipelineLayout.IsInvalidated())
    return hPipelineLayout;

  ezGALPipelineLayout* pPipelineLayout = CreatePipelineLayoutPlatform(desc);
  return InsertHashedResource<ezGALPipelineLayoutHandle>(uiHash, pPipelineLayout, m_PipelineLayouts, m_PipelineLayoutTable, m_uiPipelineLayouts);
}

void ezGALDevice::DestroyPipelineLayout(ezGALPipelineLayoutHandle& inout_hPipelineLayout)
{
  DestroyHashedResource<ezGALPipelineLayout>(inout_hPipelineLayout, m_PipelineLayouts, GALObjectType::PipelineLayout, m_uiPipelineLayouts);
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

void ezGALDevice::DestroyGraphicsPipeline(ezGALGraphicsPipelineHandle& inout_hGraphicsPipeline)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezGALGraphicsPipeline* pGraphicsPipeline = nullptr;
  if (m_GraphicsPipelines.TryGetValue(inout_hGraphicsPipeline, pGraphicsPipeline))
  {
    pGraphicsPipeline->ReleaseRef();
    m_uiGraphicsPipelines--;

    if (pGraphicsPipeline->GetRefCount() == 0)
    {
      AddDeadObject(GALObjectType::GraphicsPipeline, inout_hGraphicsPipeline);
      const ezGALGraphicsPipelineCreationDescription& desc = pGraphicsPipeline->GetDescription();

      auto hShader = desc.m_hShader;
      auto hRasterizerState = desc.m_hRasterizerState;
      auto hBlendState = desc.m_hBlendState;
      auto hDepthStencilState = desc.m_hDepthStencilState;
      auto hVertexDeclaration = desc.m_hVertexDeclaration;

      DestroyShader(hShader);
      DestroyRasterizerState(hRasterizerState);
      DestroyBlendState(hBlendState);
      DestroyDepthStencilState(hDepthStencilState);
      DestroyVertexDeclaration(hVertexDeclaration);
    }
  }

  inout_hGraphicsPipeline.Invalidate();
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

void ezGALDevice::DestroyComputePipeline(ezGALComputePipelineHandle& inout_hComputePipeline)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezGALComputePipeline* pComputePipeline = nullptr;
  if (m_ComputePipelines.TryGetValue(inout_hComputePipeline, pComputePipeline))
  {
    pComputePipeline->ReleaseRef();
    m_uiComputePipelines--;

    if (pComputePipeline->GetRefCount() == 0)
    {
      AddDeadObject(GALObjectType::ComputePipeline, inout_hComputePipeline);
      const ezGALComputePipelineCreationDescription& desc = pComputePipeline->GetDescription();

      auto hShader = desc.m_hShader;
      DestroyShader(hShader);
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

void ezGALDevice::DestroyShader(ezGALShaderHandle& inout_hShader)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezGALShader* pShader = nullptr;
  if (m_Shaders.TryGetValue(inout_hShader, pShader))
  {
    pShader->ReleaseRef();
    m_uiShaders--;

    if (pShader->GetRefCount() == 0)
    {
      AddDeadObject(GALObjectType::Shader, inout_hShader);
    }
  }

  inout_hShader.Invalidate();
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

  ezGALBufferHandle hBuffer(m_Buffers.Insert(pBuffer));
  return hBuffer;
}

void ezGALDevice::DestroyBuffer(ezGALBufferHandle& inout_hBuffer)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezGALBuffer* pBuffer = nullptr;
  if (m_Buffers.TryGetValue(inout_hBuffer, pBuffer))
  {
    AddDeadObject(GALObjectType::Buffer, inout_hBuffer);
  }

  inout_hBuffer.Invalidate();
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
  }

  inout_hBuffer.Invalidate();
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
  if (desc.Validate(this, initialData).Failed())
  {
    return {};
  }

  if (desc.m_ResourceAccess.IsImmutable())
  {
    if (desc.m_TextureFlags.IsAnySet(ezGALTextureUsageFlags::RenderTarget | ezGALTextureUsageFlags::UnorderedAccess))
    {
      ezLog::Error("m_uiStructSize must be != 0 if StructuredBuffer, IndexBuffer or VertexBuffer flag is set.");
      return {};
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

    // Create default render target view
    if (desc.m_TextureFlags.IsSet(ezGALTextureUsageFlags::RenderTarget))
    {
      ezGALRenderTargetViewCreationDescription rtDesc;
      rtDesc.m_hTexture = hTexture;
      rtDesc.m_uiFirstSlice = 0;
      rtDesc.m_uiSliceCount = desc.m_uiArraySize;
      if (desc.m_Type == ezGALTextureType::TextureCube || desc.m_Type == ezGALTextureType::TextureCubeArray)
      {
        rtDesc.m_OverrideViewType = ezGALTextureType::Texture2DArray;
      }

      pTexture->m_hDefaultRenderTargetView = GetRenderTargetView(rtDesc);
    }

    return hTexture;
  }

  return ezGALTextureHandle();
}

void ezGALDevice::DestroyTexture(ezGALTextureHandle& inout_hTexture)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezGALTexture* pTexture = nullptr;
  if (m_Textures.TryGetValue(inout_hTexture, pTexture))
  {
    AddDeadObject(GALObjectType::Texture, inout_hTexture);
  }

  inout_hTexture.Invalidate();
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
  const ezUInt32 uiSliceCount = parentDesc.m_Type == ezGALTextureType::TextureCube || parentDesc.m_Type == ezGALTextureType::TextureCubeArray ? parentDesc.m_uiArraySize * 6 : parentDesc.m_uiArraySize;
  EZ_ASSERT_DEV(uiSlice < uiSliceCount, "Proxy slice {} is out of bounds. Parent texture only has {} slices", uiSlice, uiSliceCount);
  ezGALProxyTexture* pProxyTexture = EZ_NEW(&m_Allocator, ezGALProxyTexture, hParentTexture, *pParentTexture, (ezUInt16)uiSlice);
  ezGALTextureHandle hProxyTexture(m_Textures.Insert(pProxyTexture));

  // Create default render target view
  // if (desc.m_TextureFlags.IsSet(ezGALTextureUsageFlags::RenderTarget))
  {
    ezGALRenderTargetViewCreationDescription rtDesc;
    rtDesc.m_hTexture = hProxyTexture;
    rtDesc.m_uiFirstSlice = uiSlice;
    rtDesc.m_uiSliceCount = 1;

    pProxyTexture->m_hDefaultRenderTargetView = GetRenderTargetView(rtDesc);
  }

  return hProxyTexture;
}

void ezGALDevice::DestroyProxyTexture(ezGALTextureHandle& inout_hProxyTexture)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezGALTexture* pTexture = nullptr;
  if (m_Textures.TryGetValue(inout_hProxyTexture, pTexture))
  {
    EZ_ASSERT_DEV(pTexture->GetDescription().m_Type == ezGALTextureType::Texture2DProxy, "Given texture is not a proxy texture");

    AddDeadObject(GALObjectType::Texture, inout_hProxyTexture);
  }

  inout_hProxyTexture.Invalidate();
}

ezGALTextureHandle ezGALDevice::CreateSharedTexture(const ezGALTextureCreationDescription& desc, ezArrayPtr<ezGALSystemMemoryDescription> initialData)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  /// \todo Platform independent validation (desc width & height < platform maximum, format, etc.)

  if (desc.m_ResourceAccess.IsImmutable() && (initialData.IsEmpty() || initialData.GetCount() < desc.m_uiMipLevelCount) &&
      !desc.m_TextureFlags.IsSet(ezGALTextureUsageFlags::RenderTarget))
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

void ezGALDevice::DestroySharedTexture(ezGALTextureHandle& inout_hSharedTexture)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezGALTexture* pTexture = nullptr;
  if (m_Textures.TryGetValue(inout_hSharedTexture, pTexture))
  {
    EZ_ASSERT_DEV(pTexture->GetDescription().m_Type == ezGALTextureType::Texture2DShared, "Given texture is not a shared texture texture");

    AddDeadObject(GALObjectType::Texture, inout_hSharedTexture);
  }

  inout_hSharedTexture.Invalidate();
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

void ezGALDevice::DestroyReadbackTexture(ezGALReadbackTextureHandle& inout_hTexture)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezGALReadbackTexture* pTexture = nullptr;
  if (m_ReadbackTextures.TryGetValue(inout_hTexture, pTexture))
  {
    AddDeadObject(GALObjectType::ReadbackTexture, inout_hTexture);
  }

  inout_hTexture.Invalidate();
}

ezGALReadbackBufferHandle ezGALDevice::CreateReadbackBuffer(const ezGALBufferCreationDescription& description)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezGALReadbackBuffer* pReadbackBuffer = CreateReadbackBufferPlatform(description);
  ezGALReadbackBufferHandle hBuffer(m_ReadbackBuffers.Insert(pReadbackBuffer));
  return hBuffer;
}

void ezGALDevice::DestroyReadbackBuffer(ezGALReadbackBufferHandle& inout_hBuffer)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezGALReadbackBuffer* pBuffer = nullptr;
  if (m_ReadbackBuffers.TryGetValue(inout_hBuffer, pBuffer))
  {
    AddDeadObject(GALObjectType::ReadbackBuffer, inout_hBuffer);
  }

  inout_hBuffer.Invalidate();
}

void ezGALDevice::UpdateBufferForNextFrame(ezGALBufferHandle hBuffer, ezConstByteArrayPtr sourceData, ezUInt32 uiDestOffset /*= 0*/)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  if (const ezGALBuffer* pBuffer = GetBuffer(hBuffer))
  {
    EZ_ASSERT_DEBUG(!pBuffer->GetDescription().m_ResourceAccess.m_bImmutable, "Can't update immutable buffers");
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
    EZ_ASSERT_DEBUG(!desc.m_ResourceAccess.m_bImmutable, "Can't update immutable textures");
    const ezVec3U32 vMipSize = pTexture->GetMipMapSize(destinationSubResource.m_uiMipLevel);
    const bool bDestBoxIsValid = destinationBox.IsValid() && destinationBox.GetExtents().IsZero() == false;
    if (bDestBoxIsValid && (destinationBox.m_vMax.x > vMipSize.x || destinationBox.m_vMax.y > vMipSize.y || destinationBox.m_vMax.z > vMipSize.z))
    {
      ezLog::Error("Trying to update texture outside of its bounds!");
      return;
    }

    const ezUInt32 uiWidth = bDestBoxIsValid ? ezMath::Max(destinationBox.m_vMax.x - destinationBox.m_vMin.x, 1u) : vMipSize.x;
    const ezUInt32 uiHeight = bDestBoxIsValid ? ezMath::Max(destinationBox.m_vMax.y - destinationBox.m_vMin.y, 1u) : vMipSize.y;
    const ezUInt32 uiDepth = bDestBoxIsValid ? ezMath::Max(destinationBox.m_vMax.z - destinationBox.m_vMin.z, 1u) : vMipSize.z;

    const bool bBlockCompressed = ezGALResourceFormat::IsBlockCompressed(desc.m_Format);
    const ezUInt32 uiBlockWidth = bBlockCompressed ? 4u : 1u;
    const ezUInt32 uiBlockHeight = bBlockCompressed ? 4u : 1u;
    const ezUInt32 uiBlockDepth = 1u;
    const ezUInt32 uiBlockCountX = (uiWidth + uiBlockWidth - 1) / uiBlockWidth;
    const ezUInt32 uiBlockCountY = (uiHeight + uiBlockHeight - 1) / uiBlockHeight;
    const ezUInt32 uiBlockCountZ = (uiDepth + uiBlockDepth - 1) / uiBlockDepth;
    const ezUInt32 uiBytesPerBlock = ezGALResourceFormat::GetBitsPerElement(desc.m_Format) * uiBlockWidth * uiBlockHeight * uiBlockDepth / 8;

    const ezUInt32 uiTightRowPitch = uiBlockCountX * uiBytesPerBlock;
    if (sourceData.m_uiRowPitch < uiTightRowPitch)
    {
      ezLog::Error("Invalid row pitch. Expected at least {0} got {1}", uiTightRowPitch, sourceData.m_uiRowPitch);
      return;
    }

    const ezUInt32 uiMinSlicePitch = sourceData.m_uiRowPitch * uiBlockCountY;
    if (sourceData.m_uiSlicePitch != 0 && sourceData.m_uiSlicePitch < uiMinSlicePitch)
    {
      ezLog::Error("Invalid slice pitch. Expected at least {0} got {1}", uiMinSlicePitch, sourceData.m_uiSlicePitch);
      return;
    }

    const ezUInt32 uiSlicePitch = sourceData.m_uiSlicePitch != 0 ? sourceData.m_uiSlicePitch : uiMinSlicePitch;

    if (sourceData.m_pData.GetCount() < uiSlicePitch * uiBlockCountZ)
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
      finalDestBox.m_vMax = vMipSize;
    }

    UpdateTextureForNextFramePlatform(pTexture, finalSourceData, destinationSubResource, finalDestBox);
  }
  else
  {
    ezLog::Error("No valid texture handle given to update!");
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

ezGALRenderTargetViewHandle ezGALDevice::GetRenderTargetView(const ezGALRenderTargetViewCreationDescription& desc)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();
  ezGALRenderTargetViewCreationDescription rtDesc = desc;

  ezGALTexture* pTexture = nullptr;

  if (!rtDesc.m_hTexture.IsInvalidated())
    pTexture = Get<TextureTable, ezGALTexture>(rtDesc.m_hTexture, m_Textures);

  if (pTexture == nullptr)
  {
    ezLog::Error("No valid texture handle given for render target view creation!");
    return ezGALRenderTargetViewHandle();
  }

  const ezEnum<ezGALTextureType> type = rtDesc.m_OverrideViewType != ezGALTextureType::Invalid ? rtDesc.m_OverrideViewType : pTexture->GetDescription().m_Type;
  if (type != ezGALTextureType::Texture2DArray && type != ezGALTextureType::TextureCubeArray)
  {
    if (rtDesc.m_uiSliceCount != 1)
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
  if (type == ezGALTextureType::Texture2DProxy)
  {
    ezGALProxyTexture* pProxyTexture = static_cast<ezGALProxyTexture*>(pTexture);
    rtDesc.m_uiFirstSlice = pProxyTexture->m_uiSlice;
  }

  // Hash desc and return potential existing one
  const ezUInt32 uiHash = rtDesc.CalculateHash();
  {
    ezGALRenderTargetViewHandle hRenderTarget;
    if (pTexture->m_RenderTargetViews.TryGetValue(uiHash, hRenderTarget))
      return hRenderTarget;
  }

  ezGALRenderTargetView* pRenderTargetView = CreateRenderTargetViewPlatform(pTexture, rtDesc);
  if (pRenderTargetView != nullptr)
  {
    ezGALRenderTargetViewHandle hView(m_RenderTargetViews.Insert(pRenderTargetView));
    pTexture->m_RenderTargetViews.Insert(uiHash, hView);
    return hView;
  }

  return ezGALRenderTargetViewHandle();
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

  Flush();
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

void ezGALDevice::DestroySwapChain(ezGALSwapChainHandle& inout_hSwapChain)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezGALSwapChain* pSwapChain = nullptr;
  if (m_SwapChains.TryGetValue(inout_hSwapChain, pSwapChain))
  {
    AddDeadObject(GALObjectType::SwapChain, inout_hSwapChain);
  }

  inout_hSwapChain.Invalidate();
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

void ezGALDevice::DestroyVertexDeclaration(ezGALVertexDeclarationHandle& inout_hVertexDeclaration)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ezGALVertexDeclaration* pVertexDeclaration = nullptr;
  if (m_VertexDeclarations.TryGetValue(inout_hVertexDeclaration, pVertexDeclaration))
  {
    pVertexDeclaration->ReleaseRef();
    m_uiVertexDeclarations--;

    if (pVertexDeclaration->GetRefCount() == 0)
    {
      AddDeadObject(GALObjectType::VertexDeclaration, inout_hVertexDeclaration);
    }
  }

  inout_hVertexDeclaration.Invalidate();
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

ezGALTextureHandle ezGALDevice::GetBackBufferTextureFromSwapChain(ezGALSwapChainHandle hSwapChain) const
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

void ezGALDevice::GetAllSwapChains(ezDynamicArray<ezGALSwapChainHandle>& out_swapChains) const
{
  out_swapChains.Reserve(m_SwapChains.GetCount());
  for (auto it = m_SwapChains.GetIterator(); it.IsValid(); ++it)
  {
    out_swapChains.PushBack(ezGALSwapChainHandle(it.Id()));
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
    EZ_PROFILE_SCOPE("ezGALDevice::BeforeEndFrame");

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
    ezStats::SetStat("GalDevice/SamplerStateReferences", m_uiSamplerStates);
    ezStats::SetStat("GalDevice/BindGroupLayoutReferences", m_uiBindGroupLayouts);
    ezStats::SetStat("GalDevice/BindGroupReferences", m_uiBindGroups);
    ezStats::SetStat("GalDevice/PipelineLayoutReferences", m_uiPipelineLayouts);
    ezStats::SetStat("GalDevice/GraphicsPipelineReferences", m_uiGraphicsPipelines);
    ezStats::SetStat("GalDevice/ComputePipelineReferences", m_uiComputePipelines);

    ezStats::SetStat("GalDevice/Shaders", m_Shaders.GetCount());
    ezStats::SetStat("GalDevice/VertexDeclarations", m_VertexDeclarations.GetCount());
    ezStats::SetStat("GalDevice/BlendStates", m_BlendStates.GetCount());
    ezStats::SetStat("GalDevice/DepthStencilStates", m_DepthStencilStates.GetCount());
    ezStats::SetStat("GalDevice/RasterizerStates", m_RasterizerStates.GetCount());
    ezStats::SetStat("GalDevice/SamplerStates", m_SamplerStates.GetCount());
    ezStats::SetStat("GalDevice/BindGroupLayouts", m_BindGroupLayouts.GetCount());
    ezStats::SetStat("GalDevice/BindGroups", m_BindGroups.GetCount());
    ezStats::SetStat("GalDevice/PipelineLayouts", m_PipelineLayouts.GetCount());
    ezStats::SetStat("GalDevice/GraphicsPipelines", m_GraphicsPipelines.GetCount());
    ezStats::SetStat("GalDevice/ComputePipelines", m_ComputePipelines.GetCount());

    ezStats::SetStat("GalDevice/Buffers", m_Buffers.GetCount());
    ezStats::SetStat("GalDevice/DynamicBuffers", m_DynamicBuffers.GetCount());
    ezStats::SetStat("GalDevice/Textures", m_Textures.GetCount());
    ezStats::SetStat("GalDevice/ReadbackBuffers", m_ReadbackBuffers.GetCount());
    ezStats::SetStat("GalDevice/ReadbackTextures", m_ReadbackTextures.GetCount());
    ezStats::SetStat("GalDevice/RenderTargetViews", m_RenderTargetViews.GetCount());
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

        m_BindGroupTracker.DependencyDestroyed(pSamplerState);
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
        m_BindGroupTracker.DependencyDestroyed(pBuffer);
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

        m_BindGroupTracker.DependencyDestroyed(pTexture);
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
      case GALObjectType::BindGroupLayout:
      {
        ezGALBindGroupLayoutHandle hBindGroupLayout(ezGAL::ez18_14Id(deadObject.m_uiHandle));
        ezGALBindGroupLayout* pBindGroupLayout = nullptr;

        EZ_VERIFY(m_BindGroupLayouts.Remove(hBindGroupLayout, &pBindGroupLayout), "Unexpected invalid handle");
        m_BindGroupLayoutTable.Remove(pBindGroupLayout->GetDescription().CalculateHash());

        DestroyBindGroupLayoutPlatform(pBindGroupLayout);
        break;
      }
      case GALObjectType::BindGroup:
      {
        ezGALBindGroupHandle hBindGroup(ezGAL::ez18_14Id(deadObject.m_uiHandle));
        ezGALBindGroup* pBindGroup = nullptr;

        EZ_VERIFY(m_BindGroups.Remove(hBindGroup, &pBindGroup), "Unexpected invalid handle");
        m_BindGroupTable.Remove(pBindGroup->GetDescription().CalculateHash());

        m_BindGroupTracker.RemoveResource(pBindGroup);
        DestroyBindGroupPlatform(pBindGroup);
        break;
      }
      case GALObjectType::PipelineLayout:
      {
        ezGALPipelineLayoutHandle hPipelineLayout(ezGAL::ez18_14Id(deadObject.m_uiHandle));
        ezGALPipelineLayout* pPipelineLayout = nullptr;

        EZ_VERIFY(m_PipelineLayouts.Remove(hPipelineLayout, &pPipelineLayout), "Unexpected invalid handle");
        m_PipelineLayoutTable.Remove(pPipelineLayout->GetDescription().CalculateHash());

        DestroyPipelineLayoutPlatform(pPipelineLayout);
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

void ezGALDevice::OnBindGroupInvalidatedEventHandler(ezGALBindGroup* pBindGroup)
{
  EZ_GALDEVICE_LOCK_AND_CHECK();
  pBindGroup->Invalidate(this);
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
