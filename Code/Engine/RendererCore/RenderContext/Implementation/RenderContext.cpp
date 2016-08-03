#include <RendererCore/PCH.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderLoop/RenderLoop.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Shader/ShaderPermutationResource.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/Textures/TextureResource.h>
#include <Foundation/Types/ScopeExit.h>

ezRenderContext* ezRenderContext::s_DefaultInstance = nullptr;
ezHybridArray<ezRenderContext*, 4> ezRenderContext::s_Instances;

ezMap<ezRenderContext::ShaderVertexDecl, ezGALVertexDeclarationHandle> ezRenderContext::s_GALVertexDeclarations;

ezMutex ezRenderContext::s_ConstantBufferStorageMutex;
ezIdTable<ezConstantBufferStorageId, ezConstantBufferStorageBase*> ezRenderContext::s_ConstantBufferStorageTable;
ezMap<ezUInt32, ezDynamicArray<ezConstantBufferStorageBase*>> ezRenderContext::s_FreeConstantBufferStorage;

ezGALSamplerStateHandle ezRenderContext::s_hDefaultSamplerStates[4];

EZ_BEGIN_SUBSYSTEM_DECLARATION(Graphics, RendererContext)

BEGIN_SUBSYSTEM_DEPENDENCIES
"Foundation",
"Core"
END_SUBSYSTEM_DEPENDENCIES

ON_CORE_STARTUP
{
}

ON_CORE_SHUTDOWN
{
}

ON_ENGINE_STARTUP
{
}

ON_ENGINE_SHUTDOWN
{
  ezRenderContext::OnEngineShutdown();
}

EZ_END_SUBSYSTEM_DECLARATION

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
  m_Topology = ezGALPrimitiveTopology::Triangles;
  m_uiMeshBufferPrimitiveCount = 0;

  m_hGlobalConstantBufferStorage = CreateConstantBufferStorage<GlobalConstants>();

  ezRenderLoop::s_EndFrameEvent.AddEventHandler(ezMakeDelegate(&ezRenderContext::OnEndFrame, this));
}

ezRenderContext::~ezRenderContext()
{
  ezRenderLoop::s_EndFrameEvent.RemoveEventHandler(ezMakeDelegate(&ezRenderContext::OnEndFrame, this));

  DeleteConstantBufferStorage(m_hGlobalConstantBufferStorage);

  if (s_DefaultInstance == this)
    s_DefaultInstance = nullptr;

  s_Instances.RemoveSwap(this);
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
  ezHashedString* pOldValue;
  bool bExisted = m_PermutationVariables.TryGetValue(sHashedName, pOldValue);

  ezHashedString sName;
  ezHashedString sValue;
  if (!ezShaderManager::IsPermutationValueAllowed(szName, sHashedName, sTempValue, sName, sValue))
  {
    return;
  }

  if (!bExisted || *pOldValue != sValue)
  {
    m_PermutationVariables.Insert(sName, sValue);
    m_StateFlags.Add(ezRenderContextFlags::ShaderStateChanged);
  }
}

void ezRenderContext::SetShaderPermutationVariable(const ezHashedString& sName, const ezHashedString& sValue)
{
  ezHashedString* pOldValue;
  bool bExisted = m_PermutationVariables.TryGetValue(sName, pOldValue);

  if (!ezShaderManager::IsPermutationValueAllowed(sName, sValue))
  {
    return;
  }

  if (!bExisted || *pOldValue != sValue)
  {
    m_PermutationVariables.Insert(sName, sValue);
    m_StateFlags.Add(ezRenderContextFlags::ShaderStateChanged);
  }
}


void ezRenderContext::BindMaterial(const ezMaterialResourceHandle& hMaterial)
{
  // Don't set m_hMaterial directly since we first need to check whether the material has been modified in the mean time.
  m_hNewMaterial = hMaterial;
  m_StateFlags.Add(ezRenderContextFlags::MaterialBindingChanged);
}

void ezRenderContext::BindTexture(ezGALShaderStage::Enum stage, const ezTempHashedString& sSlotName, const ezTextureResourceHandle& hTexture)
{
  ezResourceLock<ezTextureResource> pTexture(hTexture, ezResourceAcquireMode::AllowFallback);
  BindTexture(stage, sSlotName, ezGALDevice::GetDefaultDevice()->GetDefaultResourceView(pTexture->GetGALTexture()), pTexture->GetGALSamplerState());
}

void ezRenderContext::BindTexture(ezGALShaderStage::Enum stage, const ezTempHashedString& sSlotName, ezGALResourceViewHandle hResourceView, ezGALSamplerStateHandle hSamplerState)
{
  TextureViewSampler* pTextureViewSampler = nullptr;
  if (m_BoundTextures[stage].TryGetValue(sSlotName.GetHash(), pTextureViewSampler))
  {
    if (pTextureViewSampler->m_hResourceView == hResourceView && pTextureViewSampler->m_hSamplerState == hSamplerState)
      return;

    pTextureViewSampler->m_hResourceView = hResourceView;
    pTextureViewSampler->m_hSamplerState = hSamplerState;
  }
  else
  {
    m_BoundTextures[stage].Insert(sSlotName.GetHash(), TextureViewSampler(hResourceView, hSamplerState));
  }

  m_StateFlags.Add(ezRenderContextFlags::TextureBindingChanged);
}


void ezRenderContext::BindBuffer(ezGALShaderStage::Enum stage, const ezTempHashedString& sSlotName, ezGALResourceViewHandle hResourceView)
{
  ezGALResourceViewHandle* pOldResourceView = nullptr;
  if (m_BoundBuffer[stage].TryGetValue(sSlotName.GetHash(), pOldResourceView))
  {
    if (*pOldResourceView == hResourceView)
      return;

    *pOldResourceView = hResourceView;
  }
  else
  {
    m_BoundBuffer[stage].Insert(sSlotName.GetHash(), hResourceView);
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

void ezRenderContext::BindShader(ezShaderResourceHandle hShader, ezBitflags<ezShaderBindFlags> flags)
{
  if (flags.IsAnySet(ezShaderBindFlags::ForceRebind) || m_hActiveShader != hShader)
  {
    m_hMaterial.Invalidate();
    m_StateFlags.Remove(ezRenderContextFlags::MaterialBindingChanged);
  }

  BindShaderInternal(hShader, flags);
}

void ezRenderContext::BindMeshBuffer(const ezMeshBufferResourceHandle& hMeshBuffer)
{
  ezResourceLock<ezMeshBufferResource> pMeshBuffer(hMeshBuffer);
  BindMeshBuffer(pMeshBuffer->GetVertexBuffer(), pMeshBuffer->GetIndexBuffer(), &(pMeshBuffer->GetVertexDeclaration()), pMeshBuffer->GetTopology(), pMeshBuffer->GetPrimitiveCount());
}

void ezRenderContext::BindMeshBuffer(ezGALBufferHandle hVertexBuffer, ezGALBufferHandle hIndexBuffer, const ezVertexDeclarationInfo* pVertexDeclarationInfo,
  ezGALPrimitiveTopology::Enum topology, ezUInt32 uiPrimitiveCount)
{
  if (m_hVertexBuffer == hVertexBuffer && m_hIndexBuffer == hIndexBuffer && m_pVertexDeclarationInfo == pVertexDeclarationInfo &&
    m_Topology == topology && m_uiMeshBufferPrimitiveCount == uiPrimitiveCount)
  {
    return;
  }

  m_hVertexBuffer = hVertexBuffer;
  m_hIndexBuffer = hIndexBuffer;
  m_pVertexDeclarationInfo = pVertexDeclarationInfo;
  m_Topology = topology;
  m_uiMeshBufferPrimitiveCount = uiPrimitiveCount;

  m_StateFlags.Add(ezRenderContextFlags::MeshBufferBindingChanged);
}

ezResult ezRenderContext::DrawMeshBuffer(ezUInt32 uiPrimitiveCount, ezUInt32 uiFirstPrimitive, ezUInt32 uiInstanceCount)
{
  if (ApplyContextStates().Failed())
  {
    m_Statistics.m_uiFailedDrawcalls++;
    return EZ_FAILURE;
  }

  EZ_ASSERT_DEV(uiFirstPrimitive < m_uiMeshBufferPrimitiveCount, "Invalid primitive range: first primitive (%d) can't be larger than number of primitives (%d)", uiFirstPrimitive, uiPrimitiveCount);

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
    if ((bForce || m_StateFlags.IsAnySet(ezRenderContextFlags::TextureBindingChanged | ezRenderContextFlags::BufferBindingChanged | ezRenderContextFlags::ConstantBufferBindingChanged)))
    {
      if (pShaderPermutation == nullptr)
        pShaderPermutation = ezResourceManager::BeginAcquireResource(m_hActiveShaderPermutation, ezResourceAcquireMode::NoFallback);
    }

    if (bForce || m_StateFlags.IsSet(ezRenderContextFlags::TextureBindingChanged))
    {
      for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
      {
        if (auto pBin = pShaderPermutation->GetShaderStageBinary((ezGALShaderStage::Enum) stage))
        {
          ApplyTextureBindings((ezGALShaderStage::Enum) stage, pBin);
        }
      }

      m_StateFlags.Remove(ezRenderContextFlags::TextureBindingChanged);
    }

    if (bForce || m_StateFlags.IsSet(ezRenderContextFlags::BufferBindingChanged))
    {
      for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
      {
        if (auto pBin = pShaderPermutation->GetShaderStageBinary((ezGALShaderStage::Enum) stage))
        {
          ApplyBufferBindings((ezGALShaderStage::Enum)stage, pBin);
        }
      }

      m_StateFlags.Remove(ezRenderContextFlags::BufferBindingChanged);
    }

    if (pMaterial != nullptr)
    {
      pMaterial->UpdateConstantBuffer(pShaderPermutation);
      BindConstantBuffer("MaterialConstants", pMaterial->m_hConstantBufferStorage);
    }

    UploadConstants();

    if (bForce || m_StateFlags.IsSet(ezRenderContextFlags::ConstantBufferBindingChanged))
    {
      for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
      {
        if (auto pBin = pShaderPermutation->GetShaderStageBinary((ezGALShaderStage::Enum) stage))
        {
          ApplyConstantBufferBindings(pBin);
        }
      }

      m_StateFlags.Remove(ezRenderContextFlags::ConstantBufferBindingChanged);
    }
  }

  if (bForce || bRebuildVertexDeclaration || m_StateFlags.IsSet(ezRenderContextFlags::MeshBufferBindingChanged))
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
    if (m_pVertexDeclarationInfo != nullptr && BuildVertexDeclaration(m_hActiveGALShader, *m_pVertexDeclarationInfo, hVertexDeclaration).Failed())
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
  m_Topology = ezGALPrimitiveTopology::Triangles;
  m_uiMeshBufferPrimitiveCount = 0;

  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    m_BoundTextures[stage].Clear();
    m_BoundBuffer[stage].Clear();
  }

  m_BoundConstantBuffers.Clear();
}

GlobalConstants& ezRenderContext::WriteGlobalConstants()
{
  ezConstantBufferStorage<GlobalConstants>* pStorage = nullptr;
  EZ_VERIFY(TryGetConstantBufferStorage(m_hGlobalConstantBufferStorage, pStorage), "Invalid Global Constant Storage");
  return pStorage->GetDataForWriting();
}

const GlobalConstants& ezRenderContext::ReadGlobalConstants() const
{
  ezConstantBufferStorage<GlobalConstants>* pStorage = nullptr;
  EZ_VERIFY(TryGetConstantBufferStorage(m_hGlobalConstantBufferStorage, pStorage), "Invalid Global Constant Storage");
  return pStorage->GetDataForReading();
}

//static
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
      storageForSize.RemoveAtSwap(0);
    }
  }

  if (pStorage == nullptr)
  {
    pStorage = EZ_DEFAULT_NEW(ezConstantBufferStorageBase, uiSizeInBytes);
  }

  out_pStorage = pStorage;
  return ezConstantBufferStorageHandle(s_ConstantBufferStorageTable.Insert(pStorage));
}

//static
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

//static
bool ezRenderContext::TryGetConstantBufferStorage(ezConstantBufferStorageHandle hStorage, ezConstantBufferStorageBase*& out_pStorage)
{
  EZ_LOCK(s_ConstantBufferStorageMutex);
  return s_ConstantBufferStorageTable.TryGetValue(hStorage.m_InternalId, out_pStorage);
}

//static
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

    desc.m_AddressU = flags.IsSet(ezDefaultSamplerFlags::Clamp) ? ezGALTextureAddressMode::Clamp : ezGALTextureAddressMode::Wrap;
    desc.m_AddressV = flags.IsSet(ezDefaultSamplerFlags::Clamp) ? ezGALTextureAddressMode::Clamp : ezGALTextureAddressMode::Wrap;
    desc.m_AddressW = flags.IsSet(ezDefaultSamplerFlags::Clamp) ? ezGALTextureAddressMode::Clamp : ezGALTextureAddressMode::Wrap;

    s_hDefaultSamplerStates[uiSamplerStateIndex] = ezGALDevice::GetDefaultDevice()->CreateSamplerState(desc);
  }

  return s_hDefaultSamplerStates[uiSamplerStateIndex];
}

// private functions
//////////////////////////////////////////////////////////////////////////

//static
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

void ezRenderContext::OnEndFrame(ezUInt64)
{
  ResetContextState();
}

//static
ezResult ezRenderContext::BuildVertexDeclaration(ezGALShaderHandle hShader, const ezVertexDeclarationInfo& decl, ezGALVertexDeclarationHandle& out_Declaration)
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

      //stream.m_Format
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
  BindConstantBuffer("GlobalConstants", m_hGlobalConstantBufferStorage);

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


void ezRenderContext::BindShaderInternal(ezShaderResourceHandle hShader, ezBitflags<ezShaderBindFlags> flags)
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

  m_StateFlags.Add(ezRenderContextFlags::TextureBindingChanged | ezRenderContextFlags::BufferBindingChanged | ezRenderContextFlags::ConstantBufferBindingChanged);

  if (!m_hActiveShader.IsValid())
    return nullptr;

  ezResourceLock<ezShaderResource> pShader(m_hActiveShader, ezResourceAcquireMode::NoFallback);

  if (!pShader || !pShader->IsShaderValid())
    return nullptr;

  ezTempHashedString sTopologies[ezGALPrimitiveTopology::ENUM_COUNT] =
  {
    ezTempHashedString("POINTS"),
    ezTempHashedString("LINES"),
    ezTempHashedString("TRIANGLES")
  };

  SetShaderPermutationVariable("TOPOLOGY", sTopologies[m_Topology]);

  m_hActiveShaderPermutation = ezShaderManager::PreloadSinglePermutation(m_hActiveShader, m_PermutationVariables, ezTime::Seconds(0.0));

  if (!m_hActiveShaderPermutation.IsValid())
    return nullptr;

  ezShaderPermutationResource* pShaderPermutation = ezResourceManager::BeginAcquireResource(m_hActiveShaderPermutation, ezResourceAcquireMode::NoFallback);

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
  ezMaterialResource* pMaterial = ezResourceManager::BeginAcquireResource(m_hNewMaterial, ezResourceAcquireMode::AllowFallback);
  
  if (m_hNewMaterial != m_hMaterial || pMaterial->IsModified())
  {
    pMaterial->UpdateCaches();

    BindShaderInternal(pMaterial->m_hCachedShader, ezShaderBindFlags::Default);

    if (!pMaterial->m_hConstantBufferStorage.IsInvalidated())
    {
      BindConstantBuffer("MaterialConstants", pMaterial->m_hConstantBufferStorage);
    }

    for (auto it = pMaterial->m_CachedPermutationVars.GetIterator(); it.IsValid(); ++it)
    {
      SetShaderPermutationVariable(it.Key(), it.Value());
    }

    for (auto it = pMaterial->m_CachedTextureBindings.GetIterator(); it.IsValid(); ++it)
    {
      for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
      {
        BindTexture((ezGALShaderStage::Enum)stage, it.Key(), it.Value());
      }
    }

    m_hMaterial = m_hNewMaterial;
  }

  // The material needs its constant buffer updated.
  // Thus we keep it acquired until we have the correct shader permutation for the constant buffer layout.
  if (pMaterial->AreContantsModified())
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
      ezLog::Error("No resource is bound for constant buffer slot '%s'", binding.m_sName.GetData());
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
        ezLog::Error("Invalid constant buffer storage is bound for slot '%s'", binding.m_sName.GetData());
        m_pGALContext->SetConstantBuffer(binding.m_iSlot, ezGALBufferHandle());
      }
    }
  }
}

void ezRenderContext::ApplyTextureBindings(ezGALShaderStage::Enum stage, const ezShaderStageBinary* pBinary)
{
  for (const auto& binding : pBinary->m_ShaderResourceBindings)
  {
    if (binding.m_Type < ezShaderResourceBinding::Texture1D || binding.m_Type > ezShaderResourceBinding::TextureCubeArray)
      continue;

    const ezUInt32 uiResourceHash = binding.m_sName.GetHash();

    TextureViewSampler textureTuple;
    if (!m_BoundTextures[stage].TryGetValue(uiResourceHash, textureTuple))
    {
      ezLog::Error("No texture is bound for %s slot '%s'", ezGALShaderStage::Names[stage], binding.m_sName.GetData());
    }

    m_pGALContext->SetResourceView(stage, binding.m_iSlot, textureTuple.m_hResourceView);
    m_pGALContext->SetSamplerState(stage, binding.m_iSlot, textureTuple.m_hSamplerState);
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
    if (!m_BoundBuffer[stage].TryGetValue(uiResourceHash, hResourceView))
    {
      ezLog::Error("No buffer is bound for %s slot '%s'", ezGALShaderStage::Names[stage], binding.m_sName.GetData());
    }

    m_pGALContext->SetResourceView(stage, binding.m_iSlot, hResourceView);
  }
}
