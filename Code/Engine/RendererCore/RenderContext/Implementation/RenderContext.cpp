#include <RendererCore/PCH.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Shader/ShaderPermutationResource.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/Textures/TextureResource.h>

ezRenderContext* ezRenderContext::s_DefaultInstance = nullptr;
ezHybridArray<ezRenderContext*, 4> ezRenderContext::s_Instances;

ezMap<ezRenderContext::ShaderVertexDecl, ezGALVertexDeclarationHandle> ezRenderContext::s_GALVertexDeclarations;
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
  ezRenderContext::OnCoreShutdown();
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
  m_pCurrentlyModifyingBuffer = nullptr;
  m_uiMeshBufferPrimitiveCount = 0;
  m_uiLastMaterialCBSync = 0;
}

ezRenderContext::~ezRenderContext()
{
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

void ezRenderContext::SetMaterialState(const ezMaterialResourceHandle& hMaterial)
{
  if (!hMaterial.IsValid())
  {
    BindShader(ezShaderResourceHandle());
    return;
  }
    

  ezHybridArray<ezMaterialResource*, 16> materialHierarchy;

  ezMaterialResourceHandle hCurrentMaterial = hMaterial;

  while (true)
  {
    ezMaterialResource* pMaterial = ezResourceManager::BeginAcquireResource(hCurrentMaterial);

    materialHierarchy.PushBack(pMaterial);

    const ezMaterialResourceHandle& hParentMaterial = pMaterial->GetDescriptor().m_hBaseMaterial;
    if (!hParentMaterial.IsValid() || hParentMaterial == hCurrentMaterial)
      break;

    hCurrentMaterial = hParentMaterial;
  }

  ezShaderResourceHandle hShader;

  // set state of parent material first
  for (ezUInt32 i = materialHierarchy.GetCount(); i-- > 0; )
  {
    ezMaterialResource* pMaterial = materialHierarchy[i];
    const ezMaterialResourceDescriptor& desc = pMaterial->GetDescriptor();

    if (desc.m_hShader.IsValid())
      hShader = desc.m_hShader;

    for (const auto& permutationVar : desc.m_PermutationVars)
    {
      SetShaderPermutationVariable(permutationVar.m_sName, permutationVar.m_sValue);
    }

    for (const auto& shaderConstant : desc.m_ShaderConstants)
    {
      SetMaterialParameter(shaderConstant.m_Name, shaderConstant.m_Value);
    }

    for (const auto& textureBinding : desc.m_TextureBindings)
    {
      ezResourceLock<ezTextureResource> pTexture(textureBinding.m_Value, ezResourceAcquireMode::AllowFallback);
      ezGALResourceViewHandle hResourceView = ezGALDevice::GetDefaultDevice()->GetDefaultResourceView(pTexture->GetGALTexture());
      ezGALSamplerStateHandle hSamplerState = pTexture->GetGALSamplerState();

      for (int i = 0; i < ezGALShaderStage::ENUM_COUNT; i++)
      {
        BindTexture((ezGALShaderStage::Enum)i, textureBinding.m_Name, hResourceView, hSamplerState);
      }
    }

    ezResourceManager::EndAcquireResource(pMaterial);
  }

  // Always bind the shader so that in case of an invalid shader the drawcall is skipped later.
  // Otherwise we will render with the shader of the previous material which can lead to strange behavior.
  BindShader(hShader);
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
  ezShaderPermutationResource* pShaderPermutation = nullptr;

  bool bRebuildVertexDeclaration = m_StateFlags.IsAnySet(ezRenderContextFlags::ShaderStateChanged | ezRenderContextFlags::MeshBufferBindingChanged);

  if (bForce || m_StateFlags.IsSet(ezRenderContextFlags::ShaderStateChanged))
  {
    m_hActiveGALShader.Invalidate();

    m_StateFlags.Remove(ezRenderContextFlags::ShaderStateValid);
    m_StateFlags.Add(ezRenderContextFlags::TextureBindingChanged | ezRenderContextFlags::ConstantBufferBindingChanged);

    if (!m_hActiveShader.IsValid())
      return EZ_FAILURE;

    ezResourceLock<ezShaderResource> pShader(m_hActiveShader, ezResourceAcquireMode::AllowFallback);

    if (!pShader || !pShader->IsShaderValid())
      return EZ_FAILURE;

    ezTempHashedString sTopologies[ezGALPrimitiveTopology::ENUM_COUNT] = 
    {
      ezTempHashedString("POINTS"),
      ezTempHashedString("LINES"),
      ezTempHashedString("TRIANGLES")
    };

    SetShaderPermutationVariable("TOPOLOGY", sTopologies[m_Topology]);
    
    m_hActiveShaderPermutation = ezShaderManager::PreloadSinglePermutation(m_hActiveShader, m_PermutationVariables, ezTime::Seconds(0.0));

    if (!m_hActiveShaderPermutation.IsValid())
      return EZ_FAILURE;

    pShaderPermutation = ezResourceManager::BeginAcquireResource(m_hActiveShaderPermutation, ezResourceAcquireMode::AllowFallback);

    if (!pShaderPermutation->IsShaderValid())
    {
      ezResourceManager::EndAcquireResource(pShaderPermutation);
      return EZ_FAILURE;
    }

    m_hActiveGALShader = pShaderPermutation->GetGALShader();
    EZ_ASSERT_DEV(!m_hActiveGALShader.IsInvalidated(), "Invalid GAL Shader handle.");

    m_pGALContext->SetShader(m_hActiveGALShader);

    // Set render state from shader (unless they are all deactivated)
    if (!m_ShaderBindFlags.AreAllSet(ezShaderBindFlags::NoBlendState | ezShaderBindFlags::NoRasterizerState | ezShaderBindFlags::NoDepthStencilState))
    {
      if (!m_ShaderBindFlags.IsSet(ezShaderBindFlags::NoBlendState))
        m_pGALContext->SetBlendState(pShaderPermutation->GetBlendState());

      if (!m_ShaderBindFlags.IsSet(ezShaderBindFlags::NoRasterizerState))
        m_pGALContext->SetRasterizerState(pShaderPermutation->GetRasterizerState());

      if (!m_ShaderBindFlags.IsSet(ezShaderBindFlags::NoDepthStencilState))
        m_pGALContext->SetDepthStencilState(pShaderPermutation->GetDepthStencilState());
    }

    m_StateFlags.Remove(ezRenderContextFlags::ShaderStateChanged);
    m_StateFlags.Add(ezRenderContextFlags::ShaderStateValid);
  }

  if ((bForce || m_StateFlags.IsSet(ezRenderContextFlags::TextureBindingChanged)) && m_hActiveShaderPermutation.IsValid())
  {
    if (pShaderPermutation == nullptr)
      pShaderPermutation = ezResourceManager::BeginAcquireResource(m_hActiveShaderPermutation, ezResourceAcquireMode::AllowFallback);

    for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
    {
      if (auto pBin = pShaderPermutation->GetShaderStageBinary((ezGALShaderStage::Enum) stage))
      {
        ApplyTextureBindings((ezGALShaderStage::Enum) stage, pBin);
      }
    }

    m_StateFlags.Remove(ezRenderContextFlags::TextureBindingChanged);
  }

  if ((bForce || m_StateFlags.IsSet(ezRenderContextFlags::BufferBindingChanged)) && m_hActiveShaderPermutation.IsValid())
  {
    if (pShaderPermutation == nullptr)
      pShaderPermutation = ezResourceManager::BeginAcquireResource(m_hActiveShaderPermutation, ezResourceAcquireMode::AllowFallback);

    for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
    {
      if (auto pBin = pShaderPermutation->GetShaderStageBinary((ezGALShaderStage::Enum) stage))
      {
        ApplyBufferBindings((ezGALShaderStage::Enum) stage, pBin);
      }
    }

    m_StateFlags.Remove(ezRenderContextFlags::BufferBindingChanged);
  }

  UploadGlobalConstants();

  if ((bForce || m_StateFlags.IsSet(ezRenderContextFlags::ConstantBufferBindingChanged) || (s_LastMaterialParamModification > m_uiLastMaterialCBSync)) && m_hActiveShaderPermutation.IsValid())
  {
    m_uiLastMaterialCBSync = s_LastMaterialParamModification;

    if (pShaderPermutation == nullptr)
      pShaderPermutation = ezResourceManager::BeginAcquireResource(m_hActiveShaderPermutation, ezResourceAcquireMode::AllowFallback);

    for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
    {
      auto pBin = pShaderPermutation->GetShaderStageBinary((ezGALShaderStage::Enum) stage);

      if (pBin == nullptr)
        continue;

      ApplyConstantBufferBindings(pBin);
    }

    m_StateFlags.Remove(ezRenderContextFlags::ConstantBufferBindingChanged);
  }

  if (pShaderPermutation != nullptr)
    ezResourceManager::EndAcquireResource(pShaderPermutation);

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

    m_pGALContext->SetVertexDeclaration(hVertexDeclaration);

    m_StateFlags.Remove(ezRenderContextFlags::MeshBufferBindingChanged);
  }

  return EZ_SUCCESS;
}

void ezRenderContext::BindShader(ezShaderResourceHandle hShader, ezBitflags<ezShaderBindFlags> flags)
{
  m_ShaderBindFlags = flags;

  if (flags.IsAnySet(ezShaderBindFlags::ForceRebind) || m_hActiveShader != hShader)
    m_StateFlags.Add(ezRenderContextFlags::ShaderStateChanged);

  m_hActiveShader = hShader;
}

ezGALShaderHandle ezRenderContext::GetActiveGALShader()
{
  // make sure the internal state is up to date
  if (ApplyContextStates(false).Failed())
  {
    // return invalid handle ?
  }

  if (!m_StateFlags.IsSet(ezRenderContextFlags::ShaderStateValid))
    return ezGALShaderHandle(); // invalid handle

  return m_hActiveGALShader;
}

//static
void ezRenderContext::OnEngineShutdown()
{
  ezShaderStageBinary::OnEngineShutdown();

  for (auto rc : s_Instances)
    EZ_DEFAULT_DELETE(rc);

  s_Instances.Clear();
  s_hGlobalConstantBuffer.Invalidate();

  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(s_hDefaultSamplerStates); ++i)
  {
    if (!s_hDefaultSamplerStates[i].IsInvalidated())
    {
      ezGALDevice::GetDefaultDevice()->DestroySamplerState(s_hDefaultSamplerStates[i]);
      s_hDefaultSamplerStates[i].Invalidate();
    }
  }

  for (auto it = s_GALVertexDeclarations.GetIterator(); it.IsValid(); ++it)
  {
    ezGALDevice::GetDefaultDevice()->DestroyVertexDeclaration(it.Value());
  }

  s_GALVertexDeclarations.Clear();

  // reset to a default state by re-constructing the struct
  ezMemoryUtils::Construct(&s_GlobalConstants, 1);
}

//static
void ezRenderContext::OnCoreShutdown()
{
  EZ_ASSERT_DEV(s_GALVertexDeclarations.IsEmpty(), "ezRenderContext::OnEngineShutdown has not been called. Either ezStartup::ShutdownEngine was not called or ezStartup::StartupEngine was not called at program start");

  for (ezUInt32 i = 0; i < ezGALShaderStage::ENUM_COUNT; ++i)
    ezShaderStageBinary::s_ShaderStageBinaries[i].Clear();
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

void ezRenderContext::ApplyTextureBindings(ezGALShaderStage::Enum stage, const ezShaderStageBinary* pBinary)
{
  for (const auto& resourceBinding : pBinary->m_ShaderResourceBindings)
  {
    if (resourceBinding.m_Type < ezShaderStageResource::Texture1D || resourceBinding.m_Type > ezShaderStageResource::TextureCubeArray)
      continue;

    const ezUInt32 uiResourceHash = resourceBinding.m_Name.GetHash();

    TextureViewSampler* textureTuple;
    if (!m_BoundTextures[stage].TryGetValue(uiResourceHash, textureTuple))
    {
      ezLog::Error("No resource is bound for shader slot '%s'", resourceBinding.m_Name.GetData());
      continue;
    }

    if (textureTuple == nullptr)
    {
      ezLog::Error("An invalid resource is bound for shader slot '%s'", resourceBinding.m_Name.GetData());
      continue;
    }

    m_pGALContext->SetResourceView(stage, resourceBinding.m_iSlot, textureTuple->m_hResourceView);
    m_pGALContext->SetSamplerState(stage, resourceBinding.m_iSlot, textureTuple->m_hSamplerState);
  }
}

void ezRenderContext::ApplyBufferBindings(ezGALShaderStage::Enum stage, const ezShaderStageBinary* pBinary)
{
  for (const auto& resourceBinding : pBinary->m_ShaderResourceBindings)
  {
    if (resourceBinding.m_Type != ezShaderStageResource::GenericBuffer)
      continue;

    const ezUInt32 uiResourceHash = resourceBinding.m_Name.GetHash();

    ezGALResourceViewHandle hResourceView;
    if (!m_BoundBuffer[stage].TryGetValue(uiResourceHash, hResourceView))
    {
      ezLog::Error("No resource is bound for shader slot '%s'", resourceBinding.m_Name.GetData());
      continue;
    }

    m_pGALContext->SetResourceView(stage, resourceBinding.m_iSlot, hResourceView);
  }
}
