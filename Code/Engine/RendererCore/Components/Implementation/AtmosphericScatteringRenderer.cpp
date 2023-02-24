#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Types/ScopeExit.h>
#include <RendererCore/Components/AtmosphericScatteringRenderer.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/RenderGraph/RenderGraph.h>
#include <RendererCore/RenderGraph/RenderGraphContext.h>
#include <RendererCore/RenderGraph/RenderGraphManager.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/Device/Device.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAtmosphericScatteringRenderData, 1, ezRTTIDefaultAllocator<ezAtmosphericScatteringRenderData>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAtmosphericScatteringRenderer, 1, ezRTTIDefaultAllocator<ezAtmosphericScatteringRenderer>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, AtmosphericScatteringLUTs)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core",
    "RenderWorld"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    ezAtmosphericScatteringLUTs::OnEngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    ezAtmosphericScatteringLUTs::OnEngineShutdown();
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

//////////////////////////////////////////////////////////////////////////
// ezAtmosphericScatteringLUTs

namespace
{
  /// Number of frames a set of lookup tables is kept alive after it was last requested.
  /// Has to cover the frames that may still be in flight with multithreaded rendering.
  constexpr ezUInt64 s_uiFramesToKeepUnused = 4;

  struct LUTSet
  {
    ezGALTextureHandle m_hTransmittance;
    ezGALTextureHandle m_hMultiScattering;
    ezGALTextureHandle m_hSkyView;
    ezUInt32 m_uiAtmosphereHash = 0;
    ezUInt32 m_uiSkyViewHash = 0;
    ezUInt64 m_uiLastUsedFrame = 0;
    bool m_bTexturesCreated = false;
    bool m_bContentValid = false; ///< Reset when an update failed, so that the next request recomputes all tables.
    bool m_bErrorLogged = false;
  };

  struct PendingUpdate
  {
    ezUInt64 m_uiKey = 0;
    ezGALTextureHandle m_hTransmittance;
    ezGALTextureHandle m_hMultiScattering;
    ezGALTextureHandle m_hSkyView;
    ezAtmosphereConstants m_Constants;
    bool m_bUpdateAtmosphere = false; // transmittance + multi-scattering
  };

  struct Data
  {
    ezMutex m_Mutex;
    ezHashTable<ezUInt64, LUTSet> m_LUTs;

    // double buffered, written during extraction, consumed when the same frame gets rendered
    // ezAtmosphereConstants is a 16 byte aligned constant buffer struct, so the array needs an aligned allocator
    ezDynamicArray<PendingUpdate, ezAlignedAllocatorWrapper> m_PendingUpdates[2];

    ezSharedPtr<ezRenderGraph> m_pRenderGraph;
    ezShaderResourceHandle m_hTransmittanceShader;
    ezShaderResourceHandle m_hMultiScatteringShader;
    ezShaderResourceHandle m_hSkyViewShader;
    ezConstantBufferStorageHandle m_hConstants;
  };

  Data* s_pData = nullptr;

  /// Hash of all parameters that affect the transmittance and multi-scattering tables.
  ezUInt32 ComputeAtmosphereHash(const ezAtmosphereConstants& c)
  {
    const float values[] = {
      c.RayleighScattering.x, c.RayleighScattering.y, c.RayleighScattering.z,
      c.OzoneAbsorption.x, c.OzoneAbsorption.y, c.OzoneAbsorption.z,
      c.GroundAlbedo.x, c.GroundAlbedo.y, c.GroundAlbedo.z,
      c.PlanetRadius, c.AtmosphereRadius, c.MieScattering, c.MieAbsorption, c.RayleighScaleHeight, c.MieScaleHeight};

    return ezHashingUtils::xxHash32(values, sizeof(values));
  }

  /// Hash of all parameters that affect the sky-view table, which includes everything that affects the other tables.
  ezUInt32 ComputeSkyViewHash(const ezAtmosphereConstants& c, ezUInt32 uiAtmosphereHash)
  {
    const float values[] = {c.SunDirection.x, c.SunDirection.y, c.SunDirection.z, c.MieAnisotropy, c.MaxSkyViewHeight};
    return ezHashingUtils::xxHash32(values, sizeof(values), uiAtmosphereHash);
  }

  ezGALTextureHandle CreateLUT(ezUInt32 uiWidth, ezUInt32 uiHeight, ezUInt32 uiDepth)
  {
    ezGALTextureCreationDescription desc;
    desc.m_uiWidth = uiWidth;
    desc.m_uiHeight = uiHeight;
    desc.m_uiDepth = uiDepth;
    desc.m_Type = uiDepth > 1 ? ezGALTextureType::Texture3D : ezGALTextureType::Texture2D;
    desc.m_Format = ezGALResourceFormat::RGBAHalf;
    desc.m_TextureFlags = ezGALTextureUsageFlags::ShaderResource | ezGALTextureUsageFlags::UnorderedAccess;
    desc.m_ResourceAccess.m_bImmutable = false;

    return ezGALDevice::GetDefaultDevice()->CreateTexture(desc);
  }

  void DestroyLUTs(LUTSet& ref_set)
  {
    ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
    pDevice->DestroyTexture(ref_set.m_hTransmittance);
    pDevice->DestroyTexture(ref_set.m_hMultiScattering);
    pDevice->DestroyTexture(ref_set.m_hSkyView);
  }

  /// Called from the render graph when a dispatch failed, typically because a shader didn't compile.
  /// Invalidates the tables, so they get recomputed on the next request. Only the first failure per set is logged.
  void OnUpdateFailed(ezUInt64 uiKey, ezStringView sPass)
  {
    EZ_LOCK(s_pData->m_Mutex);

    LUTSet* pSet = s_pData->m_LUTs.GetValue(uiKey);
    if (pSet == nullptr)
      return;

    pSet->m_bContentValid = false;

    if (!pSet->m_bErrorLogged)
    {
      pSet->m_bErrorLogged = true;
      ezLog::Error("Atmospheric scattering: '{}' failed, the sky will not be rendered correctly.", sPass);
    }
  }

  constexpr ezUInt32 GetNumGroups(ezUInt32 uiSize)
  {
    return (uiSize + ATMOSPHERE_LUT_THREADS - 1) / ATMOSPHERE_LUT_THREADS;
  }

  void AddComputePasses(ezRenderGraph& ref_graph, const PendingUpdate& update)
  {
    const ezRenderGraphTextureHandle hTransmittance = ref_graph.ImportTexture(update.m_hTransmittance);
    const ezRenderGraphTextureHandle hMultiScattering = ref_graph.ImportTexture(update.m_hMultiScattering);
    const ezRenderGraphTextureHandle hSkyView = ref_graph.ImportTexture(update.m_hSkyView);

    const ezAtmosphereConstants constants = update.m_Constants;
    const ezUInt64 uiKey = update.m_uiKey;
    const ezConstantBufferStorageHandle hConstants = s_pData->m_hConstants;

    // Compute shaders must not be skipped while they are still loading, otherwise the tables would stay uninitialized.
    auto PrepareDispatch = [constants, hConstants](ezRenderContext* pRC, const ezShaderResourceHandle& hShader) -> ezBindGroupBuilder&
    {
      *ezRenderContext::GetConstantBufferData<ezAtmosphereConstants>(hConstants) = constants;
      pRC->BindShader(hShader);
      ezBindGroupBuilder& bindGroup = pRC->GetBindGroup(EZ_GAL_BIND_GROUP_DRAW_CALL);
      bindGroup.BindBuffer("ezAtmosphereConstants", hConstants);
      return bindGroup;
    };

    if (update.m_bUpdateAtmosphere)
    {
      {
        auto pass = ref_graph.AddComputePass("AtmosphereTransmittanceLUT");
        pass.WriteTexture(hTransmittance);
        pass.HasSideEffects();
        pass.SetExecuteCallback([PrepareDispatch, uiKey, hTransmittance, hShader = s_pData->m_hTransmittanceShader](const ezRenderGraphContext& ctx)
          {
            ezRenderContext* pRC = ctx.GetRenderContext();
            const bool bPrevAsync = pRC->GetAllowAsyncShaderLoading();
            pRC->SetAllowAsyncShaderLoading(false);
            EZ_SCOPE_EXIT(pRC->SetAllowAsyncShaderLoading(bPrevAsync));

            ezBindGroupBuilder& bindGroup = PrepareDispatch(pRC, hShader);
            bindGroup.BindTexture("TransmittanceOutput", ctx.ResolveTexture(hTransmittance));
            if (pRC->Dispatch(GetNumGroups(ATMOSPHERE_TRANSMITTANCE_LUT_WIDTH), GetNumGroups(ATMOSPHERE_TRANSMITTANCE_LUT_HEIGHT), 1).Failed())
            {
              OnUpdateFailed(uiKey, "AtmosphereTransmittanceLUT");
            } });
      }

      {
        auto pass = ref_graph.AddComputePass("AtmosphereMultiScatteringLUT");
        pass.ReadTexture(hTransmittance);
        pass.WriteTexture(hMultiScattering);
        pass.HasSideEffects();
        pass.SetExecuteCallback([PrepareDispatch, uiKey, hTransmittance, hMultiScattering, hShader = s_pData->m_hMultiScatteringShader](const ezRenderGraphContext& ctx)
          {
            ezRenderContext* pRC = ctx.GetRenderContext();
            const bool bPrevAsync = pRC->GetAllowAsyncShaderLoading();
            pRC->SetAllowAsyncShaderLoading(false);
            EZ_SCOPE_EXIT(pRC->SetAllowAsyncShaderLoading(bPrevAsync));

            ezBindGroupBuilder& bindGroup = PrepareDispatch(pRC, hShader);
            bindGroup.BindTexture("TransmittanceLut", ctx.ResolveTexture(hTransmittance));
            bindGroup.BindTexture("MultiScatteringOutput", ctx.ResolveTexture(hMultiScattering));
            if (pRC->Dispatch(GetNumGroups(ATMOSPHERE_MULTISCATTERING_LUT_SIZE), GetNumGroups(ATMOSPHERE_MULTISCATTERING_LUT_SIZE), 1).Failed())
            {
              OnUpdateFailed(uiKey, "AtmosphereMultiScatteringLUT");
            } });
      }
    }

    {
      auto pass = ref_graph.AddComputePass("AtmosphereSkyViewLUT");
      pass.ReadTexture(hTransmittance);
      pass.ReadTexture(hMultiScattering);
      pass.WriteTexture(hSkyView);
      pass.HasSideEffects();
      pass.SetExecuteCallback([PrepareDispatch, uiKey, hTransmittance, hMultiScattering, hSkyView, hShader = s_pData->m_hSkyViewShader](const ezRenderGraphContext& ctx)
        {
          ezRenderContext* pRC = ctx.GetRenderContext();
          const bool bPrevAsync = pRC->GetAllowAsyncShaderLoading();
          pRC->SetAllowAsyncShaderLoading(false);
          EZ_SCOPE_EXIT(pRC->SetAllowAsyncShaderLoading(bPrevAsync));

          ezBindGroupBuilder& bindGroup = PrepareDispatch(pRC, hShader);
          bindGroup.BindTexture("TransmittanceLut", ctx.ResolveTexture(hTransmittance));
          bindGroup.BindTexture("MultiScatteringLut", ctx.ResolveTexture(hMultiScattering));
          bindGroup.BindTexture("SkyViewOutput", ctx.ResolveTexture(hSkyView));
          if (pRC->Dispatch(GetNumGroups(ATMOSPHERE_SKYVIEW_LUT_WIDTH), GetNumGroups(ATMOSPHERE_SKYVIEW_LUT_HEIGHT), ATMOSPHERE_SKYVIEW_LUT_DEPTH).Failed())
          {
            OnUpdateFailed(uiKey, "AtmosphereSkyViewLUT");
          } });
    }
  }
} // namespace

// static
ezAtmosphericScatteringLUTs::LUTs ezAtmosphericScatteringLUTs::Request(ezUInt64 uiKey, const ezAtmosphereConstants& constants)
{
  EZ_LOCK(s_pData->m_Mutex);

  LUTSet& set = s_pData->m_LUTs[uiKey];
  set.m_uiLastUsedFrame = ezRenderWorld::GetFrameCounter();

  if (!set.m_bTexturesCreated)
  {
    set.m_bTexturesCreated = true;
    set.m_hTransmittance = CreateLUT(ATMOSPHERE_TRANSMITTANCE_LUT_WIDTH, ATMOSPHERE_TRANSMITTANCE_LUT_HEIGHT, 1);
    set.m_hMultiScattering = CreateLUT(ATMOSPHERE_MULTISCATTERING_LUT_SIZE, ATMOSPHERE_MULTISCATTERING_LUT_SIZE, 1);
    set.m_hSkyView = CreateLUT(ATMOSPHERE_SKYVIEW_LUT_WIDTH, ATMOSPHERE_SKYVIEW_LUT_HEIGHT, ATMOSPHERE_SKYVIEW_LUT_DEPTH);
  }

  const ezUInt32 uiAtmosphereHash = ComputeAtmosphereHash(constants);
  const ezUInt32 uiSkyViewHash = ComputeSkyViewHash(constants, uiAtmosphereHash);

  const bool bUpdateAtmosphere = !set.m_bContentValid || set.m_uiAtmosphereHash != uiAtmosphereHash;
  const bool bUpdateSkyView = bUpdateAtmosphere || set.m_uiSkyViewHash != uiSkyViewHash;

  if (bUpdateSkyView)
  {
    PendingUpdate& update = s_pData->m_PendingUpdates[ezRenderWorld::GetDataIndexForExtraction()].ExpandAndGetRef();
    update.m_uiKey = uiKey;
    update.m_hTransmittance = set.m_hTransmittance;
    update.m_hMultiScattering = set.m_hMultiScattering;
    update.m_hSkyView = set.m_hSkyView;
    update.m_Constants = constants;
    update.m_bUpdateAtmosphere = bUpdateAtmosphere;

    set.m_uiAtmosphereHash = uiAtmosphereHash;
    set.m_uiSkyViewHash = uiSkyViewHash;
    set.m_bContentValid = true;
  }

  LUTs result;
  result.m_hTransmittance = set.m_hTransmittance;
  result.m_hSkyView = set.m_hSkyView;
  return result;
}

// static
void ezAtmosphericScatteringLUTs::OnEngineStartup()
{
  s_pData = EZ_DEFAULT_NEW(Data);

  s_pData->m_hTransmittanceShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Atmosphere/AtmosphereTransmittanceLUT.ezShader");
  s_pData->m_hMultiScatteringShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Atmosphere/AtmosphereMultiScatteringLUT.ezShader");
  s_pData->m_hSkyViewShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Atmosphere/AtmosphereSkyViewLUT.ezShader");
  s_pData->m_hConstants = ezRenderContext::CreateConstantBufferStorage<ezAtmosphereConstants>();

  ezRenderWorld::GetRenderEvent().AddEventHandler(OnRenderEvent);
}

// static
void ezAtmosphericScatteringLUTs::OnEngineShutdown()
{
  ezRenderWorld::GetRenderEvent().RemoveEventHandler(OnRenderEvent);

  for (auto it = s_pData->m_LUTs.GetIterator(); it.IsValid(); ++it)
  {
    DestroyLUTs(it.Value());
  }

  ezRenderContext::DeleteConstantBufferStorage(s_pData->m_hConstants);

  EZ_DEFAULT_DELETE(s_pData);
}

// static
void ezAtmosphericScatteringLUTs::OnRenderEvent(const ezRenderWorldRenderEvent& e)
{
  if (e.m_Type != ezRenderWorldRenderEvent::Type::BeginRender)
    return;

  EZ_LOCK(s_pData->m_Mutex);

  // delete tables that are no longer in use
  {
    const ezUInt64 uiFrame = ezRenderWorld::GetFrameCounter();

    for (auto it = s_pData->m_LUTs.GetIterator(); it.IsValid();)
    {
      if (it.Value().m_uiLastUsedFrame + s_uiFramesToKeepUnused < uiFrame)
      {
        DestroyLUTs(it.Value());
        it = s_pData->m_LUTs.Remove(it);
      }
      else
      {
        ++it;
      }
    }
  }

  ezDynamicArray<PendingUpdate, ezAlignedAllocatorWrapper>& pendingUpdates = s_pData->m_PendingUpdates[ezRenderWorld::GetDataIndexForRendering()];
  if (pendingUpdates.IsEmpty())
    return;

  EZ_PROFILE_SCOPE("AtmosphericScatteringLUTs");

  if (s_pData->m_pRenderGraph == nullptr)
  {
    s_pData->m_pRenderGraph = ezRenderGraphManager::CreateRenderGraph("AtmosphericScatteringLUTs", ezRenderGraphPhase::PreRender);
  }

  s_pData->m_pRenderGraph->Reset();

  for (const PendingUpdate& update : pendingUpdates)
  {
    AddComputePasses(*s_pData->m_pRenderGraph, update);
  }

  pendingUpdates.Clear();

  ezRenderGraphManager::EnqueueRenderGraph(s_pData->m_pRenderGraph);
}

//////////////////////////////////////////////////////////////////////////
// ezAtmosphericScatteringRenderer

ezAtmosphericScatteringRenderer::ezAtmosphericScatteringRenderer()
{
  m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Atmosphere/AtmosphereSky.ezShader");
  m_hConstants = ezRenderContext::CreateConstantBufferStorage<ezAtmosphereConstants>();
}

ezAtmosphericScatteringRenderer::~ezAtmosphericScatteringRenderer()
{
  ezRenderContext::DeleteConstantBufferStorage(m_hConstants);
}

void ezAtmosphericScatteringRenderer::GetSupportedRenderDataTypes(ezDynamicArray<const ezRTTI*>& out_types) const
{
  out_types.PushBack(ezGetStaticRTTI<ezAtmosphericScatteringRenderData>());
}

void ezAtmosphericScatteringRenderer::RenderBatch(const ezRenderViewContext& renderViewContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const
{
  ezRenderContext* pContext = renderViewContext.m_pRenderContext;

  for (auto it = batch.GetIterator<ezAtmosphericScatteringRenderData>(); it.IsValid(); ++it)
  {
    const ezAtmosphericScatteringRenderData* pRenderData = it;

    *ezRenderContext::GetConstantBufferData<ezAtmosphereConstants>(m_hConstants) = pRenderData->m_Constants;

    pContext->BindShader(m_hShader);

    ezBindGroupBuilder& bindGroup = pContext->GetBindGroup(EZ_GAL_BIND_GROUP_DRAW_CALL);
    bindGroup.BindTexture("TransmittanceLut", pRenderData->m_hTransmittanceLUT);
    bindGroup.BindTexture("SkyViewLut", pRenderData->m_hSkyViewLUT);
    bindGroup.BindBuffer("ezAtmosphereConstants", m_hConstants);

    pContext->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, 1);
    pContext->DrawMeshBuffer().IgnoreResult();
  }
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_AtmosphericScatteringRenderer);
