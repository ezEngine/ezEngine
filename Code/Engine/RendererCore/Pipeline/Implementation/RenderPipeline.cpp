#include <RendererCore/RendererCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Math/ColorScheme.h>
#include <Foundation/Math/Frustum.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/SimdMath/SimdBBox.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Utilities/DGMLWriter.h>
#include <RendererCore/Components/AlwaysVisibleComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/FrameDataProvider.h>
#include <RendererCore/Pipeline/Passes/TargetPass.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Rasterizer/RasterizerView.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderGraph/RenderGraphManager.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererFoundation/Resources/Texture.h>

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
ezCVarBool ezRenderPipeline::cvar_SpatialCullingVis("Spatial.Culling.Vis", false, ezCVarFlags::Default, "Enables debug visualization of visibility culling");
ezCVarBool cvar_SpatialCullingShowStats("Spatial.Culling.ShowStats", false, ezCVarFlags::Default, "Display some stats of the visibility culling");
#endif

ezCVarBool cvar_SpatialCullingOcclusionEnable("Spatial.Occlusion.Enable", true, ezCVarFlags::Default, "Use software rasterization for occlusion culling.");
ezCVarBool cvar_SpatialCullingOcclusionVisView("Spatial.Occlusion.VisView", false, ezCVarFlags::Default, "Render the occlusion framebuffer as an overlay.");
ezCVarFloat cvar_SpatialCullingOcclusionBoundsInlation("Spatial.Occlusion.BoundsInflation", 0.5f, ezCVarFlags::Default, "How much to inflate bounds during occlusion check.");
ezCVarFloat cvar_SpatialCullingOcclusionFarPlane("Spatial.Occlusion.FarPlane", 50.0f, ezCVarFlags::Default, "Far plane distance for finding occluders.");

ezRenderPipeline::ezRenderPipeline(ezDynamicArray<ezUniquePtr<ezRenderPipelinePass>>&& passes, ezDynamicArray<ezUniquePtr<ezExtractor>>&& extractors, ezArrayPtr<const ezRenderPipelineResourceLoaderConnection> connections)
  : m_PassGraph(std::move(passes), std::move(extractors), connections)
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  m_AverageCullingTime = ezTime::MakeFromSeconds(0.1f);
#endif

  for (const ezUniquePtr<ezRenderPipelinePass>& pPass : m_PassGraph.GetPasses())
  {
    pPass->m_pPipeline = this;
  }

  ezRenderGraphManager::s_RenderEvent.AddEventHandler(ezMakeDelegate(&ezRenderPipeline::OnRenderEvent, this));
}

ezRenderPipeline::~ezRenderPipeline()
{
  ezRenderGraphManager::s_RenderEvent.RemoveEventHandler(ezMakeDelegate(&ezRenderPipeline::OnRenderEvent, this));

  ezGALDevice::GetDefaultDevice()->DestroyTexture(m_hOcclusionDebugViewTexture);

  m_Data[0].Clear();
  m_Data[1].Clear();
}

void ezRenderPipeline::OnRenderEvent(const ezRenderGraphRenderEvent& e)
{
  if (e.m_Type == ezRenderGraphRenderEvent::Type::BeforeGraphExecution && e.m_pGraph == m_pRenderGraph)
  {
    UpdateRenderContext(*e.m_pContext);
  }
  else if (e.m_Type == ezRenderGraphRenderEvent::Type::AfterGraphExecution && e.m_pGraph == m_pRenderGraph)
  {
    {
      ezRenderWorldRenderEvent renderEvent;
      renderEvent.m_Type = ezRenderWorldRenderEvent::Type::AfterPipelineExecution;
      renderEvent.m_pRenderViewContext = &m_RenderViewContext;
      renderEvent.m_uiFrameCounter = ezRenderWorld::GetFrameCounter();

      EZ_PROFILE_SCOPE("AfterPipelineExecution");
      ezRenderWorld::s_RenderEvent.Broadcast(renderEvent);
    }

    auto& data = m_Data[ezRenderWorld::GetDataIndexForRendering()];
    data.Clear();
    m_CurrentRenderThread = (ezThreadID)0;
  }
}

void ezRenderPipeline::GetPasses(ezDynamicArray<const ezRenderPipelinePass*>& ref_passes) const
{
  ref_passes.Reserve(ref_passes.GetCount() + m_PassGraph.GetPasses().GetCount());

  for (const ezUniquePtr<ezRenderPipelinePass>& pPass : m_PassGraph.GetPasses())
  {
    ref_passes.PushBack(pPass.Borrow());
  }
}

void ezRenderPipeline::GetPasses(ezDynamicArray<ezRenderPipelinePass*>& ref_passes)
{
  ref_passes.Reserve(ref_passes.GetCount() + m_PassGraph.GetPasses().GetCount());

  for (const ezUniquePtr<ezRenderPipelinePass>& pPass : m_PassGraph.GetPasses())
  {
    ref_passes.PushBack(pPass.Borrow());
  }
}

ezRenderPipelinePass* ezRenderPipeline::GetPassByName(const ezStringView& sPassName)
{
  return m_PassGraph.GetPassByName(sPassName);
}

ezHashedString ezRenderPipeline::GetViewName() const
{
  return m_sName;
}

bool ezRenderPipeline::ShouldRender() const
{
  auto& data = m_Data[ezRenderWorld::GetDataIndexForRendering()];

  const ezWorld* pWorld = ezWorld::GetWorld(data.GetWorldHandle());
  if (pWorld == nullptr)
    return false;

  return true;
}

ezRenderPipeline::PipelineState ezRenderPipeline::Rebuild(const ezView& view)
{
  ezLogBlock b("ezRenderPipeline::Rebuild");

  bool bRes = RebuildInternal(view);
  if (!bRes)
    m_PipelineState = PipelineState::RebuildError;
  return m_PipelineState;
}

bool ezRenderPipeline::RebuildInternal(const ezView& view)
{
  UpdateViewData(view, ezRenderWorld::GetDataIndexForRendering());
  if (m_PassGraph.CullDeadPasses().Failed() || m_PassGraph.SortPasses().Failed())
    return false;
  m_PipelineState = PipelineState::Initialized;
  return true;
}

bool ezRenderPipeline::RebuildRenderGraph(const ezViewData& viewData, const ezCamera& camera)
{
  m_uiSettingsModificationCounter = camera.GetSettingsModificationCounter();

  if (!m_pRenderGraph)
  {
    m_pRenderGraph = ezRenderGraphManager::CreateRenderGraph("RenderPipeline");
    m_pRenderGraph->SetUserData(&m_RenderViewContext);
  }

  m_pRenderGraph->SetUserName(viewData.m_sName);
  m_pRenderGraph->Reset();
  if (!AddRenderPasses(viewData, camera))
    return false;
  if (!UpdateTextureProviders())
    return false;

  m_PipelineState = PipelineState::RenderGraphBuilt;
  return true;
}

bool ezRenderPipeline::AddRenderPasses(const ezViewData& viewData, const ezCamera& camera)
{
  return m_PassGraph.AddRenderPasses(viewData, camera, *m_pRenderGraph).Succeeded();
}

bool ezRenderPipeline::UpdateTextureProviders()
{
  return m_PassGraph.UpdateTextureProviders(*m_pRenderGraph).Succeeded();
}


void ezRenderPipeline::UpdateViewData(const ezView& view, ezUInt32 uiDataIndex)
{
  if (!view.IsValid())
    return;

  if (uiDataIndex == ezRenderWorld::GetDataIndexForExtraction() && m_CurrentExtractThread != (ezThreadID)0)
    return;

  EZ_ASSERT_DEV(uiDataIndex <= 1, "Data index must be 0 or 1");
  auto& data = m_Data[uiDataIndex];

  data.SetCamera(*view.GetCamera());
  data.SetViewData(view.GetData());
}

void ezRenderPipeline::GetExtractors(ezDynamicArray<const ezExtractor*>& ref_extractors) const
{
  ref_extractors.Reserve(ref_extractors.GetCount() + m_PassGraph.GetExtractors().GetCount());

  for (const ezUniquePtr<ezExtractor>& pExtractor : m_PassGraph.GetExtractors())
  {
    ref_extractors.PushBack(pExtractor.Borrow());
  }
}

void ezRenderPipeline::GetExtractors(ezDynamicArray<ezExtractor*>& ref_extractors)
{
  ref_extractors.Reserve(ref_extractors.GetCount() + m_PassGraph.GetExtractors().GetCount());

  for (const ezUniquePtr<ezExtractor>& pExtractor : m_PassGraph.GetExtractors())
  {
    ref_extractors.PushBack(pExtractor.Borrow());
  }
}


ezExtractor* ezRenderPipeline::GetExtractorByName(const ezStringView& sExtractorName)
{
  return m_PassGraph.GetExtractorByName(sExtractorName);
}

ezFrameDataProviderBase* ezRenderPipeline::GetFrameDataProvider(const ezRTTI* pRtti) const
{
  ezUInt32 uiIndex = 0;
  if (m_TypeToDataProviderIndex.TryGetValue(pRtti, uiIndex))
  {
    return m_DataProviders[uiIndex].Borrow();
  }

  ezUniquePtr<ezFrameDataProviderBase> pNewDataProvider = pRtti->GetAllocator()->Allocate<ezFrameDataProviderBase>();
  ezFrameDataProviderBase* pResult = pNewDataProvider.Borrow();
  pResult->m_pOwnerPipeline = this;

  m_TypeToDataProviderIndex.Insert(pRtti, m_DataProviders.GetCount());
  m_DataProviders.PushBack(std::move(pNewDataProvider));

  return pResult;
}

void ezRenderPipeline::ExtractData(const ezView& view)
{
  EZ_ASSERT_DEV(m_CurrentExtractThread == (ezThreadID)0, "Extract must not be called from multiple threads.");
  m_CurrentExtractThread = ezThreadUtils::GetCurrentThreadID();

  // Is this view already extracted?
  if (m_uiLastExtractionFrame == ezRenderWorld::GetFrameCounter())
  {
    EZ_REPORT_FAILURE("View '{0}' is extracted multiple times", view.GetName());
    return;
  }

  EZ_PROFILE_SCOPE("ezRenderPipeline::ExtractData");

  m_uiLastExtractionFrame = ezRenderWorld::GetFrameCounter();

  // Determine visible objects
  FindVisibleObjects(view);

  // Extract and sort data
  auto& data = m_Data[ezRenderWorld::GetDataIndexForExtraction()];

  // Usually clear is not needed, only if the multithreading flag is switched during runtime.
  data.Clear();

  ezRenderWorldExtractionEvent extractionEvent;
  extractionEvent.m_Type = ezRenderWorldExtractionEvent::Type::BeforeViewExtraction;
  extractionEvent.m_pView = &view;
  extractionEvent.m_pExtractedRenderData = &data;
  extractionEvent.m_uiFrameCounter = ezRenderWorld::GetFrameCounter();
  ezRenderWorld::s_ExtractionEvent.Broadcast(extractionEvent);

  // Store camera and viewdata
  data.SetCamera(*view.GetCamera());
  data.SetViewData(view.GetData());
  data.SetWorldHandle(view.GetWorld()->GetHandle());
  data.SetWorldTime(view.GetWorld()->GetClock().GetAccumulatedTime());
  data.SetWorldDebugContext(view.GetWorld());
  data.SetViewDebugContext(view.GetHandle());

  // Extract object render data
  for (const ezUniquePtr<ezExtractor>& pExtractor : m_PassGraph.GetExtractors())
  {
    if (pExtractor->m_bActive)
    {
      EZ_PROFILE_SCOPE(pExtractor->m_sName.GetData());

      pExtractor->Extract(view, m_VisibleObjects, data);
    }
  }

  for (auto& processor : m_RenderDataProcessors)
  {
    processor(data);
  }

  data.SortAndBatch();

  for (const ezUniquePtr<ezExtractor>& pExtractor : m_PassGraph.GetExtractors())
  {
    if (pExtractor->m_bActive)
    {
      EZ_PROFILE_SCOPE(pExtractor->m_sName.GetData());

      pExtractor->PostSortAndBatch(view, m_VisibleObjects, data);
    }
  }

  extractionEvent.m_Type = ezRenderWorldExtractionEvent::Type::AfterViewExtraction;
  ezRenderWorld::s_ExtractionEvent.Broadcast(extractionEvent);

  m_CurrentExtractThread = (ezThreadID)0;
}

ezUniquePtr<ezRasterizerViewPool> g_pRasterizerViewPool;

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, SwRasterizer)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    g_pRasterizerViewPool = EZ_DEFAULT_NEW(ezRasterizerViewPool);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    g_pRasterizerViewPool.Clear();
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

void ezRenderPipeline::FindVisibleObjects(const ezView& view)
{
  EZ_PROFILE_SCOPE("ezRenderPipeline::FindVisibleObjects");

  ezFrustum frustum;
  view.ComputeCullingFrustum(frustum);

  EZ_LOCK(view.GetWorld()->GetReadMarker());

  const bool bIsMainView = (view.GetCameraUsageHint() == ezCameraUsageHint::MainView || view.GetCameraUsageHint() == ezCameraUsageHint::EditorView);
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  const bool bRecordStats = cvar_SpatialCullingShowStats && bIsMainView;
  ezSpatialSystem::QueryStats stats;
#endif

  ezSpatialSystem::QueryParams queryParams;
  queryParams.m_uiCategoryBitmask = ezDefaultSpatialDataCategories::RenderStatic.GetBitmask() | ezDefaultSpatialDataCategories::RenderDynamic.GetBitmask();
  queryParams.m_pIncludeTags = &view.m_IncludeTags;
  queryParams.m_pExcludeTags = &view.m_ExcludeTags;
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  queryParams.m_pStats = bRecordStats ? &stats : nullptr;
#endif

  ezFrustum limitedFrustum = frustum;
  const ezPlane farPlane = limitedFrustum.GetPlane(ezFrustum::PlaneType::FarPlane);
  limitedFrustum.AccessPlane(ezFrustum::PlaneType::FarPlane) = ezPlane::MakeFromNormalAndPoint(farPlane.m_vNormal, view.GetCullingCamera()->GetCenterPosition() + farPlane.m_vNormal * cvar_SpatialCullingOcclusionFarPlane.GetValue()); // only use occluders closer than this

  ezRasterizerView* pRasterizer = PrepareOcclusionCulling(limitedFrustum, view);
  EZ_SCOPE_EXIT(g_pRasterizerViewPool->ReturnRasterizerView(pRasterizer));

  const ezVisibilityState::Enum visType = bIsMainView ? ezVisibilityState::Direct : ezVisibilityState::Indirect;

  if (pRasterizer != nullptr && pRasterizer->HasRasterizedAnyOccluders())
  {
    auto IsOccluded = [=](const ezSimdBBox& aabb)
    {
      // grow the bbox by some percent to counter the lower precision of the occlusion buffer

      const ezSimdVec4f c = aabb.GetCenter();
      const ezSimdVec4f e = aabb.GetHalfExtents();
      const ezSimdBBox aabb2 = ezSimdBBox::MakeFromCenterAndHalfExtents(c, e.CompMul(ezSimdVec4f(1.0f + cvar_SpatialCullingOcclusionBoundsInlation)));

      return !pRasterizer->IsVisible(aabb2);
    };

    m_VisibleObjects.Clear();
    view.GetWorld()->GetSpatialSystem()->FindVisibleObjects(frustum, queryParams, m_VisibleObjects, IsOccluded, visType);
  }
  else
  {
    m_VisibleObjects.Clear();
    view.GetWorld()->GetSpatialSystem()->FindVisibleObjects(frustum, queryParams, m_VisibleObjects, {}, visType);
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  if (pRasterizer)
  {
    if (view.GetCameraUsageHint() == ezCameraUsageHint::EditorView || view.GetCameraUsageHint() == ezCameraUsageHint::MainView)
    {
      PreviewOcclusionBuffer(*pRasterizer, view);
    }
  }
#endif

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  ezViewHandle hView = view.GetHandle();

  if (cvar_SpatialCullingVis && bIsMainView)
  {
    ezDebugRenderer::DrawLineFrustum(view.GetWorld(), frustum, ezColor::LimeGreen, false);
  }

  if (bRecordStats)
  {
    ezStringBuilder sb;

    ezDebugRenderer::DrawInfoText(hView, ezDebugTextPlacement::TopLeft, "VisCulling", "Visibility Culling Stats", ezColor::LimeGreen);

    sb.SetFormat("Total Num Objects: {0}", stats.m_uiTotalNumObjects);
    ezDebugRenderer::DrawInfoText(hView, ezDebugTextPlacement::TopLeft, "VisCulling", sb, ezColor::LimeGreen);

    sb.SetFormat("Num Objects Tested: {0}", stats.m_uiNumObjectsTested);
    ezDebugRenderer::DrawInfoText(hView, ezDebugTextPlacement::TopLeft, "VisCulling", sb, ezColor::LimeGreen);

    sb.SetFormat("Num Objects Passed: {0}", stats.m_uiNumObjectsPassed);
    ezDebugRenderer::DrawInfoText(hView, ezDebugTextPlacement::TopLeft, "VisCulling", sb, ezColor::LimeGreen);

    // Exponential moving average for better readability.
    m_AverageCullingTime = ezMath::Lerp(m_AverageCullingTime, stats.m_TimeTaken, 0.05f);

    sb.SetFormat("Time Taken: {0}ms", m_AverageCullingTime.GetMilliseconds());
    ezDebugRenderer::DrawInfoText(hView, ezDebugTextPlacement::TopLeft, "VisCulling", sb, ezColor::LimeGreen);

    view.GetWorld()->GetSpatialSystem()->GetInternalStats(sb);
    ezDebugRenderer::DrawInfoText(hView, ezDebugTextPlacement::TopLeft, "VisCulling", sb, ezColor::AntiqueWhite);
  }
#endif
}

void ezRenderPipeline::EnqueueRenderGraph(ezRenderContext* pRenderContext)
{
  ezLogBlock b("EnqueueRenderGraph");

  EZ_PROFILE_SCOPE(m_sName.GetData());

  ezUInt32 uiDataIndex = ezRenderWorld::GetDataIndexForRendering();
  auto& data = m_Data[uiDataIndex];

  EZ_ASSERT_DEV(m_PipelineState >= PipelineState::Initialized, "Pipeline must be rebuild before rendering.");
  m_PipelineState = PipelineState::Initialized;

  if (!RebuildRenderGraph(data.GetViewData(), data.GetCamera()))
  {
    ezLog::Error("Failed call to RebuildRenderGraph, pipeline rendering aborted");
    return;
  }

  if (m_pRenderGraph->ValidateImportedResources().Failed())
  {
    ezLog::Error("Imported resources are no longer valid, pipeline rendering aborted");
    m_PipelineState = PipelineState::RebuildError;
    return;
  }

  // Apply view dependencies: import shared textures with the required initial state.
  for (const ezTextureDependency& dep : data.GetTextureViewDependencies())
  {
    m_pRenderGraph->ImportTexture(dep.m_hTexture, dep.m_RequiredState, dep.m_Stage);
  }

  // Apply view dependencies: import shared buffers with the required initial state.
  for (const ezBufferDependency& dep : data.GetBufferViewDependencies())
  {
    m_pRenderGraph->ImportBuffer(dep.m_hBuffer, dep.m_RequiredState, dep.m_Stage);
  }

  EZ_ASSERT_DEV(m_CurrentRenderThread == (ezThreadID)0, "Render must not be called from multiple threads.");
  m_CurrentRenderThread = ezThreadUtils::GetCurrentThreadID();

  EZ_ASSERT_DEV(m_uiLastRenderFrame != ezRenderWorld::GetFrameCounter(), "Render must not be called multiple times per frame.");
  m_uiLastRenderFrame = ezRenderWorld::GetFrameCounter();
  m_pRenderGraph->SetUserName(m_sName);
  ezRenderGraphManager::EnqueueRenderGraph(m_pRenderGraph);
}

void ezRenderPipeline::UpdateRenderContext(ezRenderGraphContext& ctx)
{
  EZ_ASSERT_DEV(m_CurrentRenderThread == ezThreadUtils::GetCurrentThreadID(), "Graph executed on wrong thread");
  auto pRenderContext = ctx.GetRenderContext();
  auto& data = m_Data[ezRenderWorld::GetDataIndexForRendering()];
  const ezCamera* pCamera = &data.GetCamera();
  const ezViewData* pViewData = &data.GetViewData();

  auto& gc = pRenderContext->WriteGlobalConstants();
  for (int i = 0; i < 2; ++i)
  {
    gc.CameraToScreenMatrix[i] = pViewData->m_ProjectionMatrix[i];
    gc.ScreenToCameraMatrix[i] = pViewData->m_InverseProjectionMatrix[i];
    gc.WorldToCameraMatrix[i] = pViewData->m_ViewMatrix[i];
    gc.CameraToWorldMatrix[i] = pViewData->m_InverseViewMatrix[i];
    gc.WorldToScreenMatrix[i] = pViewData->m_ViewProjectionMatrix[i];
    gc.ScreenToWorldMatrix[i] = pViewData->m_InverseViewProjectionMatrix[i];
  }

  const ezRectFloat& viewport = pViewData->m_ViewPortRect;
  gc.ViewportSize = ezVec4(viewport.width, viewport.height, 1.0f / viewport.width, 1.0f / viewport.height);

  float fNear = pCamera->GetNearPlane();
  float fFar = pCamera->GetFarPlane();
  gc.ClipPlanes = ezVec4(fNear, fFar, 1.0f / fFar, 0.0f);

  const bool bIsShadowPass = pViewData->m_CameraUsageHint == ezCameraUsageHint::Shadow;
  const bool bIsDirectionalLightShadow = bIsShadowPass && pCamera->IsOrthographic();
  gc.MaxZValue = bIsDirectionalLightShadow ? 0.0f : ezMath::MinValue<float>();

  gc.Exposure = pCamera->GetExposure();
  gc.RenderPass = ezViewRenderMode::GetRenderPassForShader(pViewData->m_ViewRenderMode);
  gc.IsShadowPass = bIsShadowPass;

  pRenderContext->SetGlobalAndWorldTimeConstants(data.GetWorldTime());

  m_RenderViewContext.m_pPipeline = this;
  m_RenderViewContext.m_pCamera = pCamera;
  m_RenderViewContext.m_pViewData = pViewData;
  m_RenderViewContext.m_pRenderContext = pRenderContext;
  m_RenderViewContext.m_pWorldDebugContext = &data.GetWorldDebugContext();
  m_RenderViewContext.m_pViewDebugContext = &data.GetViewDebugContext();

  EZ_ASSERT_DEBUG(ezWorld::GetWorld(data.GetWorldHandle()) != nullptr, "Trying to render a deleted world");

  // Set camera mode permutation variable here since it doesn't change throughout the frame
  static ezHashedString sCameraMode = ezMakeHashedString("CAMERA_MODE");
  static ezHashedString sOrtho = ezMakeHashedString("CAMERA_MODE_ORTHO");
  static ezHashedString sPerspective = ezMakeHashedString("CAMERA_MODE_PERSPECTIVE");
  static ezHashedString sStereo = ezMakeHashedString("CAMERA_MODE_STEREO");

  static ezHashedString sClipSpaceFlipped = ezMakeHashedString("CLIP_SPACE_FLIPPED");
  static ezHashedString sTrue = ezMakeHashedString("TRUE");
  static ezHashedString sFalse = ezMakeHashedString("FALSE");

  if (pCamera->IsOrthographic())
    pRenderContext->SetShaderPermutationVariable(sCameraMode, sOrtho);
  else if (pCamera->IsStereoscopic())
    pRenderContext->SetShaderPermutationVariable(sCameraMode, sStereo);
  else
    pRenderContext->SetShaderPermutationVariable(sCameraMode, sPerspective);

  EZ_ASSERT_DEV(pCamera->IsStereoscopic() == false || ezGALDevice::GetDefaultDevice()->GetCapabilities().m_bSupportsVSRenderTargetArrayIndex, "Vertex shader render target index must be supported for stereo rendering.");

  pRenderContext->SetShaderPermutationVariable(sClipSpaceFlipped, ezClipSpaceYMode::RenderToTextureDefault == ezClipSpaceYMode::Flipped ? sTrue : sFalse);

  // Also set pipeline specific permutation vars
  for (auto& var : m_PermutationVars)
  {
    pRenderContext->SetShaderPermutationVariable(var.m_sName, var.m_sValue);
  }

  // Apply bindings
  ezBindGroupBuilder& bindGroup = pRenderContext->GetBindGroup(EZ_GAL_BIND_GROUP_FRAME);
  for (const ezSamplerBinding& binding : data.GetSamplerBindings())
  {
    bindGroup.BindSampler(binding.m_sSlotName, binding.m_Sampler.m_hSampler);
  }
  for (const ezBufferBinding& binding : data.GetBufferBindings())
  {
    bindGroup.BindBuffer(binding.m_sSlotName, binding.m_Buffer.m_hBuffer, binding.m_Buffer.m_BufferRange, binding.m_Buffer.m_OverrideTexelBufferFormat);
  }
  for (const ezTextureBinding& binding : data.GetTextureBindings())
  {
    bindGroup.BindTexture(binding.m_sSlotName, binding.m_Texture.m_hTexture, binding.m_Texture.m_TextureRange, binding.m_Texture.m_OverrideViewFormat, binding.m_Texture.m_OverrideViewType);
  }

  {
    ezRenderWorldRenderEvent renderEvent;
    renderEvent.m_Type = ezRenderWorldRenderEvent::Type::BeforePipelineExecution;
    renderEvent.m_pRenderViewContext = &m_RenderViewContext;
    renderEvent.m_uiFrameCounter = ezRenderWorld::GetFrameCounter();

    EZ_PROFILE_SCOPE("BeforePipelineExecution");
    ezRenderWorld::s_RenderEvent.Broadcast(renderEvent);
  }
}

const ezExtractedRenderData& ezRenderPipeline::GetRenderData() const
{
  return m_Data[ezRenderWorld::GetDataIndexForRendering()];
}

void ezRenderPipeline::AddViewDependency(ezGALTextureHandle hTexture, ezBitflags<ezGALResourceState> requiredState, ezBitflags<ezGALShaderStageFlags> stage)
{
  m_Data[ezRenderWorld::GetDataIndexForExtraction()].AddViewDependency(hTexture, requiredState, stage);
}

void ezRenderPipeline::AddViewDependency(ezGALBufferHandle hBuffer, ezBitflags<ezGALResourceState> requiredState, ezBitflags<ezGALShaderStageFlags> stage)
{
  m_Data[ezRenderWorld::GetDataIndexForExtraction()].AddViewDependency(hBuffer, requiredState, stage);
}

ezRenderDataBatchList ezRenderPipeline::GetRenderDataBatchesWithCategory(ezRenderData::Category category) const
{
  auto& data = m_Data[ezRenderWorld::GetDataIndexForRendering()];
  return data.GetRenderDataBatchesWithCategory(category);
}

ezArrayPtr<const ezTextureDependency> ezRenderPipeline::GetTextureDependenciesWithCategory(ezRenderData::Category category) const
{
  auto& data = m_Data[ezRenderWorld::GetDataIndexForRendering()];
  return data.GetTextureDependenciesWithCategory(category);
}

ezArrayPtr<const ezBufferDependency> ezRenderPipeline::GetBufferDependenciesWithCategory(ezRenderData::Category category) const
{
  auto& data = m_Data[ezRenderWorld::GetDataIndexForRendering()];
  return data.GetBufferDependenciesWithCategory(category);
}

ezUInt32 ezRenderPipeline::AddRenderDataProcessor(RenderDataProcessor processor)
{
  ezUInt32 uiIndex = m_RenderDataProcessors.GetCount();
  m_RenderDataProcessors.PushBack(processor);
  return uiIndex;
}

void ezRenderPipeline::CreateDgmlGraph(ezDGMLGraph& ref_graph)
{
  /*
  ezStringBuilder sTmp;
  ezHashTable<const ezRenderPipelineNode*, ezUInt32> nodeMap;
  nodeMap.Reserve(m_Passes.GetCount() + m_TextureUsage.GetCount() * 3);
  for (ezUInt32 p = 0; p < m_Passes.GetCount(); ++p)
  {
    const auto& pPass = m_Passes[p];
    sTmp.SetFormat("#{}: {}", p, ezStringUtils::IsNullOrEmpty(pPass->GetName()) ? pPass->GetDynamicRTTI()->GetTypeName() : pPass->GetName());

    ezDGMLGraph::NodeDesc nd;
    nd.m_Color = ezColor::Gray;
    nd.m_Shape = ezDGMLGraph::NodeShape::Rectangle;
    ezUInt32 uiGraphNode = ref_graph.AddNode(sTmp, &nd);
    nodeMap.Insert(pPass.Borrow(), uiGraphNode);
  }

  for (ezUInt32 i = 0; i < m_TextureUsage.GetCount(); ++i)
  {
    const TextureUsageData& data = m_TextureUsage[i];

    for (const ezRenderPipelinePassConnection* pCon : data.m_UsedBy)
    {
      ezDGMLGraph::NodeDesc nd;
      nd.m_Color = data.m_pTextureProvider ? ezColor::Black : ezColorScheme::GetColor(static_cast<ezColorScheme::Enum>(i % ezColorScheme::Count), 4);
      nd.m_Shape = ezDGMLGraph::NodeShape::RoundedRectangle;

      ezStringBuilder sFormat;
      if (!ezReflectionUtils::EnumerationToString(ezGetStaticRTTI<ezGALResourceFormat>(), pCon->m_Desc.m_Format, sFormat, ezReflectionUtils::EnumConversionMode::ValueNameOnly))
      {
        sFormat.SetFormat("Unknown Format {}", (int)pCon->m_Desc.m_Format);
      }
      sTmp.SetFormat("{} #{}: {}x{}:{}, MSAA:{}, {}Format: {}", data.m_pTextureProvider ? "External" : "PoolTexture", i, pCon->m_Desc.m_uiWidth, pCon->m_Desc.m_uiHeight, pCon->m_Desc.m_uiArraySize, (int)pCon->m_Desc.m_SampleCount, ezGALResourceFormat::IsDepthFormat(pCon->m_Desc.m_Format) ? "Depth" : "Color", sFormat);
      ezUInt32 uiTextureNode = ref_graph.AddNode(sTmp, &nd);

      ezUInt32 uiOutputNode = *nodeMap.GetValue(pCon->m_pOutput->m_pParent);
      ref_graph.AddConnection(uiOutputNode, uiTextureNode, pCon->m_pOutput->m_pParent->GetPinName(pCon->m_pOutput));
      for (const ezRenderPipelineNodePin* pInput : pCon->m_Inputs)
      {
        ezUInt32 uiInputNode = *nodeMap.GetValue(pInput->m_pParent);
        ref_graph.AddConnection(uiTextureNode, uiInputNode, pInput->m_pParent->GetPinName(pInput));
      }
    }
  }
  */
}

ezRasterizerView* ezRenderPipeline::PrepareOcclusionCulling(const ezFrustum& frustum, const ezView& view)
{
#if EZ_ENABLED(EZ_PLATFORM_ARCH_X86)
  if (!cvar_SpatialCullingOcclusionEnable)
    return nullptr;

  auto& cpuFeatures = ezSystemInformation::Get().GetCpuFeatures();
  if (!cpuFeatures.IsAvx1Available() || !cpuFeatures.HW_FMA3)
    return nullptr;

  ezRasterizerView* pRasterizer = nullptr;

  // extract all occlusion geometry from the scene
  EZ_PROFILE_SCOPE("PrepareOcclusionCulling");

  pRasterizer = g_pRasterizerViewPool->GetRasterizerView(static_cast<ezUInt32>(view.GetViewport().width / 2), static_cast<ezUInt32>(view.GetViewport().height / 2), (float)view.GetViewport().width / (float)view.GetViewport().height);
  pRasterizer->SetCamera(view.GetCullingCamera());

  {
    EZ_PROFILE_SCOPE("FindOccluders");

    ezSpatialSystem::QueryParams queryParams;
    queryParams.m_uiCategoryBitmask = ezDefaultSpatialDataCategories::OcclusionStatic.GetBitmask() | ezDefaultSpatialDataCategories::OcclusionDynamic.GetBitmask();
    queryParams.m_pIncludeTags = &view.m_IncludeTags;
    queryParams.m_pExcludeTags = &view.m_ExcludeTags;

    m_VisibleObjects.Clear();
    view.GetWorld()->GetSpatialSystem()->FindVisibleObjects(frustum, queryParams, m_VisibleObjects, {}, ezVisibilityState::Indirect);
  }

  pRasterizer->BeginScene();

  {
    EZ_PROFILE_SCOPE("ExtractOccluders");

    for (const ezGameObject* pObj : m_VisibleObjects)
    {
      ezMsgExtractOccluderData msg;
      pObj->SendMessage(msg);

      for (const auto& ed : msg.m_ExtractedOccluderData)
      {
        pRasterizer->AddObject(ed.m_pObject, ed.m_Transform);
      }
    }
  }

  pRasterizer->EndScene();

  return pRasterizer;
#else
  return nullptr;
#endif
}

void ezRenderPipeline::PreviewOcclusionBuffer(const ezRasterizerView& rasterizer, const ezView& view)
{
  if (!cvar_SpatialCullingOcclusionVisView || !rasterizer.HasRasterizedAnyOccluders())
    return;

  EZ_PROFILE_SCOPE("Occlusion::DebugPreview");

  const ezUInt32 uiImgWidth = rasterizer.GetResolutionX();
  const ezUInt32 uiImgHeight = rasterizer.GetResolutionY();

  // get the debug image from the rasterizer
  ezDynamicArray<ezColorLinearUB> fb;
  fb.SetCountUninitialized(uiImgWidth * uiImgHeight);
  rasterizer.ReadBackFrame(fb);

  const float w = (float)uiImgWidth;
  const float h = (float)uiImgHeight;
  ezRectFloat rectInPixel1 = ezRectFloat(5.0f, 5.0f, w + 10, h + 10);
  ezRectFloat rectInPixel2 = ezRectFloat(10.0f, 10.0f, w, h);

  ezDebugRenderer::Draw2DRectangle(view.GetHandle(), rectInPixel1, 0.0f, ezColor::MediumPurple);

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  // check whether we need to re-create the texture
  if (!m_hOcclusionDebugViewTexture.IsInvalidated())
  {
    const ezGALTexture* pTexture = pDevice->GetTexture(m_hOcclusionDebugViewTexture);

    if (pTexture->GetDescription().m_uiWidth != uiImgWidth ||
        pTexture->GetDescription().m_uiHeight != uiImgHeight)
    {
      pDevice->DestroyTexture(m_hOcclusionDebugViewTexture);
    }
  }

  // create the texture
  if (m_hOcclusionDebugViewTexture.IsInvalidated())
  {
    ezGALTextureCreationDescription desc;
    desc.m_uiWidth = uiImgWidth;
    desc.m_uiHeight = uiImgHeight;
    desc.m_Format = ezGALResourceFormat::RGBAUByteNormalized;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_hOcclusionDebugViewTexture = pDevice->CreateTexture(desc);
  }

  // upload the image to the texture
  {
    ezGALSystemMemoryDescription sourceData;
    sourceData.m_pData = fb.GetByteArrayPtr();
    sourceData.m_uiRowPitch = uiImgWidth * sizeof(ezColorLinearUB);

    pDevice->UpdateTextureForNextFrame(m_hOcclusionDebugViewTexture, sourceData);
  }

  ezDebugRenderer::Draw2DRectangle(view.GetHandle(), rectInPixel2, 0.0f, ezColor::White, m_hOcclusionDebugViewTexture, ezVec2(1, -1));
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderPipeline);
