#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Types/ScopeExit.h>
#include <RendererCore/Material/MaterialManager.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/DynamicMeshBufferResource.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Shader/ShaderPermutationResource.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/Texture3DResource.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/ImmutableSamplers.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>
#include <RendererFoundation/State/PipelineCache.h>

ezRenderContext* ezRenderContext::s_pDefaultInstance = nullptr;
ezGALCommandEncoder* ezRenderContext::s_pCommandEncoder = nullptr;
ezHybridArray<ezRenderContext*, 4> ezRenderContext::s_Instances;

ezMap<ezRenderContext::ShaderVertexDecl, ezGALVertexDeclarationHandle> ezRenderContext::s_GALVertexDeclarations;

ezMutex ezRenderContext::s_ConstantBufferStorageMutex;
ezIdTable<ezConstantBufferStorageId, ezConstantBufferStorageBase*> ezRenderContext::s_ConstantBufferStorageTable;
ezMap<ezUInt32, ezDynamicArray<ezConstantBufferStorageBase*>> ezRenderContext::s_FreeConstantBufferStorage;

namespace
{
  ezUInt32 GetVertexBufferStride(ezGALDevice* pDevice, ezGALBufferHandle hBuffer)
  {
    if (!hBuffer.IsInvalidated())
    {
      if (const ezGALBuffer* pBuffer = pDevice->GetBuffer(hBuffer))
      {
        return pBuffer->GetDescription().m_uiStructSize;
      }
    }
    return 0;
  }

  void ConvertVertexStreamInfo(const ezVertexStreamInfo& stream, ezGALVertexAttribute& out_gal)
  {
    out_gal.m_eFormat = stream.m_Format;
    out_gal.m_eSemantic = stream.m_Semantic;
    out_gal.m_uiOffset = stream.m_uiOffset;
    out_gal.m_uiVertexBufferSlot = stream.m_uiVertexBufferSlot;
  }
} // namespace

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, RendererContext)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "Foundation",
  "Core",
  "ImmutableSamplers"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    ezRenderContext::RegisterImmutableSamplers();
    ezGALDevice::s_Events.AddEventHandler(ezMakeDelegate(&ezRenderContext::GALStaticDeviceEventHandler));
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezGALDevice::s_Events.RemoveEventHandler(ezMakeDelegate(&ezRenderContext::GALStaticDeviceEventHandler));
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    ezRenderContext::OnEngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    ezRenderContext::OnEngineShutdown();
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

//////////////////////////////////////////////////////////////////////////

ezRenderContext::Statistics::Statistics()
{
  Reset();
}

void ezRenderContext::Statistics::Reset()
{
  m_uiFailedDrawcalls = 0;
}

//////////////////////////////////////////////////////////////////////////

ezRenderContext* ezRenderContext::GetDefaultInstance()
{
  if (s_pDefaultInstance == nullptr)
    s_pDefaultInstance = CreateInstance(s_pCommandEncoder);

  EZ_ASSERT_DEBUG(s_pDefaultInstance != nullptr, "Default instance should have been created during device creation");
  return s_pDefaultInstance;
}

ezRenderContext* ezRenderContext::CreateInstance(ezGALCommandEncoder* pCommandEncoder)
{
  return EZ_DEFAULT_NEW(ezRenderContext, pCommandEncoder);
}

void ezRenderContext::DestroyInstance(ezRenderContext* pRenderer)
{
  EZ_DEFAULT_DELETE(pRenderer);
}

ezRenderContext::ezRenderContext(ezGALCommandEncoder* pCommandEncoder)
{
  s_Instances.PushBack(this);

  m_pGALCommandEncoder = pCommandEncoder;

  m_StateFlags = ezRenderContextFlags::AllStatesInvalid;
  m_GraphicsPipeline.m_Topology = ezGALPrimitiveTopology::ENUM_COUNT; // Set to something invalid
  m_uiMeshBufferPrimitiveCount = 0;
  m_DefaultTextureFilter = ezTextureFilterSetting::FixedAnisotropic4x;
  m_bAllowAsyncShaderLoading = false;

  m_hGlobalConstantBufferStorage = CreateConstantBufferStorage<ezGlobalConstants>();

  // If no push constants are supported, they are emulated via constant buffers.
  if (ezGALDevice::GetDefaultDevice()->GetCapabilities().m_uiMaxPushConstantsSize == 0)
  {
    m_hPushConstantsStorage = CreateConstantBufferStorage(128);
  }
  ResetContextState();
}

ezRenderContext::~ezRenderContext()
{
  DeleteConstantBufferStorage(m_hGlobalConstantBufferStorage);
  if (!m_hPushConstantsStorage.IsInvalidated())
  {
    DeleteConstantBufferStorage(m_hPushConstantsStorage);
  }

  if (s_pDefaultInstance == this)
    s_pDefaultInstance = nullptr;

  s_Instances.RemoveAndSwap(this);
}

ezRenderContext::Statistics ezRenderContext::GetAndResetStatistics()
{
  ezRenderContext::Statistics ret = m_Statistics;
  ret.Reset();

  return ret;
}

void ezRenderContext::BeginRendering(const ezGALRenderingSetup& renderingSetup, const ezRectFloat& viewport, const char* szName, bool bStereoSupport)
{
  EZ_ASSERT_DEBUG(m_bRendering == false && m_bCompute == false, "Already in a scope");
  m_bRendering = true;
  m_GraphicsPipeline.m_RenderPass = renderingSetup.GetRenderPass();
  m_StateFlags.Add(ezRenderContextFlags::PipelineChanged);
  const ezGALMSAASampleCount::Enum msaaSampleCount = renderingSetup.GetRenderPass().m_Msaa;
  if (msaaSampleCount != ezGALMSAASampleCount::None)
  {
    SetShaderPermutationVariable("MSAA", "TRUE");
  }
  else
  {
    SetShaderPermutationVariable("MSAA", "FALSE");
  }

  auto& gc = WriteGlobalConstants();
  gc.ViewportSize = ezVec4(viewport.width, viewport.height, 1.0f / viewport.width, 1.0f / viewport.height);
  gc.NumMsaaSamples = msaaSampleCount;

  m_pGALCommandEncoder->BeginRendering(renderingSetup, szName);

  m_pGALCommandEncoder->SetViewport(viewport);

  m_bStereoRendering = bStereoSupport;
}

void ezRenderContext::EndRendering()
{
  m_pGALCommandEncoder->EndRendering();

  m_bStereoRendering = false;
  m_bRendering = false;
  // TODO: The render context needs to reset its state after every encoding block if we want to record to separate command buffers.
  // Although this is currently not possible since a lot of high level code binds stuff only once per frame on the render context.
  // Resetting the state after every encoding block breaks those assumptions.
  // ResetContextState();
}

void ezRenderContext::BeginCompute(const char* szName /*= ""*/)
{
  EZ_ASSERT_DEBUG(m_bRendering == false && m_bCompute == false, "Already in a scope");
  m_pGALCommandEncoder->BeginCompute(szName);
  m_bCompute = true;
  m_StateFlags.Add(ezRenderContextFlags::PipelineChanged);
}

void ezRenderContext::EndCompute()
{
  m_pGALCommandEncoder->EndCompute();
  m_bCompute = false;
  m_StateFlags.Add(ezRenderContextFlags::PipelineChanged);
  // TODO: See EndRendering
  // ResetContextState();
}

void ezRenderContext::SetShaderPermutationVariable(const char* szName, const ezTempHashedString& sTempValue)
{
  ezTempHashedString sHashedName(szName);

  ezHashedString sName;
  ezHashedString sValue;
  if (ezShaderManager::IsPermutationValueAllowed(szName, sHashedName, sTempValue, sName, sValue))
  {
    SetShaderPermutationVariableInternal(sName, sValue);
  }
}

void ezRenderContext::SetShaderPermutationVariable(const ezHashedString& sName, const ezHashedString& sValue)
{
  if (ezShaderManager::IsPermutationValueAllowed(sName, sValue))
  {
    SetShaderPermutationVariableInternal(sName, sValue);
  }
}


void ezRenderContext::BindMaterial(const ezMaterialResourceHandle& hMaterial)
{
  // Don't set m_hMaterial directly since we first need to check whether the material has been modified in the mean time.
  m_hNewMaterial = hMaterial;
  m_StateFlags.Add(ezRenderContextFlags::MaterialBindingChanged);
}

void ezRenderContext::BindTexture2D(const ezTempHashedString& sSlotName, const ezTexture2DResourceHandle& hTexture,
  ezResourceAcquireMode acquireMode /*= ezResourceAcquireMode::AllowLoadingFallback*/)
{
  if (hTexture.IsValid())
  {
    ezResourceLock<ezTexture2DResource> pTexture(hTexture, acquireMode);
    BindTexture2D(sSlotName, ezGALDevice::GetDefaultDevice()->GetDefaultResourceView(pTexture->GetGALTexture()));
    BindSamplerState(sSlotName, pTexture->GetGALSamplerState());
  }
  else
  {
    BindTexture2D(sSlotName, ezGALTextureResourceViewHandle());
  }
}

void ezRenderContext::BindTexture3D(const ezTempHashedString& sSlotName, const ezTexture3DResourceHandle& hTexture,
  ezResourceAcquireMode acquireMode /*= ezResourceAcquireMode::AllowLoadingFallback*/)
{
  if (hTexture.IsValid())
  {
    ezResourceLock<ezTexture3DResource> pTexture(hTexture, acquireMode);
    BindTexture3D(sSlotName, ezGALDevice::GetDefaultDevice()->GetDefaultResourceView(pTexture->GetGALTexture()));
    BindSamplerState(sSlotName, pTexture->GetGALSamplerState());
  }
  else
  {
    BindTexture3D(sSlotName, ezGALTextureResourceViewHandle());
  }
}

void ezRenderContext::BindTextureCube(const ezTempHashedString& sSlotName, const ezTextureCubeResourceHandle& hTexture,
  ezResourceAcquireMode acquireMode /*= ezResourceAcquireMode::AllowLoadingFallback*/)
{
  if (hTexture.IsValid())
  {
    ezResourceLock<ezTextureCubeResource> pTexture(hTexture, acquireMode);
    BindTextureCube(sSlotName, ezGALDevice::GetDefaultDevice()->GetDefaultResourceView(pTexture->GetGALTexture()));
    BindSamplerState(sSlotName, pTexture->GetGALSamplerState());
  }
  else
  {
    BindTextureCube(sSlotName, ezGALTextureResourceViewHandle());
  }
}

void ezRenderContext::BindTexture2D(const ezTempHashedString& sSlotName, ezGALTextureResourceViewHandle hResourceView)
{
  ezGALTextureResourceViewHandle* pOldResourceView = nullptr;
  if (m_BoundTextures2D.TryGetValue(sSlotName.GetHash(), pOldResourceView))
  {
    if (*pOldResourceView == hResourceView)
      return;

    *pOldResourceView = hResourceView;
  }
  else
  {
    m_BoundTextures2D.Insert(sSlotName.GetHash(), hResourceView);
  }

  m_StateFlags.Add(ezRenderContextFlags::TextureBindingChanged);
}

void ezRenderContext::BindTexture3D(const ezTempHashedString& sSlotName, ezGALTextureResourceViewHandle hResourceView)
{
  ezGALTextureResourceViewHandle* pOldResourceView = nullptr;
  if (m_BoundTextures3D.TryGetValue(sSlotName.GetHash(), pOldResourceView))
  {
    if (*pOldResourceView == hResourceView)
      return;

    *pOldResourceView = hResourceView;
  }
  else
  {
    m_BoundTextures3D.Insert(sSlotName.GetHash(), hResourceView);
  }

  m_StateFlags.Add(ezRenderContextFlags::TextureBindingChanged);
}

void ezRenderContext::BindTextureCube(const ezTempHashedString& sSlotName, ezGALTextureResourceViewHandle hResourceView)
{
  ezGALTextureResourceViewHandle* pOldResourceView = nullptr;
  if (m_BoundTexturesCube.TryGetValue(sSlotName.GetHash(), pOldResourceView))
  {
    if (*pOldResourceView == hResourceView)
      return;

    *pOldResourceView = hResourceView;
  }
  else
  {
    m_BoundTexturesCube.Insert(sSlotName.GetHash(), hResourceView);
  }

  m_StateFlags.Add(ezRenderContextFlags::TextureBindingChanged);
}

void ezRenderContext::BindUAV(const ezTempHashedString& sSlotName, ezGALTextureUnorderedAccessViewHandle hUnorderedAccessView)
{
  ezGALTextureUnorderedAccessViewHandle* pOldResourceView = nullptr;
  if (m_BoundTextureUAVs.TryGetValue(sSlotName.GetHash(), pOldResourceView))
  {
    if (*pOldResourceView == hUnorderedAccessView)
      return;

    *pOldResourceView = hUnorderedAccessView;
  }
  else
  {
    m_BoundTextureUAVs.Insert(sSlotName.GetHash(), hUnorderedAccessView);
  }

  m_StateFlags.Add(ezRenderContextFlags::UAVBindingChanged);
}

void ezRenderContext::BindUAV(const ezTempHashedString& sSlotName, ezGALBufferUnorderedAccessViewHandle hUnorderedAccessView)
{
  ezGALBufferUnorderedAccessViewHandle* pOldResourceView = nullptr;
  if (m_BoundBufferUAVs.TryGetValue(sSlotName.GetHash(), pOldResourceView))
  {
    if (*pOldResourceView == hUnorderedAccessView)
      return;

    *pOldResourceView = hUnorderedAccessView;
  }
  else
  {
    m_BoundBufferUAVs.Insert(sSlotName.GetHash(), hUnorderedAccessView);
  }

  m_StateFlags.Add(ezRenderContextFlags::UAVBindingChanged);
}


void ezRenderContext::BindSamplerState(const ezTempHashedString& sSlotName, ezGALSamplerStateHandle hSamplerSate)
{
  EZ_ASSERT_DEBUG(sSlotName != "LinearSampler", "'LinearSampler' is a resevered sampler name and must not be set manually.");
  EZ_ASSERT_DEBUG(sSlotName != "LinearClampSampler", "'LinearClampSampler' is a resevered sampler name and must not be set manually.");
  EZ_ASSERT_DEBUG(sSlotName != "PointSampler", "'PointSampler' is a resevered sampler name and must not be set manually.");
  EZ_ASSERT_DEBUG(sSlotName != "PointClampSampler", "'PointClampSampler' is a resevered sampler name and must not be set manually.");

  ezGALSamplerStateHandle* pOldSamplerState = nullptr;
  if (m_BoundSamplers.TryGetValue(sSlotName.GetHash(), pOldSamplerState))
  {
    if (*pOldSamplerState == hSamplerSate)
      return;

    *pOldSamplerState = hSamplerSate;
  }
  else
  {
    m_BoundSamplers.Insert(sSlotName.GetHash(), hSamplerSate);
  }

  m_StateFlags.Add(ezRenderContextFlags::SamplerBindingChanged);
}

void ezRenderContext::BindBuffer(const ezTempHashedString& sSlotName, ezGALBufferResourceViewHandle hResourceView)
{
  ezGALBufferResourceViewHandle* pOldResourceView = nullptr;
  if (m_BoundBuffer.TryGetValue(sSlotName.GetHash(), pOldResourceView))
  {
    if (*pOldResourceView == hResourceView)
      return;

    *pOldResourceView = hResourceView;
  }
  else
  {
    m_BoundBuffer.Insert(sSlotName.GetHash(), hResourceView);
  }

  m_StateFlags.Add(ezRenderContextFlags::BufferBindingChanged);
}

void ezRenderContext::BindConstantBuffer(const ezTempHashedString& sSlotName, ezGALBufferHandle hConstantBuffer)
{
  BoundConstantBuffer* pBoundConstantBuffer = nullptr;
  if (m_BoundConstantBuffers.TryGetValue(sSlotName.GetHash(), pBoundConstantBuffer))
  {
    if (pBoundConstantBuffer->m_hConstantBuffer == hConstantBuffer)
      return;

    pBoundConstantBuffer->m_hConstantBuffer = hConstantBuffer;
    pBoundConstantBuffer->m_hConstantBufferStorage.Invalidate();
  }
  else
  {
    m_BoundConstantBuffers.Insert(sSlotName.GetHash(), BoundConstantBuffer(hConstantBuffer));
  }

  m_StateFlags.Add(ezRenderContextFlags::ConstantBufferBindingChanged);
}

void ezRenderContext::BindConstantBuffer(const ezTempHashedString& sSlotName, ezConstantBufferStorageHandle hConstantBufferStorage)
{
  BoundConstantBuffer* pBoundConstantBuffer = nullptr;
  if (m_BoundConstantBuffers.TryGetValue(sSlotName.GetHash(), pBoundConstantBuffer))
  {
    if (pBoundConstantBuffer->m_hConstantBufferStorage == hConstantBufferStorage)
      return;

    pBoundConstantBuffer->m_hConstantBuffer.Invalidate();
    pBoundConstantBuffer->m_hConstantBufferStorage = hConstantBufferStorage;
  }
  else
  {
    m_BoundConstantBuffers.Insert(sSlotName.GetHash(), BoundConstantBuffer(hConstantBufferStorage));
  }

  m_StateFlags.Add(ezRenderContextFlags::ConstantBufferBindingChanged);
}

void ezRenderContext::SetPushConstants(const ezTempHashedString& sSlotName, ezArrayPtr<const ezUInt8> data)
{

  if (!m_hPushConstantsStorage.IsInvalidated())
  {
    EZ_ASSERT_DEBUG(data.GetCount() <= 128, "Push constants are not allowed to be bigger than 128 bytes.");
    ezConstantBufferStorageBase* pStorage = nullptr;
    bool bResult = TryGetConstantBufferStorage(m_hPushConstantsStorage, pStorage);
    if (bResult)
    {
      ezArrayPtr<ezUInt8> targetStorage = pStorage->GetRawDataForWriting();
      ezMemoryUtils::Copy(targetStorage.GetPtr(), data.GetPtr(), data.GetCount());
      BindConstantBuffer(sSlotName, m_hPushConstantsStorage);
    }
  }
  else
  {
    EZ_ASSERT_DEBUG(data.GetCount() <= ezGALDevice::GetDefaultDevice()->GetCapabilities().m_uiMaxPushConstantsSize, "Push constants are not allowed to be bigger than {} bytes.", ezGALDevice::GetDefaultDevice()->GetCapabilities().m_uiMaxPushConstantsSize);
    m_pGALCommandEncoder->SetPushConstants(data);
  }
}

void ezRenderContext::BindShader(const ezShaderResourceHandle& hShader, ezBitflags<ezShaderBindFlags> flags)
{
  m_hMaterial.Invalidate();
  m_StateFlags.Remove(ezRenderContextFlags::MaterialBindingChanged);

  BindShaderInternal(hShader, flags);
}

void ezRenderContext::SetBlendState(ezGALBlendStateHandle hBlendState)
{
  m_GraphicsPipeline.m_hBlendState = hBlendState;
  m_StateFlags.Add(ezRenderContextFlags::PipelineChanged);
}

void ezRenderContext::SetDepthStencilState(ezGALDepthStencilStateHandle hDepthStencilState)
{
  m_GraphicsPipeline.m_hDepthStencilState = hDepthStencilState;
  m_StateFlags.Add(ezRenderContextFlags::PipelineChanged);
}

void ezRenderContext::SetRasterizerState(ezGALRasterizerStateHandle hRasterizerState)
{
  m_GraphicsPipeline.m_hRasterizerState = hRasterizerState;
  m_StateFlags.Add(ezRenderContextFlags::PipelineChanged);
}

void ezRenderContext::BindMeshBuffer(const ezMeshBufferResourceHandle& hMeshBuffer)
{
  ezResourceLock<ezMeshBufferResource> pMeshBuffer(hMeshBuffer, ezResourceAcquireMode::AllowLoadingFallback);
  BindMeshBuffer(pMeshBuffer->GetVertexBuffer(), pMeshBuffer->GetIndexBuffer(), &(pMeshBuffer->GetVertexDeclaration()), pMeshBuffer->GetTopology(),
    pMeshBuffer->GetPrimitiveCount());
}

void ezRenderContext::BindMeshBuffer(ezGALBufferHandle hVertexBuffer, ezGALBufferHandle hIndexBuffer,
  const ezVertexDeclarationInfo* pVertexDeclarationInfo, ezGALPrimitiveTopology::Enum topology, ezUInt32 uiPrimitiveCount, ezGALBufferHandle hVertexBuffer2, ezGALBufferHandle hVertexBuffer3, ezGALBufferHandle hVertexBuffer4)
{
  if (m_hVertexBuffers[0] == hVertexBuffer && m_hVertexBuffers[1] == hVertexBuffer2 && m_hVertexBuffers[2] == hVertexBuffer3 && m_hVertexBuffers[3] == hVertexBuffer4 && m_hIndexBuffer == hIndexBuffer && m_pVertexDeclarationInfo == pVertexDeclarationInfo && m_GraphicsPipeline.m_Topology == topology && m_uiMeshBufferPrimitiveCount == uiPrimitiveCount)
  {
    return;
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  if (pVertexDeclarationInfo)
  {
    for (ezUInt32 i1 = 0; i1 < pVertexDeclarationInfo->m_VertexStreams.GetCount(); ++i1)
    {
      for (ezUInt32 i2 = 0; i2 < pVertexDeclarationInfo->m_VertexStreams.GetCount(); ++i2)
      {
        if (i1 != i2)
        {
          EZ_ASSERT_DEBUG(pVertexDeclarationInfo->m_VertexStreams[i1].m_Semantic != pVertexDeclarationInfo->m_VertexStreams[i2].m_Semantic,
            "Same semantic cannot be used twice in the same vertex declaration");
        }
      }
    }
  }
#endif

  if (m_GraphicsPipeline.m_Topology != topology)
  {
    m_GraphicsPipeline.m_Topology = topology;
    m_StateFlags.Add(ezRenderContextFlags::PipelineChanged);

    ezTempHashedString sTopologies[] = {
      ezTempHashedString("TOPOLOGY_POINTS"),
      ezTempHashedString("TOPOLOGY_LINES"),
      ezTempHashedString("TOPOLOGY_TRIANGLES"),
      ezTempHashedString("TOPOLOGY_TRIANGLESTRIP"),
    };

    static_assert(EZ_ARRAY_SIZE(sTopologies) == ezGALPrimitiveTopology::ENUM_COUNT);

    SetShaderPermutationVariable("TOPOLOGY", sTopologies[m_GraphicsPipeline.m_Topology]);
  }

  m_hVertexBuffers[0] = hVertexBuffer;
  m_hVertexBuffers[1] = hVertexBuffer2;
  m_hVertexBuffers[2] = hVertexBuffer3;
  m_hVertexBuffers[3] = hVertexBuffer4;

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  for (ezUInt32 i = 0; i < 4; ++i)
  {
    m_VertexBufferStrides[i] = GetVertexBufferStride(pDevice, m_hVertexBuffers[i]);
    m_VertexBufferBindingRates[i] = ezGALVertexBindingRate::Vertex;
    m_VertexBufferOffsets[i] = 0;
  }

  m_hIndexBuffer = hIndexBuffer;
  m_pVertexDeclarationInfo = pVertexDeclarationInfo;
  m_uiMeshBufferPrimitiveCount = uiPrimitiveCount;

  m_StateFlags.Add(ezRenderContextFlags::MeshBufferBindingChanged);
}

void ezRenderContext::BindMeshBuffer(const ezDynamicMeshBufferResourceHandle& hDynamicMeshBuffer)
{
  ezResourceLock<ezDynamicMeshBufferResource> pMeshBuffer(hDynamicMeshBuffer, ezResourceAcquireMode::AllowLoadingFallback);
  BindMeshBuffer(pMeshBuffer->GetVertexBuffer(), pMeshBuffer->GetIndexBuffer(), &(pMeshBuffer->GetVertexDeclaration()), pMeshBuffer->GetDescriptor().m_Topology, pMeshBuffer->GetDescriptor().m_uiMaxPrimitives, pMeshBuffer->GetColorBuffer());
}

void ezRenderContext::BindVertexBuffer(ezGALBufferHandle hVertexBuffer, ezUInt32 uiSlot, ezEnum<ezGALVertexBindingRate> rate, ezUInt32 uiOffset)
{
  EZ_ASSERT_DEBUG(uiSlot < EZ_GAL_MAX_VERTEX_BUFFER_COUNT, "Vertex buffer slot is out of bounds");
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  m_hVertexBuffers[uiSlot] = hVertexBuffer;
  m_VertexBufferStrides[uiSlot] = GetVertexBufferStride(pDevice, hVertexBuffer);
  m_VertexBufferBindingRates[uiSlot] = rate;
  m_VertexBufferOffsets[uiSlot] = uiOffset;

  m_StateFlags.Add(ezRenderContextFlags::MeshBufferBindingChanged);
}

void ezRenderContext::SetCustomVertexStreams(ezArrayPtr<ezVertexStreamInfo> customStreams)
{
  m_CustomVertexStreams.m_VertexStreams = customStreams;
  m_CustomVertexStreams.ComputeHash();
  m_StateFlags.Add(ezRenderContextFlags::MeshBufferBindingChanged);
}

ezResult ezRenderContext::DrawMeshBuffer(ezUInt32 uiPrimitiveCount, ezUInt32 uiFirstPrimitive, ezUInt32 uiInstanceCount)
{
  if (ApplyContextStates().Failed() || uiPrimitiveCount == 0 || uiInstanceCount == 0)
  {
    m_Statistics.m_uiFailedDrawcalls++;
    return EZ_FAILURE;
  }

  EZ_ASSERT_DEV(uiFirstPrimitive < m_uiMeshBufferPrimitiveCount, "Invalid primitive range: first primitive ({0}) can't be larger than number of primitives ({1})", uiFirstPrimitive, uiPrimitiveCount);

  uiPrimitiveCount = ezMath::Min(uiPrimitiveCount, m_uiMeshBufferPrimitiveCount - uiFirstPrimitive);
  EZ_ASSERT_DEV(uiPrimitiveCount > 0, "Invalid primitive range: number of primitives can't be zero.");

  auto pCommandEncoder = GetCommandEncoder();

  const ezUInt32 uiIndexCount = ezGALPrimitiveTopology::GetIndexCount(m_GraphicsPipeline.m_Topology, uiPrimitiveCount);
  const ezUInt32 uiFirstIndex = ezGALPrimitiveTopology::GetIndexCount(m_GraphicsPipeline.m_Topology, uiFirstPrimitive);

  if (m_bStereoRendering)
  {
    uiInstanceCount *= 2;
  }

  if (uiInstanceCount > 1)
  {
    if (!m_hIndexBuffer.IsInvalidated())
    {
      return pCommandEncoder->DrawIndexedInstanced(uiIndexCount, uiInstanceCount, uiFirstIndex);
    }
    else
    {
      return pCommandEncoder->DrawInstanced(uiIndexCount, uiInstanceCount, uiFirstIndex);
    }
  }
  else
  {
    if (!m_hIndexBuffer.IsInvalidated())
    {
      return pCommandEncoder->DrawIndexed(uiIndexCount, uiFirstIndex);
    }
    else
    {
      return pCommandEncoder->Draw(uiIndexCount, uiFirstIndex);
    }
  }

  return EZ_SUCCESS;
}

ezResult ezRenderContext::Dispatch(ezUInt32 uiThreadGroupCountX, ezUInt32 uiThreadGroupCountY, ezUInt32 uiThreadGroupCountZ)
{
  if (ApplyContextStates().Failed())
  {
    m_Statistics.m_uiFailedDrawcalls++;
    return EZ_FAILURE;
  }

  return GetCommandEncoder()->Dispatch(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);
}

ezResult ezRenderContext::ApplyContextStates(bool bForce)
{
  EZ_ASSERT_DEBUG(m_bRendering || m_bCompute, "Must be either in a rendering or compute scope");

  // First apply material state since this can modify all other states.
  if (bForce || m_StateFlags.IsSet(ezRenderContextFlags::MaterialBindingChanged))
  {
    ApplyMaterialState();
    m_StateFlags.Remove(ezRenderContextFlags::MaterialBindingChanged);
  }

  ezShaderPermutationResource* pShaderPermutation = nullptr;
  EZ_SCOPE_EXIT(if (pShaderPermutation != nullptr) { ezResourceManager::EndAcquireResource(pShaderPermutation); });

  bool bRebuildVertexDeclaration = m_StateFlags.IsAnySet(ezRenderContextFlags::ShaderStateChanged | ezRenderContextFlags::MeshBufferBindingChanged);

  if (bForce || m_StateFlags.IsSet(ezRenderContextFlags::ShaderStateChanged))
  {
    pShaderPermutation = ApplyShaderState();
    if (pShaderPermutation == nullptr)
    {
      return EZ_FAILURE;
    }

    m_StateFlags.Remove(ezRenderContextFlags::ShaderStateChanged);
  }

  if (m_hActiveShaderPermutation.IsValid())
  {
    const bool bDirty = (bForce || m_StateFlags.IsAnySet(ezRenderContextFlags::TextureBindingChanged | ezRenderContextFlags::UAVBindingChanged |
                                                         ezRenderContextFlags::SamplerBindingChanged | ezRenderContextFlags::BufferBindingChanged |
                                                         ezRenderContextFlags::ConstantBufferBindingChanged));

    const ezGALShader* pShader = nullptr;
    if (bDirty)
    {
      if (pShaderPermutation == nullptr)
      {
        pShaderPermutation = ezResourceManager::BeginAcquireResource(m_hActiveShaderPermutation, ezResourceAcquireMode::BlockTillLoaded);
      }
      if (pShaderPermutation == nullptr)
      {
        return EZ_FAILURE;
      }
      // #TODO_SHADER It's a bit unclean that we need to acquire the GAL shader on this level. Unfortunately, we need the resource binding on both the GAL and the high level renderer and the only alternative is some kind of duplication of the data.
      pShader = ezGALDevice::GetDefaultDevice()->GetShader(m_hActiveGALShader);
    }


    ezLogBlock applyBindingsBlock("Applying Shader Bindings", pShaderPermutation ? pShaderPermutation->GetResourceDescription().GetData() : "");
    UploadConstants();
    if (bDirty)
    {
      if (bForce || m_StateFlags.IsSet(ezRenderContextFlags::UAVBindingChanged))
      {
        ApplyUAVBindings(pShader);
        m_StateFlags.Remove(ezRenderContextFlags::UAVBindingChanged);
      }

      if (bForce || m_StateFlags.IsSet(ezRenderContextFlags::TextureBindingChanged))
      {
        ApplyTextureBindings(pShader);
        m_StateFlags.Remove(ezRenderContextFlags::TextureBindingChanged);
      }

      if (bForce || m_StateFlags.IsSet(ezRenderContextFlags::SamplerBindingChanged))
      {
        ApplySamplerBindings(pShader);
        m_StateFlags.Remove(ezRenderContextFlags::SamplerBindingChanged);
      }

      if (bForce || m_StateFlags.IsSet(ezRenderContextFlags::BufferBindingChanged))
      {
        ApplyBufferBindings(pShader);
        m_StateFlags.Remove(ezRenderContextFlags::BufferBindingChanged);
      }

      if (bForce || m_StateFlags.IsSet(ezRenderContextFlags::ConstantBufferBindingChanged))
      {
        ApplyConstantBufferBindings(pShader);
        m_StateFlags.Remove(ezRenderContextFlags::ConstantBufferBindingChanged);
      }
    }
  }

  if ((bForce || bRebuildVertexDeclaration) && !m_bCompute)
  {
    if (m_hActiveGALShader.IsInvalidated())
      return EZ_FAILURE;

    auto pCommandEncoder = GetCommandEncoder();

    if (bForce || m_StateFlags.IsSet(ezRenderContextFlags::MeshBufferBindingChanged))
    {
      for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(m_hVertexBuffers); ++i)
      {
        pCommandEncoder->SetVertexBuffer(i, m_hVertexBuffers[i], m_VertexBufferOffsets[i]);
      }

      if (!m_hIndexBuffer.IsInvalidated())
        pCommandEncoder->SetIndexBuffer(m_hIndexBuffer);
    }

    ezGALVertexDeclarationHandle hVertexDeclaration;
    const bool bHasVertexDeclarations = m_pVertexDeclarationInfo != nullptr || !m_CustomVertexStreams.m_VertexStreams.IsEmpty();
    if (bHasVertexDeclarations && BuildVertexDeclaration(m_hActiveGALShader, m_VertexBufferStrides, m_VertexBufferBindingRates, *m_pVertexDeclarationInfo, m_CustomVertexStreams, hVertexDeclaration).Failed())
      return EZ_FAILURE;

    // If there is a vertex buffer we need a valid vertex declaration as well.
    if ((!m_hVertexBuffers[0].IsInvalidated() || !m_hVertexBuffers[1].IsInvalidated() || !m_hVertexBuffers[2].IsInvalidated() || !m_hVertexBuffers[3].IsInvalidated()) && hVertexDeclaration.IsInvalidated())
      return EZ_FAILURE;

    m_GraphicsPipeline.m_hVertexDeclaration = hVertexDeclaration;
    m_StateFlags.Add(ezRenderContextFlags::PipelineChanged);

    m_StateFlags.Remove(ezRenderContextFlags::MeshBufferBindingChanged);
  }

  if (bForce || m_StateFlags.IsSet(ezRenderContextFlags::PipelineChanged))
  {
    if (m_bRendering)
      m_pGALCommandEncoder->SetGraphicsPipeline(ezGALPipelineCache::GetPipeline(m_GraphicsPipeline));
    else if (m_bCompute)
      m_pGALCommandEncoder->SetComputePipeline(ezGALPipelineCache::GetPipeline(m_ComputePipeline));
  }

  return EZ_SUCCESS;
}

void ezRenderContext::ResetContextState()
{
  EZ_PROFILE_SCOPE("ezRenderContext::ResetContextState");

  m_StateFlags = ezRenderContextFlags::AllStatesInvalid;

  m_hActiveShader.Invalidate();
  m_hActiveGALShader.Invalidate();

  m_PermutationVariables.Clear();
  m_hNewMaterial.Invalidate();
  m_hMaterial.Invalidate();

  m_hActiveShaderPermutation.Invalidate();

  static_assert(EZ_ARRAY_SIZE(m_hVertexBuffers) == EZ_GAL_MAX_VERTEX_BUFFER_COUNT);
  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(m_hVertexBuffers); ++i)
  {
    m_hVertexBuffers[i].Invalidate();
    m_VertexBufferStrides[i] = 0;
    m_VertexBufferBindingRates[i] = ezGALVertexBindingRate::Vertex;
    m_VertexBufferOffsets[i] = 0;
  }
  m_CustomVertexStreams.m_VertexStreams.Clear();
  m_CustomVertexStreams.m_uiHash = 0;
  m_hIndexBuffer.Invalidate();
  m_pVertexDeclarationInfo = nullptr;
  m_GraphicsPipeline.m_Topology = ezGALPrimitiveTopology::ENUM_COUNT; // Set to something invalid
  m_uiMeshBufferPrimitiveCount = 0;

  m_BoundTextures2D.Clear();
  m_BoundTextures3D.Clear();
  m_BoundTexturesCube.Clear();
  m_BoundBuffer.Clear();

  m_BoundSamplers.Clear();
  // Platforms that do not support immutable samples like DX11 still need them to be bound manually, so they are bound here.
  ezTempHashedString sLinearSampler("LinearSampler");
  for (auto it : ezGALImmutableSamplers::GetImmutableSamplers())
  {
    m_BoundSamplers.Insert(it.Key().GetHash(), it.Value());

    if (it.Key() == sLinearSampler)
    {
      m_hFallbackSampler = it.Value();
    }
  }
  EZ_ASSERT_DEBUG(!m_hFallbackSampler.IsInvalidated(), "'LinearSampler' should have been registered as an immutable sampler.");

  m_BoundTextureUAVs.Clear();
  m_BoundBufferUAVs.Clear();
  m_BoundConstantBuffers.Clear();
}

ezGlobalConstants& ezRenderContext::WriteGlobalConstants()
{
  ezConstantBufferStorage<ezGlobalConstants>* pStorage = nullptr;
  EZ_VERIFY(TryGetConstantBufferStorage(m_hGlobalConstantBufferStorage, pStorage), "Invalid Global Constant Storage");
  return pStorage->GetDataForWriting();
}

const ezGlobalConstants& ezRenderContext::ReadGlobalConstants() const
{
  ezConstantBufferStorage<ezGlobalConstants>* pStorage = nullptr;
  EZ_VERIFY(TryGetConstantBufferStorage(m_hGlobalConstantBufferStorage, pStorage), "Invalid Global Constant Storage");
  return pStorage->GetDataForReading();
}

void ezRenderContext::SetGlobalAndWorldTimeConstants(ezTime worldTime)
{
  auto& gc = WriteGlobalConstants();

  // Wrap around to prevent floating point issues. A wrap around of 1000 allows all frequencies with 3 digits after the decimal.
  const double fWrapAround = 1000.0;
  gc.DeltaTime = (float)ezClock::GetGlobalClock()->GetTimeDiff().GetSeconds();
  gc.GlobalTime = (float)ezMath::Mod(ezClock::GetGlobalClock()->GetAccumulatedTime().GetSeconds(), fWrapAround);
  gc.WorldTime = (float)ezMath::Mod(worldTime.GetSeconds(), fWrapAround);
}

// static
ezConstantBufferStorageHandle ezRenderContext::CreateConstantBufferStorage(ezUInt32 uiSizeInBytes, ezConstantBufferStorageBase*& out_pStorage)
{
  EZ_ASSERT_DEV(ezMemoryUtils::IsSizeAligned(uiSizeInBytes, 16u), "Storage struct for constant buffer is not aligned to 16 bytes");

  EZ_LOCK(s_ConstantBufferStorageMutex);

  ezConstantBufferStorageBase* pStorage = nullptr;

  auto it = s_FreeConstantBufferStorage.Find(uiSizeInBytes);
  if (it.IsValid())
  {
    ezDynamicArray<ezConstantBufferStorageBase*>& storageForSize = it.Value();
    if (!storageForSize.IsEmpty())
    {
      pStorage = storageForSize[0];
      storageForSize.RemoveAtAndSwap(0);
    }
  }

  if (pStorage == nullptr)
  {
    pStorage = EZ_DEFAULT_NEW(ezConstantBufferStorageBase, uiSizeInBytes);
  }

  out_pStorage = pStorage;
  return ezConstantBufferStorageHandle(s_ConstantBufferStorageTable.Insert(pStorage));
}

// static
void ezRenderContext::DeleteConstantBufferStorage(ezConstantBufferStorageHandle hStorage)
{
  EZ_LOCK(s_ConstantBufferStorageMutex);

  ezConstantBufferStorageBase* pStorage = nullptr;
  if (!s_ConstantBufferStorageTable.Remove(hStorage.m_InternalId, &pStorage))
  {
    // already deleted
    return;
  }
  pStorage->BeforeBeginFrame();

  ezUInt32 uiSizeInBytes = pStorage->m_Data.GetCount();

  auto it = s_FreeConstantBufferStorage.Find(uiSizeInBytes);
  if (!it.IsValid())
  {
    it = s_FreeConstantBufferStorage.Insert(uiSizeInBytes, ezDynamicArray<ezConstantBufferStorageBase*>());
  }

  it.Value().PushBack(pStorage);
}

// static
bool ezRenderContext::TryGetConstantBufferStorage(ezConstantBufferStorageHandle hStorage, ezConstantBufferStorageBase*& out_pStorage)
{
  EZ_LOCK(s_ConstantBufferStorageMutex);
  return s_ConstantBufferStorageTable.TryGetValue(hStorage.m_InternalId, out_pStorage);
}

// static
ezGALSamplerStateCreationDescription ezRenderContext::GetDefaultSamplerState(ezBitflags<ezDefaultSamplerFlags> flags)
{
  ezGALSamplerStateCreationDescription desc;
  desc.m_MinFilter = flags.IsSet(ezDefaultSamplerFlags::LinearFiltering) ? ezGALTextureFilterMode::Linear : ezGALTextureFilterMode::Point;
  desc.m_MagFilter = flags.IsSet(ezDefaultSamplerFlags::LinearFiltering) ? ezGALTextureFilterMode::Linear : ezGALTextureFilterMode::Point;
  desc.m_MipFilter = flags.IsSet(ezDefaultSamplerFlags::LinearFiltering) ? ezGALTextureFilterMode::Linear : ezGALTextureFilterMode::Point;

  desc.m_AddressU = flags.IsSet(ezDefaultSamplerFlags::Clamp) ? ezImageAddressMode::Clamp : ezImageAddressMode::Repeat;
  desc.m_AddressV = flags.IsSet(ezDefaultSamplerFlags::Clamp) ? ezImageAddressMode::Clamp : ezImageAddressMode::Repeat;
  desc.m_AddressW = flags.IsSet(ezDefaultSamplerFlags::Clamp) ? ezImageAddressMode::Clamp : ezImageAddressMode::Repeat;
  return desc;
}

// private functions
//////////////////////////////////////////////////////////////////////////

// static
void ezRenderContext::LoadBuiltinShader(ezShaderUtils::ezBuiltinShaderType type, ezShaderUtils::ezBuiltinShader& out_shader)
{
  ezShaderResourceHandle hActiveShader;
  bool bStereo = false;
  switch (type)
  {
    case ezShaderUtils::ezBuiltinShaderType::CopyImageArray:
      bStereo = true;
      [[fallthrough]];
    case ezShaderUtils::ezBuiltinShaderType::CopyImage:
      hActiveShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/Copy.ezShader");
      break;
    case ezShaderUtils::ezBuiltinShaderType::DownscaleImageArray:
      bStereo = true;
      [[fallthrough]];
    case ezShaderUtils::ezBuiltinShaderType::DownscaleImage:
      hActiveShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/Downscale.ezShader");
      break;
  }

  EZ_ASSERT_DEV(hActiveShader.IsValid(), "Could not load builtin shader!");

  ezHashTable<ezHashedString, ezHashedString> permutationVariables;
  static ezHashedString sVSRTAI = ezMakeHashedString("VERTEX_SHADER_RENDER_TARGET_ARRAY_INDEX");
  static ezHashedString sTrue = ezMakeHashedString("TRUE");
  static ezHashedString sFalse = ezMakeHashedString("FALSE");
  static ezHashedString sCameraMode = ezMakeHashedString("CAMERA_MODE");
  static ezHashedString sPerspective = ezMakeHashedString("CAMERA_MODE_PERSPECTIVE");
  static ezHashedString sStereo = ezMakeHashedString("CAMERA_MODE_STEREO");

  permutationVariables.Insert(sCameraMode, bStereo ? sStereo : sPerspective);
  if (ezGALDevice::GetDefaultDevice()->GetCapabilities().m_bSupportsVSRenderTargetArrayIndex)
    permutationVariables.Insert(sVSRTAI, sTrue);
  else
    permutationVariables.Insert(sVSRTAI, sFalse);


  ezShaderPermutationResourceHandle hActiveShaderPermutation = ezShaderManager::PreloadSinglePermutation(hActiveShader, permutationVariables, false);

  EZ_ASSERT_DEV(hActiveShaderPermutation.IsValid(), "Could not load builtin shader permutation!");

  ezResourceLock<ezShaderPermutationResource> pShaderPermutation(hActiveShaderPermutation, ezResourceAcquireMode::BlockTillLoaded);

  EZ_ASSERT_DEV(pShaderPermutation->IsShaderValid(), "Builtin shader permutation shader is invalid!");

  out_shader.m_hActiveGALShader = pShaderPermutation->GetGALShader();
  EZ_ASSERT_DEV(!out_shader.m_hActiveGALShader.IsInvalidated(), "Invalid GAL Shader handle.");

  out_shader.m_hBlendState = pShaderPermutation->GetBlendState();
  out_shader.m_hDepthStencilState = pShaderPermutation->GetDepthStencilState();
  out_shader.m_hRasterizerState = pShaderPermutation->GetRasterizerState();
}

// static
void ezRenderContext::RegisterImmutableSamplers()
{
  ezGALImmutableSamplers::RegisterImmutableSampler(ezMakeHashedString("LinearSampler"), GetDefaultSamplerState(ezDefaultSamplerFlags::LinearFiltering)).AssertSuccess("Failed to register immutable sampler");
  ezGALImmutableSamplers::RegisterImmutableSampler(ezMakeHashedString("LinearClampSampler"), GetDefaultSamplerState(ezDefaultSamplerFlags::LinearFiltering | ezDefaultSamplerFlags::Clamp)).AssertSuccess("Failed to register immutable sampler");
  ezGALImmutableSamplers::RegisterImmutableSampler(ezMakeHashedString("PointSampler"), GetDefaultSamplerState(ezDefaultSamplerFlags::PointFiltering)).AssertSuccess("Failed to register immutable sampler");
  ezGALImmutableSamplers::RegisterImmutableSampler(ezMakeHashedString("PointClampSampler"), GetDefaultSamplerState(ezDefaultSamplerFlags::PointFiltering | ezDefaultSamplerFlags::Clamp)).AssertSuccess("Failed to register immutable sampler");
}

// static
void ezRenderContext::OnEngineStartup()
{
  ezShaderUtils::g_RequestBuiltinShaderCallback = ezMakeDelegate(ezRenderContext::LoadBuiltinShader);
}

// static
void ezRenderContext::OnEngineShutdown()
{
  ezShaderUtils::g_RequestBuiltinShaderCallback = {};
  ezShaderStageBinary::OnEngineShutdown();

  for (auto rc : s_Instances)
    EZ_DEFAULT_DELETE(rc);

  s_Instances.Clear();

  // Cleanup vertex declarations
  {
    for (auto it = s_GALVertexDeclarations.GetIterator(); it.IsValid(); ++it)
    {
      ezGALDevice::GetDefaultDevice()->DestroyVertexDeclaration(it.Value());
    }

    s_GALVertexDeclarations.Clear();
  }

  // Cleanup constant buffer storage
  {
    for (auto it = s_ConstantBufferStorageTable.GetIterator(); it.IsValid(); ++it)
    {
      ezConstantBufferStorageBase* pStorage = it.Value();
      EZ_DEFAULT_DELETE(pStorage);
    }

    s_ConstantBufferStorageTable.Clear();

    for (auto it = s_FreeConstantBufferStorage.GetIterator(); it.IsValid(); ++it)
    {
      ezDynamicArray<ezConstantBufferStorageBase*>& storageForSize = it.Value();
      for (auto& pStorage : storageForSize)
      {
        EZ_DEFAULT_DELETE(pStorage);
      }
    }

    s_FreeConstantBufferStorage.Clear();
  }
}

// static
void ezRenderContext::GALStaticDeviceEventHandler(const ezGALDeviceEvent& e)
{
  if (e.m_Type == ezGALDeviceEvent::Type::AfterBeginCommands)
  {
    s_pCommandEncoder = e.m_pCommandEncoder;
    if (s_pDefaultInstance)
    {
      s_pDefaultInstance->m_StateFlags = ezRenderContextFlags::AllStatesInvalid;
      s_pDefaultInstance->m_pGALCommandEncoder = e.m_pCommandEncoder;
    }
  }
  else if (e.m_Type == ezGALDeviceEvent::Type::BeforeEndCommands)
  {
    s_pCommandEncoder = nullptr;
    if (s_pDefaultInstance)
      s_pDefaultInstance->m_pGALCommandEncoder = nullptr;
  }
  else if (e.m_Type == ezGALDeviceEvent::Type::BeforeBeginFrame)
  {
    for (auto it = s_ConstantBufferStorageTable.GetIterator(); it.IsValid(); ++it)
    {
      it.Value()->BeforeBeginFrame();
    }
  }
}

// static
ezResult ezRenderContext::BuildVertexDeclaration(ezGALShaderHandle hShader, ezArrayPtr<ezUInt32> vertexBufferStrides, ezArrayPtr<ezEnum<ezGALVertexBindingRate>> vertexBufferBindingRates, const ezVertexDeclarationInfo& decl, const ezVertexDeclarationInfo& customVertexDecl, ezGALVertexDeclarationHandle& out_Declaration)
{
  ezInt32 iHighestUsedBinding = -1;
  for (ezUInt32 slot = 0; slot < decl.m_VertexStreams.GetCount(); ++slot)
  {
    iHighestUsedBinding = ezMath::Max(iHighestUsedBinding, static_cast<ezInt32>(decl.m_VertexStreams[slot].m_uiVertexBufferSlot));
  }
  for (ezUInt32 slot = 0; slot < customVertexDecl.m_VertexStreams.GetCount(); ++slot)
  {
    iHighestUsedBinding = ezMath::Max(iHighestUsedBinding, static_cast<ezInt32>(customVertexDecl.m_VertexStreams[slot].m_uiVertexBufferSlot));
  }
  EZ_ASSERT_DEBUG(iHighestUsedBinding < (ezInt32)vertexBufferStrides.GetCount(), "Not enough vertex buffer strides");
  EZ_ASSERT_DEBUG(iHighestUsedBinding < (ezInt32)vertexBufferBindingRates.GetCount(), "Not enough vertex buffer binding rates");

  ShaderVertexDecl svd;
  {
    svd.m_hShader = hShader;
    ezHashStreamWriter32 writer;
    writer << decl.m_uiHash;
    writer << customVertexDecl.m_uiHash;
    for (ezInt32 bufferIndex = 0; bufferIndex <= iHighestUsedBinding; ++bufferIndex)
    {
      writer << vertexBufferStrides[bufferIndex];
      writer << vertexBufferBindingRates[bufferIndex];
    }
    svd.m_uiVertexDeclarationHash = writer.GetHashValue();
  }

  bool bExisted = false;
  auto it = s_GALVertexDeclarations.FindOrAdd(svd, &bExisted);

  if (!bExisted)
  {
    ezGALVertexDeclarationCreationDescription vd;
    vd.m_hShader = hShader;

    for (ezUInt32 slot = 0; slot < decl.m_VertexStreams.GetCount(); ++slot)
    {
      ConvertVertexStreamInfo(decl.m_VertexStreams[slot], vd.m_VertexAttributes.ExpandAndGetRef());
    }
    for (ezUInt32 slot = 0; slot < customVertexDecl.m_VertexStreams.GetCount(); ++slot)
    {
      ConvertVertexStreamInfo(customVertexDecl.m_VertexStreams[slot], vd.m_VertexAttributes.ExpandAndGetRef());
    }

    for (ezInt32 bufferIndex = 0; bufferIndex <= iHighestUsedBinding; ++bufferIndex)
    {
      ezGALVertexBinding binding;
      binding.m_uiStride = vertexBufferStrides[bufferIndex];
      binding.m_Rate = vertexBufferBindingRates[bufferIndex];
      vd.m_VertexBindings.PushBack(binding);
    }
    out_Declaration = ezGALDevice::GetDefaultDevice()->CreateVertexDeclaration(vd);

    if (out_Declaration.IsInvalidated())
    {
      /* This can happen when the resource system gives you a fallback resource, which then selects a shader that
      does not fit the mesh layout.
      E.g. when a material is not yet loaded and the fallback material is used, that fallback material may
      use another shader, that requires more data streams, than what the mesh provides.
      This problem will go away, once the proper material is loaded.

      This can be fixed by ensuring that the fallback material uses a shader that only requires data that is
      always there, e.g. only position and maybe a texcoord, and of course all meshes must provide at least those
      data streams.

      Otherwise, this is harmless, the renderer will ignore invalid drawcalls and once all the correct stuff is
      available, it will work.
      */

      ezLog::Warning("Failed to create vertex declaration");
      return EZ_FAILURE;
    }

    it.Value() = out_Declaration;
  }

  out_Declaration = it.Value();
  return EZ_SUCCESS;
}

void ezRenderContext::UploadConstants()
{
  BindConstantBuffer("ezGlobalConstants", m_hGlobalConstantBufferStorage);

  for (auto it = m_BoundConstantBuffers.GetIterator(); it.IsValid(); ++it)
  {
    ezConstantBufferStorageHandle hConstantBufferStorage = it.Value().m_hConstantBufferStorage;
    ezConstantBufferStorageBase* pConstantBufferStorage = nullptr;
    if (TryGetConstantBufferStorage(hConstantBufferStorage, pConstantBufferStorage))
    {
      pConstantBufferStorage->UploadData(m_pGALCommandEncoder);
    }
  }
}

void ezRenderContext::SetShaderPermutationVariableInternal(const ezHashedString& sName, const ezHashedString& sValue)
{
  ezHashedString* pOldValue = nullptr;
  m_PermutationVariables.TryGetValue(sName, pOldValue);

  if (pOldValue == nullptr || *pOldValue != sValue)
  {
    m_PermutationVariables.Insert(sName, sValue);
    m_StateFlags.Add(ezRenderContextFlags::ShaderStateChanged);
  }
}

void ezRenderContext::BindShaderInternal(const ezShaderResourceHandle& hShader, ezBitflags<ezShaderBindFlags> flags)
{
  if (flags.IsAnySet(ezShaderBindFlags::ForceRebind) || m_hActiveShader != hShader)
  {
    m_ShaderBindFlags = flags;
    m_hActiveShader = hShader;

    m_StateFlags.Add(ezRenderContextFlags::ShaderStateChanged);
  }
}

ezShaderPermutationResource* ezRenderContext::ApplyShaderState()
{
  m_hActiveGALShader.Invalidate();

  m_StateFlags.Add(ezRenderContextFlags::TextureBindingChanged | ezRenderContextFlags::SamplerBindingChanged | ezRenderContextFlags::BufferBindingChanged | ezRenderContextFlags::ConstantBufferBindingChanged | ezRenderContextFlags::PipelineChanged);

  if (!m_hActiveShader.IsValid())
    return nullptr;

  m_hActiveShaderPermutation = ezShaderManager::PreloadSinglePermutation(m_hActiveShader, m_PermutationVariables, m_bAllowAsyncShaderLoading);

  if (!m_hActiveShaderPermutation.IsValid())
    return nullptr;

  ezShaderPermutationResource* pShaderPermutation = ezResourceManager::BeginAcquireResource(
    m_hActiveShaderPermutation, m_bAllowAsyncShaderLoading ? ezResourceAcquireMode::AllowLoadingFallback : ezResourceAcquireMode::BlockTillLoaded);

  if (!pShaderPermutation->IsShaderValid())
  {
    ezResourceManager::EndAcquireResource(pShaderPermutation);
    return nullptr;
  }

  m_hActiveGALShader = pShaderPermutation->GetGALShader();
  m_GraphicsPipeline.m_hShader = m_hActiveGALShader;
  m_ComputePipeline.m_hShader = m_hActiveGALShader;
  EZ_ASSERT_DEV(!m_hActiveGALShader.IsInvalidated(), "Invalid GAL Shader handle.");

  // Set render state from shader
  if (!m_bCompute)
  {
    if (!m_ShaderBindFlags.IsSet(ezShaderBindFlags::NoBlendState))
      m_GraphicsPipeline.m_hBlendState = pShaderPermutation->GetBlendState();

    if (!m_ShaderBindFlags.IsSet(ezShaderBindFlags::NoRasterizerState))
      m_GraphicsPipeline.m_hRasterizerState = pShaderPermutation->GetRasterizerState();

    if (!m_ShaderBindFlags.IsSet(ezShaderBindFlags::NoDepthStencilState))
      m_GraphicsPipeline.m_hDepthStencilState = pShaderPermutation->GetDepthStencilState();
  }

  return pShaderPermutation;
}

void ezRenderContext::ApplyMaterialState()
{
  if (!m_hNewMaterial.IsValid())
  {
    BindShaderInternal(ezShaderResourceHandle(), ezShaderBindFlags::Default);
    return;
  }

  // check whether material has been modified
  ezResourceLock<ezMaterialResource> pMaterial(m_hNewMaterial, ezResourceAcquireMode::AllowLoadingFallback);

  if (m_hNewMaterial != m_hMaterial)
  {
    const ezMaterialManager::MaterialData* data = ezMaterialManager::GetMaterialData(pMaterial.GetPointer());
    if (data == nullptr)
    {
      BindShaderInternal(ezShaderResourceHandle(), ezShaderBindFlags::Default);
      return;
    }

    BindShaderInternal(data->m_hShader, ezShaderBindFlags::Default);

    if (!data->m_hStructuredBufferView.IsInvalidated())
    {
      BindBuffer("materialData", data->m_hStructuredBufferView);
    }
    else if (!data->m_hConstantBuffer.IsInvalidated())
    {
      BindConstantBuffer("materialData", data->m_hConstantBuffer);
    }

    for (const ezPermutationVar& perm : data->m_PermutationVars)
    {
      SetShaderPermutationVariableInternal(perm.m_sName, perm.m_sValue);
    }

    for (const ezMaterialResourceDescriptor::Texture2DBinding& binding : data->m_Texture2DBindings)
    {
      BindTexture2D(binding.m_Name, binding.m_Value);
    }

    for (const ezMaterialResourceDescriptor::TextureCubeBinding& binding : data->m_TextureCubeBindings)
    {
      BindTextureCube(binding.m_Name, binding.m_Value);
    }

    m_hMaterial = m_hNewMaterial;
  }
}

void ezRenderContext::ApplyConstantBufferBindings(const ezGALShader* pShader)
{
  const auto& bindings = pShader->GetBindingMapping();
  for (const ezShaderResourceBinding& binding : bindings)
  {
    if (binding.m_ResourceType != ezGALShaderResourceType::ConstantBuffer)
      continue;

    const ezUInt64 uiResourceHash = binding.m_sName.GetHash();

    BoundConstantBuffer boundConstantBuffer;
    if (!m_BoundConstantBuffers.TryGetValue(uiResourceHash, boundConstantBuffer))
    {
      // If the shader was compiled with debug info the shader compiler will not strip unused resources and
      // thus this error would trigger although the shader doesn't actually use the resource.
      // #TODO_SHADER if (!pBinary->GetByteCode()->m_bWasCompiledWithDebug)
      {
        ezLog::Error("No resource is bound for constant buffer slot '{0}'", binding.m_sName);
      }
      m_pGALCommandEncoder->SetConstantBuffer(binding, ezGALBufferHandle());
      continue;
    }

    if (!boundConstantBuffer.m_hConstantBuffer.IsInvalidated())
    {
      m_pGALCommandEncoder->SetConstantBuffer(binding, boundConstantBuffer.m_hConstantBuffer);
    }
    else
    {
      ezConstantBufferStorageBase* pConstantBufferStorage = nullptr;
      if (TryGetConstantBufferStorage(boundConstantBuffer.m_hConstantBufferStorage, pConstantBufferStorage))
      {
        m_pGALCommandEncoder->SetConstantBuffer(binding, pConstantBufferStorage->GetGALBufferHandle());
      }
      else
      {
        ezLog::Error("Invalid constant buffer storage is bound for slot '{0}'", binding.m_sName);
        m_pGALCommandEncoder->SetConstantBuffer(binding, ezGALBufferHandle());
      }
    }
  }
}

void ezRenderContext::ApplyTextureBindings(const ezGALShader* pShader)
{
  const auto& bindings = pShader->GetBindingMapping();
  for (const ezShaderResourceBinding& binding : bindings)
  {
    const ezUInt64 uiResourceHash = binding.m_sName.GetHash();
    ezGALTextureResourceViewHandle hResourceView;

    if (binding.m_ResourceType == ezGALShaderResourceType::Texture || binding.m_ResourceType == ezGALShaderResourceType::TextureAndSampler)
    {
      switch (binding.m_TextureType)
      {
        case ezGALShaderTextureType::Texture2D:
        case ezGALShaderTextureType::Texture2DArray:
        case ezGALShaderTextureType::Texture2DMS:
        case ezGALShaderTextureType::Texture2DMSArray:
          m_BoundTextures2D.TryGetValue(uiResourceHash, hResourceView);
          m_pGALCommandEncoder->SetResourceView(binding, hResourceView);
          break;
        case ezGALShaderTextureType::Texture3D:
          m_BoundTextures3D.TryGetValue(uiResourceHash, hResourceView);
          m_pGALCommandEncoder->SetResourceView(binding, hResourceView);
          break;
        case ezGALShaderTextureType::TextureCube:
        case ezGALShaderTextureType::TextureCubeArray:
          m_BoundTexturesCube.TryGetValue(uiResourceHash, hResourceView);
          m_pGALCommandEncoder->SetResourceView(binding, hResourceView);
          break;
        case ezGALShaderTextureType::Texture1D:
        case ezGALShaderTextureType::Texture1DArray:
        default:
          EZ_ASSERT_NOT_IMPLEMENTED;
          break;
      }
    }
  }
}

void ezRenderContext::ApplyUAVBindings(const ezGALShader* pShader)
{
  const auto& bindings = pShader->GetBindingMapping();
  for (const ezShaderResourceBinding& binding : bindings)
  {
    auto type = ezGALShaderResourceCategory::MakeFromShaderDescriptorType(binding.m_ResourceType);
    if (type.IsSet(ezGALShaderResourceCategory::TextureUAV))
    {
      const ezUInt64 uiResourceHash = binding.m_sName.GetHash();
      ezGALTextureUnorderedAccessViewHandle hResourceView;
      m_BoundTextureUAVs.TryGetValue(uiResourceHash, hResourceView);
      m_pGALCommandEncoder->SetUnorderedAccessView(binding, hResourceView);
    }
    else if (type.IsSet(ezGALShaderResourceCategory::BufferUAV))
    {
      const ezUInt64 uiResourceHash = binding.m_sName.GetHash();
      ezGALBufferUnorderedAccessViewHandle hResourceView;
      m_BoundBufferUAVs.TryGetValue(uiResourceHash, hResourceView);
      m_pGALCommandEncoder->SetUnorderedAccessView(binding, hResourceView);
    }
  }
}

void ezRenderContext::ApplySamplerBindings(const ezGALShader* pShader)
{
  const auto& bindings = pShader->GetBindingMapping();
  for (const ezShaderResourceBinding& binding : bindings)
  {
    auto type = ezGALShaderResourceCategory::MakeFromShaderDescriptorType(binding.m_ResourceType);
    if (type.IsSet(ezGALShaderResourceCategory::Sampler))
    {
      const ezUInt64 uiResourceHash = binding.m_sName.GetHash();

      ezGALSamplerStateHandle hSamplerState;
      if (!m_BoundSamplers.TryGetValue(uiResourceHash, hSamplerState))
      {
        // Fallback in case no sampler was set.
        hSamplerState = m_hFallbackSampler;
      }

      m_pGALCommandEncoder->SetSamplerState(binding, hSamplerState);
    }
  }
}

void ezRenderContext::ApplyBufferBindings(const ezGALShader* pShader)
{
  const auto& bindings = pShader->GetBindingMapping();
  for (const ezShaderResourceBinding& binding : bindings)
  {
    if (binding.m_ResourceType == ezGALShaderResourceType::TexelBuffer || binding.m_ResourceType == ezGALShaderResourceType::StructuredBuffer)
    {
      const ezUInt64 uiResourceHash = binding.m_sName.GetHash();

      ezGALBufferResourceViewHandle hResourceView;
      m_BoundBuffer.TryGetValue(uiResourceHash, hResourceView);

      m_pGALCommandEncoder->SetResourceView(binding, hResourceView);
    }
  }
}

void ezRenderContext::SetDefaultTextureFilter(ezTextureFilterSetting::Enum filter)
{
  EZ_ASSERT_DEBUG(
    filter >= ezTextureFilterSetting::FixedBilinear && filter <= ezTextureFilterSetting::FixedAnisotropic16x, "Invalid default texture filter");
  filter = ezMath::Clamp(filter, ezTextureFilterSetting::FixedBilinear, ezTextureFilterSetting::FixedAnisotropic16x);

  if (m_DefaultTextureFilter == filter)
    return;

  m_DefaultTextureFilter = filter;
}

ezTextureFilterSetting::Enum ezRenderContext::GetSpecificTextureFilter(ezTextureFilterSetting::Enum configuration) const
{
  if (configuration >= ezTextureFilterSetting::FixedNearest && configuration <= ezTextureFilterSetting::FixedAnisotropic16x)
    return configuration;

  int iFilter = m_DefaultTextureFilter;

  switch (configuration)
  {
    case ezTextureFilterSetting::LowestQuality:
      iFilter -= 2;
      break;
    case ezTextureFilterSetting::LowQuality:
      iFilter -= 1;
      break;
    case ezTextureFilterSetting::HighQuality:
      iFilter += 1;
      break;
    case ezTextureFilterSetting::HighestQuality:
      iFilter += 2;
      break;
    default:
      break;
  }

  iFilter = ezMath::Clamp<int>(iFilter, ezTextureFilterSetting::FixedBilinear, ezTextureFilterSetting::FixedAnisotropic16x);

  return (ezTextureFilterSetting::Enum)iFilter;
}

void ezRenderContext::SetAllowAsyncShaderLoading(bool bAllow)
{
  m_bAllowAsyncShaderLoading = bAllow;
}

bool ezRenderContext::GetAllowAsyncShaderLoading()
{
  return m_bAllowAsyncShaderLoading;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_RenderContext_Implementation_RenderContext);
