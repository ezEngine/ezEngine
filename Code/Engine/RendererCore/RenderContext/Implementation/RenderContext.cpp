#include <RendererCorePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Types/ScopeExit.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Shader/ShaderPermutationResource.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/Texture3DResource.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

ezRenderContext* ezRenderContext::s_DefaultInstance = nullptr;
ezHybridArray<ezRenderContext*, 4> ezRenderContext::s_Instances;

ezMap<ezRenderContext::ShaderVertexDecl, ezGALVertexDeclarationHandle> ezRenderContext::s_GALVertexDeclarations;

ezMutex ezRenderContext::s_ConstantBufferStorageMutex;
ezIdTable<ezConstantBufferStorageId, ezConstantBufferStorageBase*> ezRenderContext::s_ConstantBufferStorageTable;
ezMap<ezUInt32, ezDynamicArray<ezConstantBufferStorageBase*>> ezRenderContext::s_FreeConstantBufferStorage;

ezGALSamplerStateHandle ezRenderContext::s_hDefaultSamplerStates[4];

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, RendererContext)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "Foundation",
  "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
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
  if (s_DefaultInstance == nullptr)
    s_DefaultInstance = CreateInstance();

  return s_DefaultInstance;
}

ezRenderContext* ezRenderContext::CreateInstance()
{
  return EZ_DEFAULT_NEW(ezRenderContext);
}

void ezRenderContext::DestroyInstance(ezRenderContext* pRenderer)
{
  EZ_DEFAULT_DELETE(pRenderer);
}

ezRenderContext::ezRenderContext()
{
  if (s_DefaultInstance == nullptr)
  {
    SetGALContext(ezGALDevice::GetDefaultDevice()->GetPrimaryContext()); // set up with the default device
    s_DefaultInstance = this;
  }

  s_Instances.PushBack(this);

  m_StateFlags = ezRenderContextFlags::AllStatesInvalid;
  m_Topology = ezGALPrimitiveTopology::ENUM_COUNT; // Set to something invalid
  m_uiMeshBufferPrimitiveCount = 0;
  m_DefaultTextureFilter = ezTextureFilterSetting::FixedAnisotropic4x;
  m_bAllowAsyncShaderLoading = false;

  m_hGlobalConstantBufferStorage = CreateConstantBufferStorage<ezGlobalConstants>();

  ezRenderWorld::GetRenderEvent().AddEventHandler(ezMakeDelegate(&ezRenderContext::OnRenderEvent, this));

  ResetContextState();
}

ezRenderContext::~ezRenderContext()
{
  ezRenderWorld::GetRenderEvent().RemoveEventHandler(ezMakeDelegate(&ezRenderContext::OnRenderEvent, this));

  DeleteConstantBufferStorage(m_hGlobalConstantBufferStorage);

  if (s_DefaultInstance == this)
    s_DefaultInstance = nullptr;

  s_Instances.RemoveAndSwap(this);
}

void ezRenderContext::SetGALContext(ezGALContext* pContext)
{
  m_pGALContext = pContext;
}

ezRenderContext::Statistics ezRenderContext::GetAndResetStatistics()
{
  ezRenderContext::Statistics ret = m_Statistics;
  ret.Reset();

  return ret;
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
    BindTexture2D(sSlotName, ezGALResourceViewHandle());
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
    BindTexture3D(sSlotName, ezGALResourceViewHandle());
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
    BindTextureCube(sSlotName, ezGALResourceViewHandle());
  }
}

void ezRenderContext::BindTexture2D(const ezTempHashedString& sSlotName, ezGALResourceViewHandle hResourceView)
{
  ezGALResourceViewHandle* pOldResourceView = nullptr;
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

void ezRenderContext::BindTexture3D(const ezTempHashedString& sSlotName, ezGALResourceViewHandle hResourceView)
{
  ezGALResourceViewHandle* pOldResourceView = nullptr;
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

void ezRenderContext::BindTextureCube(const ezTempHashedString& sSlotName, ezGALResourceViewHandle hResourceView)
{
  ezGALResourceViewHandle* pOldResourceView = nullptr;
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

void ezRenderContext::BindUAV(const ezTempHashedString& sSlotName, ezGALUnorderedAccessViewHandle hUnorderedAccessView)
{
  ezGALUnorderedAccessViewHandle* pOldResourceView = nullptr;
  if (m_BoundUAVs.TryGetValue(sSlotName.GetHash(), pOldResourceView))
  {
    if (*pOldResourceView == hUnorderedAccessView)
      return;

    *pOldResourceView = hUnorderedAccessView;
  }
  else
  {
    m_BoundUAVs.Insert(sSlotName.GetHash(), hUnorderedAccessView);
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

void ezRenderContext::BindBuffer(const ezTempHashedString& sSlotName, ezGALResourceViewHandle hResourceView)
{
  ezGALResourceViewHandle* pOldResourceView = nullptr;
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

void ezRenderContext::BindShader(const ezShaderResourceHandle& hShader, ezBitflags<ezShaderBindFlags> flags)
{
  m_hMaterial.Invalidate();
  m_StateFlags.Remove(ezRenderContextFlags::MaterialBindingChanged);

  BindShaderInternal(hShader, flags);
}

void ezRenderContext::BindMeshBuffer(const ezMeshBufferResourceHandle& hMeshBuffer)
{
  ezResourceLock<ezMeshBufferResource> pMeshBuffer(hMeshBuffer, ezResourceAcquireMode::AllowLoadingFallback);
  BindMeshBuffer(pMeshBuffer->GetVertexBuffer(), pMeshBuffer->GetIndexBuffer(), &(pMeshBuffer->GetVertexDeclaration()),
    pMeshBuffer->GetTopology(), pMeshBuffer->GetPrimitiveCount());
}

void ezRenderContext::BindMeshBuffer(ezGALBufferHandle hVertexBuffer, ezGALBufferHandle hIndexBuffer,
  const ezVertexDeclarationInfo* pVertexDeclarationInfo, ezGALPrimitiveTopology::Enum topology, ezUInt32 uiPrimitiveCount)
{
  if (m_hVertexBuffer == hVertexBuffer && m_hIndexBuffer == hIndexBuffer && m_pVertexDeclarationInfo == pVertexDeclarationInfo &&
      m_Topology == topology && m_uiMeshBufferPrimitiveCount == uiPrimitiveCount)
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

  if (m_Topology != topology)
  {
    m_Topology = topology;

    ezTempHashedString sTopologies[ezGALPrimitiveTopology::ENUM_COUNT] = {
      ezTempHashedString("TOPOLOGY_POINTS"), ezTempHashedString("TOPOLOGY_LINES"), ezTempHashedString("TOPOLOGY_TRIANGLES")};

    SetShaderPermutationVariable("TOPOLOGY", sTopologies[m_Topology]);
  }

  m_hVertexBuffer = hVertexBuffer;
  m_hIndexBuffer = hIndexBuffer;
  m_pVertexDeclarationInfo = pVertexDeclarationInfo;
  m_uiMeshBufferPrimitiveCount = uiPrimitiveCount;

  m_StateFlags.Add(ezRenderContextFlags::MeshBufferBindingChanged);
}

ezResult ezRenderContext::DrawMeshBuffer(ezUInt32 uiPrimitiveCount, ezUInt32 uiFirstPrimitive, ezUInt32 uiInstanceCount)
{
  if (ApplyContextStates().Failed() || uiPrimitiveCount == 0 || uiInstanceCount == 0)
  {
    m_Statistics.m_uiFailedDrawcalls++;
    return EZ_FAILURE;
  }

  EZ_ASSERT_DEV(uiFirstPrimitive < m_uiMeshBufferPrimitiveCount,
    "Invalid primitive range: first primitive ({0}) can't be larger than number of primitives ({1})", uiFirstPrimitive, uiPrimitiveCount);

  uiPrimitiveCount = ezMath::Min(uiPrimitiveCount, m_uiMeshBufferPrimitiveCount - uiFirstPrimitive);
  EZ_ASSERT_DEV(uiPrimitiveCount > 0, "Invalid primitive range: number of primitives can't be zero.");

  const ezUInt32 uiVertsPerPrimitive = ezGALPrimitiveTopology::VerticesPerPrimitive(m_pGALContext->GetPrimitiveTopology());

  uiPrimitiveCount *= uiVertsPerPrimitive;
  uiFirstPrimitive *= uiVertsPerPrimitive;

  if (uiInstanceCount > 1)
  {
    if (!m_hIndexBuffer.IsInvalidated())
    {
      m_pGALContext->DrawIndexedInstanced(uiPrimitiveCount, uiInstanceCount, uiFirstPrimitive);
    }
    else
    {
      m_pGALContext->DrawInstanced(uiPrimitiveCount, uiInstanceCount, uiFirstPrimitive);
    }
  }
  else
  {
    if (!m_hIndexBuffer.IsInvalidated())
    {
      m_pGALContext->DrawIndexed(uiPrimitiveCount, uiFirstPrimitive);
    }
    else
    {
      m_pGALContext->Draw(uiPrimitiveCount, uiFirstPrimitive);
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

  m_pGALContext->Dispatch(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);

  return EZ_SUCCESS;
}

ezResult ezRenderContext::ApplyContextStates(bool bForce)
{
  // First apply material state since this can modify all other states.
  // Note ApplyMaterialState only returns a valid material pointer if the constant buffer of this material needs to be updated.
  // This needs to be done once we have determined the correct shader permutation.
  ezMaterialResource* pMaterial = nullptr;
  EZ_SCOPE_EXIT(if (pMaterial != nullptr) { ezResourceManager::EndAcquireResource(pMaterial); });

  if (bForce || m_StateFlags.IsSet(ezRenderContextFlags::MaterialBindingChanged))
  {
    pMaterial = ApplyMaterialState();

    m_StateFlags.Remove(ezRenderContextFlags::MaterialBindingChanged);
  }

  ezShaderPermutationResource* pShaderPermutation = nullptr;
  EZ_SCOPE_EXIT(if (pShaderPermutation != nullptr) { ezResourceManager::EndAcquireResource(pShaderPermutation); });

  bool bRebuildVertexDeclaration =
    m_StateFlags.IsAnySet(ezRenderContextFlags::ShaderStateChanged | ezRenderContextFlags::MeshBufferBindingChanged);

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
    if ((bForce || m_StateFlags.IsAnySet(ezRenderContextFlags::TextureBindingChanged | ezRenderContextFlags::UAVBindingChanged |
                                         ezRenderContextFlags::SamplerBindingChanged | ezRenderContextFlags::BufferBindingChanged |
                                         ezRenderContextFlags::ConstantBufferBindingChanged)))
    {
      if (pShaderPermutation == nullptr)
        pShaderPermutation = ezResourceManager::BeginAcquireResource(m_hActiveShaderPermutation, ezResourceAcquireMode::BlockTillLoaded);
    }


    if (bForce || m_StateFlags.IsSet(ezRenderContextFlags::UAVBindingChanged))
    {
      // RWTextures/UAV are usually only suppoprted in compute and pixel shader.
      if (auto pBin = pShaderPermutation->GetShaderStageBinary(ezGALShaderStage::ComputeShader))
      {
        ApplyUAVBindings(pBin);
      }
      if (auto pBin = pShaderPermutation->GetShaderStageBinary(ezGALShaderStage::PixelShader))
      {
        ApplyUAVBindings(pBin);
      }

      m_StateFlags.Remove(ezRenderContextFlags::UAVBindingChanged);
    }

    if (bForce || m_StateFlags.IsSet(ezRenderContextFlags::TextureBindingChanged))
    {
      for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
      {
        if (auto pBin = pShaderPermutation->GetShaderStageBinary((ezGALShaderStage::Enum)stage))
        {
          ApplyTextureBindings((ezGALShaderStage::Enum)stage, pBin);
        }
      }

      m_StateFlags.Remove(ezRenderContextFlags::TextureBindingChanged);
    }

    if (bForce || m_StateFlags.IsSet(ezRenderContextFlags::SamplerBindingChanged))
    {
      for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
      {
        if (auto pBin = pShaderPermutation->GetShaderStageBinary((ezGALShaderStage::Enum)stage))
        {
          ApplySamplerBindings((ezGALShaderStage::Enum)stage, pBin);
        }
      }

      m_StateFlags.Remove(ezRenderContextFlags::SamplerBindingChanged);
    }

    if (bForce || m_StateFlags.IsSet(ezRenderContextFlags::BufferBindingChanged))
    {
      for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
      {
        if (auto pBin = pShaderPermutation->GetShaderStageBinary((ezGALShaderStage::Enum)stage))
        {
          ApplyBufferBindings((ezGALShaderStage::Enum)stage, pBin);
        }
      }

      m_StateFlags.Remove(ezRenderContextFlags::BufferBindingChanged);
    }

    if (pMaterial != nullptr)
    {
      pMaterial->UpdateConstantBuffer(pShaderPermutation);
      BindConstantBuffer("ezMaterialConstants", pMaterial->m_hConstantBufferStorage);
    }

    UploadConstants();

    if (bForce || m_StateFlags.IsSet(ezRenderContextFlags::ConstantBufferBindingChanged))
    {
      for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
      {
        if (auto pBin = pShaderPermutation->GetShaderStageBinary((ezGALShaderStage::Enum)stage))
        {
          ApplyConstantBufferBindings(pBin);
        }
      }

      m_StateFlags.Remove(ezRenderContextFlags::ConstantBufferBindingChanged);
    }
  }

  if (bForce || bRebuildVertexDeclaration)
  {
    if (m_hActiveGALShader.IsInvalidated())
      return EZ_FAILURE;

    if (bForce || m_StateFlags.IsSet(ezRenderContextFlags::MeshBufferBindingChanged))
    {
      m_pGALContext->SetPrimitiveTopology(m_Topology);
      m_pGALContext->SetVertexBuffer(0, m_hVertexBuffer);

      if (!m_hIndexBuffer.IsInvalidated())
        m_pGALContext->SetIndexBuffer(m_hIndexBuffer);
    }

    ezGALVertexDeclarationHandle hVertexDeclaration;
    if (m_pVertexDeclarationInfo != nullptr &&
        BuildVertexDeclaration(m_hActiveGALShader, *m_pVertexDeclarationInfo, hVertexDeclaration).Failed())
      return EZ_FAILURE;

    // If there is a vertex buffer we need a valid vertex declaration as well.
    if (!m_hVertexBuffer.IsInvalidated() && hVertexDeclaration.IsInvalidated())
      return EZ_FAILURE;

    m_pGALContext->SetVertexDeclaration(hVertexDeclaration);

    m_StateFlags.Remove(ezRenderContextFlags::MeshBufferBindingChanged);
  }

  return EZ_SUCCESS;
}

void ezRenderContext::ResetContextState()
{
  m_StateFlags = ezRenderContextFlags::AllStatesInvalid;

  m_hActiveShader.Invalidate();
  m_hActiveGALShader.Invalidate();

  m_hNewMaterial.Invalidate();
  m_hMaterial.Invalidate();

  m_hActiveShaderPermutation.Invalidate();

  m_hVertexBuffer.Invalidate();
  m_hIndexBuffer.Invalidate();
  m_pVertexDeclarationInfo = nullptr;
  m_Topology = ezGALPrimitiveTopology::ENUM_COUNT; // Set to something invalid
  m_uiMeshBufferPrimitiveCount = 0;

  m_BoundTextures2D.Clear();
  m_BoundTextures3D.Clear();
  m_BoundTexturesCube.Clear();
  m_BoundBuffer.Clear();

  m_BoundSamplers.Clear();
  m_BoundSamplers.Insert(ezTempHashedString::ComputeHash("LinearSampler"), GetDefaultSamplerState(ezDefaultSamplerFlags::LinearFiltering));
  m_BoundSamplers.Insert(ezTempHashedString::ComputeHash("LinearClampSampler"),
    GetDefaultSamplerState(ezDefaultSamplerFlags::LinearFiltering | ezDefaultSamplerFlags::Clamp));
  m_BoundSamplers.Insert(ezTempHashedString::ComputeHash("PointSampler"), GetDefaultSamplerState(ezDefaultSamplerFlags::PointFiltering));
  m_BoundSamplers.Insert(ezTempHashedString::ComputeHash("PointClampSampler"),
    GetDefaultSamplerState(ezDefaultSamplerFlags::PointFiltering | ezDefaultSamplerFlags::Clamp));

  m_BoundUAVs.Clear();
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

void ezRenderContext::SetViewportAndRenderTargetSetup(const ezRectFloat& viewport, const ezGALRenderTargetSetup& renderTargetSetup)
{
  ezGALMSAASampleCount::Enum msaaSampleCount = ezGALMSAASampleCount::None;

  ezGALRenderTargetViewHandle hRTV = renderTargetSetup.GetRenderTarget(0);
  if (hRTV.IsInvalidated())
  {
    hRTV = renderTargetSetup.GetDepthStencilTarget();
  }

  if (const ezGALRenderTargetView* pRTV = ezGALDevice::GetDefaultDevice()->GetRenderTargetView(hRTV))
  {
    msaaSampleCount = pRTV->GetTexture()->GetDescription().m_SampleCount;
  }

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

  m_pGALContext->SetRenderTargetSetup(renderTargetSetup);
  m_pGALContext->SetViewport(viewport);
}

// static
ezConstantBufferStorageHandle ezRenderContext::CreateConstantBufferStorage(
  ezUInt32 uiSizeInBytes, ezConstantBufferStorageBase*& out_pStorage)
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
ezGALSamplerStateHandle ezRenderContext::GetDefaultSamplerState(ezBitflags<ezDefaultSamplerFlags> flags)
{
  ezUInt32 uiSamplerStateIndex = flags.GetValue();
  EZ_ASSERT_DEV(uiSamplerStateIndex < EZ_ARRAY_SIZE(s_hDefaultSamplerStates), "");

  if (s_hDefaultSamplerStates[uiSamplerStateIndex].IsInvalidated())
  {
    ezGALSamplerStateCreationDescription desc;
    desc.m_MinFilter = flags.IsSet(ezDefaultSamplerFlags::LinearFiltering) ? ezGALTextureFilterMode::Linear : ezGALTextureFilterMode::Point;
    desc.m_MagFilter = flags.IsSet(ezDefaultSamplerFlags::LinearFiltering) ? ezGALTextureFilterMode::Linear : ezGALTextureFilterMode::Point;
    desc.m_MipFilter = flags.IsSet(ezDefaultSamplerFlags::LinearFiltering) ? ezGALTextureFilterMode::Linear : ezGALTextureFilterMode::Point;

    desc.m_AddressU = flags.IsSet(ezDefaultSamplerFlags::Clamp) ? ezImageAddressMode::Clamp : ezImageAddressMode::Repeat;
    desc.m_AddressV = flags.IsSet(ezDefaultSamplerFlags::Clamp) ? ezImageAddressMode::Clamp : ezImageAddressMode::Repeat;
    desc.m_AddressW = flags.IsSet(ezDefaultSamplerFlags::Clamp) ? ezImageAddressMode::Clamp : ezImageAddressMode::Repeat;

    s_hDefaultSamplerStates[uiSamplerStateIndex] = ezGALDevice::GetDefaultDevice()->CreateSamplerState(desc);
  }

  return s_hDefaultSamplerStates[uiSamplerStateIndex];
}

// private functions
//////////////////////////////////////////////////////////////////////////

// static
void ezRenderContext::OnEngineShutdown()
{
  ezShaderStageBinary::OnEngineShutdown();

  for (auto rc : s_Instances)
    EZ_DEFAULT_DELETE(rc);

  s_Instances.Clear();

  // Cleanup sampler states
  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(s_hDefaultSamplerStates); ++i)
  {
    if (!s_hDefaultSamplerStates[i].IsInvalidated())
    {
      ezGALDevice::GetDefaultDevice()->DestroySamplerState(s_hDefaultSamplerStates[i]);
      s_hDefaultSamplerStates[i].Invalidate();
    }
  }

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

void ezRenderContext::OnRenderEvent(const ezRenderWorldRenderEvent& e)
{
  if (e.m_Type == ezRenderWorldRenderEvent::Type::EndRender)
  {
    ResetContextState();
  }
}

// static
ezResult ezRenderContext::BuildVertexDeclaration(
  ezGALShaderHandle hShader, const ezVertexDeclarationInfo& decl, ezGALVertexDeclarationHandle& out_Declaration)
{
  ShaderVertexDecl svd;
  svd.m_hShader = hShader;
  svd.m_uiVertexDeclarationHash = decl.m_uiHash;

  bool bExisted = false;
  auto it = s_GALVertexDeclarations.FindOrAdd(svd, &bExisted);

  if (!bExisted)
  {
    const ezGALShader* pShader = ezGALDevice::GetDefaultDevice()->GetShader(hShader);

    auto pBytecode = pShader->GetDescription().m_ByteCodes[ezGALShaderStage::VertexShader];

    ezGALVertexDeclarationCreationDescription vd;
    vd.m_hShader = hShader;

    for (ezUInt32 slot = 0; slot < decl.m_VertexStreams.GetCount(); ++slot)
    {
      auto& stream = decl.m_VertexStreams[slot];

      // stream.m_Format
      ezGALVertexAttribute gal;
      gal.m_bInstanceData = false;
      gal.m_eFormat = stream.m_Format;
      gal.m_eSemantic = stream.m_Semantic;
      gal.m_uiOffset = stream.m_uiOffset;
      gal.m_uiVertexBufferSlot = 0;
      vd.m_VertexAttributes.PushBack(gal);
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
      pConstantBufferStorage->UploadData(m_pGALContext);
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

  m_StateFlags.Add(ezRenderContextFlags::TextureBindingChanged | ezRenderContextFlags::SamplerBindingChanged |
                   ezRenderContextFlags::BufferBindingChanged | ezRenderContextFlags::ConstantBufferBindingChanged);

  if (!m_hActiveShader.IsValid())
    return nullptr;

  m_hActiveShaderPermutation =
    ezShaderManager::PreloadSinglePermutation(m_hActiveShader, m_PermutationVariables, m_bAllowAsyncShaderLoading);

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
  EZ_ASSERT_DEV(!m_hActiveGALShader.IsInvalidated(), "Invalid GAL Shader handle.");

  m_pGALContext->SetShader(m_hActiveGALShader);

  // Set render state from shader
  if (!m_ShaderBindFlags.IsSet(ezShaderBindFlags::NoBlendState))
    m_pGALContext->SetBlendState(pShaderPermutation->GetBlendState());

  if (!m_ShaderBindFlags.IsSet(ezShaderBindFlags::NoRasterizerState))
    m_pGALContext->SetRasterizerState(pShaderPermutation->GetRasterizerState());

  if (!m_ShaderBindFlags.IsSet(ezShaderBindFlags::NoDepthStencilState))
    m_pGALContext->SetDepthStencilState(pShaderPermutation->GetDepthStencilState());

  return pShaderPermutation;
}

ezMaterialResource* ezRenderContext::ApplyMaterialState()
{
  if (!m_hNewMaterial.IsValid())
  {
    BindShaderInternal(ezShaderResourceHandle(), ezShaderBindFlags::Default);
    return nullptr;
  }

  // check whether material has been modified
  ezMaterialResource* pMaterial = ezResourceManager::BeginAcquireResource(m_hNewMaterial, ezResourceAcquireMode::AllowLoadingFallback);

  if (m_hNewMaterial != m_hMaterial || pMaterial->IsModified())
  {
    auto pCachedValues = pMaterial->GetOrUpdateCachedValues();

    BindShaderInternal(pCachedValues->m_hShader, ezShaderBindFlags::Default);

    if (!pMaterial->m_hConstantBufferStorage.IsInvalidated())
    {
      BindConstantBuffer("ezMaterialConstants", pMaterial->m_hConstantBufferStorage);
    }

    for (auto it = pCachedValues->m_PermutationVars.GetIterator(); it.IsValid(); ++it)
    {
      SetShaderPermutationVariableInternal(it.Key(), it.Value());
    }

    for (auto it = pCachedValues->m_Texture2DBindings.GetIterator(); it.IsValid(); ++it)
    {
      BindTexture2D(it.Key(), it.Value());
    }

    for (auto it = pCachedValues->m_TextureCubeBindings.GetIterator(); it.IsValid(); ++it)
    {
      BindTextureCube(it.Key(), it.Value());
    }

    m_hMaterial = m_hNewMaterial;
  }

  // The material needs its constant buffer updated.
  // Thus we keep it acquired until we have the correct shader permutation for the constant buffer layout.
  if (pMaterial->AreConstantsModified())
  {
    m_StateFlags.Add(ezRenderContextFlags::ConstantBufferBindingChanged);

    return pMaterial;
  }

  ezResourceManager::EndAcquireResource(pMaterial);
  return nullptr;
}

void ezRenderContext::ApplyConstantBufferBindings(const ezShaderStageBinary* pBinary)
{
  for (const auto& binding : pBinary->m_ShaderResourceBindings)
  {
    if (binding.m_Type != ezShaderResourceBinding::ConstantBuffer)
      continue;

    const ezUInt32 uiResourceHash = binding.m_sName.GetHash();

    BoundConstantBuffer boundConstantBuffer;
    if (!m_BoundConstantBuffers.TryGetValue(uiResourceHash, boundConstantBuffer))
    {
      ezLog::Error("No resource is bound for constant buffer slot '{0}'", binding.m_sName);
      m_pGALContext->SetConstantBuffer(binding.m_iSlot, ezGALBufferHandle());
      continue;
    }

    if (!boundConstantBuffer.m_hConstantBuffer.IsInvalidated())
    {
      m_pGALContext->SetConstantBuffer(binding.m_iSlot, boundConstantBuffer.m_hConstantBuffer);
    }
    else
    {
      ezConstantBufferStorageBase* pConstantBufferStorage = nullptr;
      if (TryGetConstantBufferStorage(boundConstantBuffer.m_hConstantBufferStorage, pConstantBufferStorage))
      {
        m_pGALContext->SetConstantBuffer(binding.m_iSlot, pConstantBufferStorage->GetGALBufferHandle());
      }
      else
      {
        ezLog::Error("Invalid constant buffer storage is bound for slot '{0}'", binding.m_sName);
        m_pGALContext->SetConstantBuffer(binding.m_iSlot, ezGALBufferHandle());
      }
    }
  }
}

void ezRenderContext::ApplyTextureBindings(ezGALShaderStage::Enum stage, const ezShaderStageBinary* pBinary)
{
  for (const auto& binding : pBinary->m_ShaderResourceBindings)
  {
    // we currently only support 2D and cube textures

    const ezUInt32 uiResourceHash = binding.m_sName.GetHash();
    ezGALResourceViewHandle hResourceView;

    if (binding.m_Type >= ezShaderResourceBinding::Texture2D && binding.m_Type <= ezShaderResourceBinding::Texture2DMSArray)
    {
      m_BoundTextures2D.TryGetValue(uiResourceHash, hResourceView);
      m_pGALContext->SetResourceView(stage, binding.m_iSlot, hResourceView);
    }

    if (binding.m_Type == ezShaderResourceBinding::Texture3D)
    {
      m_BoundTextures3D.TryGetValue(uiResourceHash, hResourceView);
      m_pGALContext->SetResourceView(stage, binding.m_iSlot, hResourceView);
    }

    if (binding.m_Type >= ezShaderResourceBinding::TextureCube && binding.m_Type <= ezShaderResourceBinding::TextureCubeArray)
    {
      m_BoundTexturesCube.TryGetValue(uiResourceHash, hResourceView);
      m_pGALContext->SetResourceView(stage, binding.m_iSlot, hResourceView);
    }
  }
}

void ezRenderContext::ApplyUAVBindings(const ezShaderStageBinary* pBinary)
{
  for (const auto& binding : pBinary->m_ShaderResourceBindings)
  {
    if (binding.m_Type < ezShaderResourceBinding::RWTexture1D || binding.m_Type > ezShaderResourceBinding::RWStructuredBufferWithCounter)
      continue;

    const ezUInt32 uiResourceHash = binding.m_sName.GetHash();

    ezGALUnorderedAccessViewHandle hResourceView;
    m_BoundUAVs.TryGetValue(uiResourceHash, hResourceView);

    m_pGALContext->SetUnorderedAccessView(binding.m_iSlot, hResourceView);
  }
}

void ezRenderContext::ApplySamplerBindings(ezGALShaderStage::Enum stage, const ezShaderStageBinary* pBinary)
{
  for (const auto& binding : pBinary->m_ShaderResourceBindings)
  {
    if (binding.m_Type != ezShaderResourceBinding::Sampler)
      continue;

    const ezUInt32 uiResourceHash = binding.m_sName.GetHash();

    ezGALSamplerStateHandle hSamplerState;
    if (!m_BoundSamplers.TryGetValue(uiResourceHash, hSamplerState))
    {
      hSamplerState = GetDefaultSamplerState(ezDefaultSamplerFlags::LinearFiltering); // Bind a default state to avoid DX11 errors.
    }

    m_pGALContext->SetSamplerState(stage, binding.m_iSlot, hSamplerState);
  }
}

void ezRenderContext::ApplyBufferBindings(ezGALShaderStage::Enum stage, const ezShaderStageBinary* pBinary)
{
  for (const auto& binding : pBinary->m_ShaderResourceBindings)
  {
    if (binding.m_Type != ezShaderResourceBinding::GenericBuffer)
      continue;

    const ezUInt32 uiResourceHash = binding.m_sName.GetHash();

    ezGALResourceViewHandle hResourceView;
    m_BoundBuffer.TryGetValue(uiResourceHash, hResourceView);

    m_pGALContext->SetResourceView(stage, binding.m_iSlot, hResourceView);
  }
}

void ezRenderContext::SetDefaultTextureFilter(ezTextureFilterSetting::Enum filter)
{
  EZ_ASSERT_DEBUG(filter >= ezTextureFilterSetting::FixedBilinear && filter <= ezTextureFilterSetting::FixedAnisotropic16x,
    "Invalid default texture filter");
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

EZ_STATICLINK_FILE(RendererCore, RendererCore_RenderContext_Implementation_RenderContext);
