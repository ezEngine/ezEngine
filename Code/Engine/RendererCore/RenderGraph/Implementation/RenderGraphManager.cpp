#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/RenderGraph/RenderGraphManager.h>

#include <Foundation/Configuration/Startup.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderGraph/RenderGraph.h>
#include <RendererCore/RenderGraph/RenderGraphInspectionInfo.h>
#include <RendererCore/RenderGraph/RenderGraphPassObserver.h>
#include <RendererCore/RenderGraph/RenderGraphResourcePool.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Utils/ResourceStateTracker.h>

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, RenderGraphManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    ezRenderGraphManager::OnEngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    ezRenderGraphManager::OnEngineShutdown();
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

ezEvent<const ezRenderGraphRenderEvent&, ezMutex> ezRenderGraphManager::s_RenderEvent;
ezMutex ezRenderGraphManager::s_Mutex;
ezDynamicArray<ezSharedPtr<ezRenderGraph>> ezRenderGraphManager::s_EnqueuedRenderGraphs[3];
ezDynamicArray<ezRenderGraph*> ezRenderGraphManager::s_AllRenderGraphs[3];
ezUniquePtr<ezRenderGraphResourcePool> ezRenderGraphManager::s_pPool;
ezUniquePtr<ezGALResourceStateTracker> ezRenderGraphManager::s_pStateTracker;
ezDynamicArray<ezSharedPtr<ezRenderGraphPassObserver>> ezRenderGraphManager::s_Observers;
ezSharedPtr<ezRenderGraph> ezRenderGraphManager::s_pObserverGraph;
ezDynamicArray<ezRenderGraph*> ezRenderGraphManager::s_ExecutingGraphs;
ezDynamicArray<ezRenderGraphPassObserver*> ezRenderGraphManager::s_ExecutingObservers;
ezUInt32 ezRenderGraphManager::s_uiCurrentGraphIndex = 0;
ezUInt32 ezRenderGraphManager::s_uiCurrentPassIndex = 0;


void ezRenderGraphManager::OnEngineStartup()
{
  ezGALDevice::s_Events.AddEventHandler(ezMakeDelegate(&ezRenderGraphManager::GALDeviceEventHandler));
  ezGALCommandEncoder::s_TextureBarrierValidationFailed.AddEventHandler(ezMakeDelegate(&ezRenderGraphManager::PrintTextureResourceHistory));
  ezGALCommandEncoder::s_BufferBarrierValidationFailed.AddEventHandler(ezMakeDelegate(&ezRenderGraphManager::PrintBufferResourceHistory));
  InitPool(ezGALDevice::GetDefaultDevice());
}

void ezRenderGraphManager::OnEngineShutdown()
{
  s_pObserverGraph = nullptr;

  DeinitPool(ezGALDevice::GetDefaultDevice());
  ezGALDevice::s_Events.RemoveEventHandler(ezMakeDelegate(&ezRenderGraphManager::GALDeviceEventHandler));
  ezGALCommandEncoder::s_TextureBarrierValidationFailed.RemoveEventHandler(ezMakeDelegate(&ezRenderGraphManager::PrintTextureResourceHistory));
  ezGALCommandEncoder::s_BufferBarrierValidationFailed.RemoveEventHandler(ezMakeDelegate(&ezRenderGraphManager::PrintBufferResourceHistory));
  s_pPool.Clear();
  for (auto& bucket : s_EnqueuedRenderGraphs)
    bucket.Clear();
  EZ_ASSERT_DEV(s_AllRenderGraphs[ezRenderGraphPhase::PreRender].IsEmpty(), "Not all PreRender-phase render graphs were destroyed before shutdown.");
  EZ_ASSERT_DEV(s_AllRenderGraphs[ezRenderGraphPhase::Render].IsEmpty(), "Not all render-phase render graphs were destroyed before shutdown.");
  EZ_ASSERT_DEV(s_AllRenderGraphs[ezRenderGraphPhase::PostRender].IsEmpty(), "Not all post-render-phase render graphs were destroyed before shutdown.");

  for (ezUInt32 i = s_Observers.GetCount(); i > 0; --i)
  {
    if (s_Observers[i - 1]->GetRefCount() == 1)
    {
      s_Observers.RemoveAtAndSwap(i - 1);
      continue;
    }
  }

  EZ_ASSERT_DEV(s_Observers.IsEmpty(), "Not render graph observers were destroyed before shutdown.");
}

void ezRenderGraphManager::GALDeviceEventHandler(const ezGALDeviceEvent& e)
{
  switch (e.m_Type)
  {
    case ezGALDeviceEvent::BeforeBeginFrame:
      BeginFrame();
      break;
    case ezGALDeviceEvent::AfterEndFrame:
    {
      EZ_ASSERT_DEV(s_EnqueuedRenderGraphs[0].IsEmpty() && s_EnqueuedRenderGraphs[1].IsEmpty() && s_EnqueuedRenderGraphs[2].IsEmpty(),
        "RenderAllGraphs must be called before end frame or a graph was registered after rendering finished.");
    }
    break;
    default:
      break;
  }
}

void ezRenderGraphManager::InitPool(ezGALDevice* pDevice)
{
  EZ_ASSERT_DEBUG(s_pPool == nullptr, "Render graph resource pool already initialized");
  s_pPool = EZ_DEFAULT_NEW(ezRenderGraphResourcePool, pDevice);
  s_pStateTracker = EZ_DEFAULT_NEW(ezGALResourceStateTracker, pDevice);
}

void ezRenderGraphManager::DeinitPool(ezGALDevice* pDevice)
{
  s_pStateTracker.Clear();
  for (auto& bucket : s_EnqueuedRenderGraphs)
    bucket.Clear();
  s_pPool.Clear();
}

void ezRenderGraphManager::EnqueueRenderGraph(const ezSharedPtr<ezRenderGraph>& pRenderGraph)
{
  EZ_LOCK(s_Mutex);
  EZ_ASSERT_DEV(pRenderGraph->m_pCurrentPass == nullptr, "Can't enqueue a render graph that still has a pass open for recording");
  EZ_ASSERT_DEV(pRenderGraph->m_RenderGraphState == ezRenderGraph::RenderGraphState::Recording, "Only graphs in the recording state can be enqueued");
  pRenderGraph->m_RenderGraphState = ezRenderGraph::RenderGraphState::Enqueued;
  s_EnqueuedRenderGraphs[pRenderGraph->m_Phase.GetValue()].PushBack(pRenderGraph);
}

ezSharedPtr<ezRenderGraph> ezRenderGraphManager::CreateRenderGraph(ezStringView sName, ezEnum<ezRenderGraphPhase> phase)
{
  EZ_LOCK(s_Mutex);
  ezSharedPtr<ezRenderGraph> pGraph = EZ_DEFAULT_NEW(ezRenderGraph, ezGALDevice::GetDefaultDevice(), sName, phase);
  s_AllRenderGraphs[phase.GetValue()].PushBack(pGraph.Borrow());
  return pGraph;
}

void ezRenderGraphManager::BeginFrame()
{
  EZ_LOCK(s_Mutex);
  EZ_PROFILE_SCOPE("ezRenderGraphManager::BeginFrame");
  for (auto& bucket : s_EnqueuedRenderGraphs)
    bucket.Clear();
  s_pStateTracker->Clear();
}

void ezRenderGraphManager::ExecuteRenderGraphs(ezGALDevice* pDevice)
{
  EZ_PROFILE_SCOPE("ezRenderGraphManager::ExecuteRenderGraphs");
  if (!s_pPool)
    return;

  {
    ezRenderGraphRenderEvent ev;
    ev.m_Type = ezRenderGraphRenderEvent::Type::BeginRender;
    s_RenderEvent.Broadcast(ev);
  }

  s_ExecutingGraphs.Clear();
  {
    EZ_PROFILE_SCOPE("CompileRenderGraphs");
    EZ_LOCK(s_Mutex);
    // Compile all registered graphs in phase order.
    for (auto& bucket : s_EnqueuedRenderGraphs)
    {
      for (auto& pRenderGraph : bucket)
      {
        // EnqueueRenderGraph takes a reference to the graph. If we are holding the last reference, the graph is no longer owned externally and must not be used anymore.
        if (pRenderGraph->GetRefCount() > 1 && pRenderGraph->Compile().Succeeded())
          s_ExecutingGraphs.PushBack(pRenderGraph.Borrow());
      }
    }
    s_ExecutingObservers.Clear();
    for (ezUInt32 i = s_Observers.GetCount(); i > 0; --i)
    {
      if (s_Observers[i - 1]->GetRefCount() == 1)
      {
        s_Observers.RemoveAtAndSwap(i - 1);
        continue;
      }
      // Apply any pending requests while we hold the mutex.
      // After this point, m_Request is only accessed on the render thread.
      s_Observers[i - 1]->ApplyPendingRequest();
      s_ExecutingObservers.PushBack(s_Observers[i - 1].Borrow());
    }

    // Create observer render graph
    if (!s_ExecutingObservers.IsEmpty())
    {
      if (s_pObserverGraph == nullptr)
      {
        s_pObserverGraph = CreateRenderGraph("__OBSERVER__", ezRenderGraphPhase::PostRender);
      }

      for (auto* pObserver : s_ExecutingObservers)
      {
        pObserver->RecordPreview(*s_pObserverGraph.Borrow());
      }

      if (s_pObserverGraph->Compile().Succeeded())
        s_ExecutingGraphs.PushBack(s_pObserverGraph.Borrow());
    }
  }

  {
    EZ_PROFILE_SCOPE("ComputeBarriers");
    for (auto pRenderGraph : s_ExecutingGraphs)
    {
      // Collect observers for this graph.
      ezHybridArray<ezRenderGraphPassObserver*, 4> graphObservers;
      for (auto* pObserver : s_ExecutingObservers)
      {
        if (pObserver->m_pGraph == pRenderGraph)
        {
          pObserver->Reset();
          graphObservers.PushBack(pObserver);
        }
      }

      pRenderGraph->ComputeBarriers(*s_pStateTracker.Borrow(), graphObservers);
    }
  }



  ezHybridArray<ezGALTextureBarrier, 8> textureBarriers;
  s_pStateTracker->RevertTextureState([&](const ezGALTextureBarrier& barrier)
    { textureBarriers.PushBack(barrier); });
  ezHybridArray<ezGALBufferBarrier, 8> bufferBarriers;
  s_pStateTracker->RevertBufferState([&](const ezGALBufferBarrier& barrier)
    { bufferBarriers.PushBack(barrier); });

  // Execute all registered graphs.
  auto pEncoder = pDevice->BeginCommands("RenderGraph");
  ezRenderGraphContext ctx(pEncoder, pDevice, ezRenderContext::GetDefaultInstance());
  for (s_uiCurrentGraphIndex = 0; s_uiCurrentGraphIndex < s_ExecutingGraphs.GetCount(); ++s_uiCurrentGraphIndex)
  {
    // Collect valid observers for this graph.
    ezHybridArray<ezRenderGraphPassObserver*, 4> graphObservers;
    for (auto* pObserver : s_ExecutingObservers)
    {
      if (pObserver->m_bValid && pObserver->m_pGraph == s_ExecutingGraphs[s_uiCurrentGraphIndex])
        graphObservers.PushBack(pObserver);
    }

    s_ExecutingGraphs[s_uiCurrentGraphIndex]->Execute(ctx, graphObservers);
  }
  pEncoder->TextureBarrier(textureBarriers);
  pEncoder->BufferBarrier(bufferBarriers);

  pDevice->EndCommands(pEncoder);
  s_pPool->EndFrame();
  s_pStateTracker->Clear();

  {
    ezRenderGraphRenderEvent ev;
    ev.m_Type = ezRenderGraphRenderEvent::Type::EndRender;
    s_RenderEvent.Broadcast(ev);
  }

  for (s_uiCurrentGraphIndex = 0; s_uiCurrentGraphIndex < s_ExecutingGraphs.GetCount(); ++s_uiCurrentGraphIndex)
  {
    // Graphs can only be executed once and then need to be re-recorded.
    s_ExecutingGraphs[s_uiCurrentGraphIndex]->ResetInternal(ezRenderGraph::RenderGraphState::Recording);
  }
  s_ExecutingGraphs.Clear();

  for (auto& bucket : s_EnqueuedRenderGraphs)
    bucket.Clear();
}

void ezRenderGraphManager::GetExecutionSummary(ezRenderGraphInspectionSummary& out_summary)
{
  out_summary.m_RenderGraphs.Clear();
  out_summary.m_AvailableSwapChains.Clear();

  if (ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice())
  {
    ezDynamicArray<ezGALSwapChainHandle> swapChains;
    pDevice->GetAllSwapChains(swapChains);
    out_summary.m_AvailableSwapChains.Reserve(swapChains.GetCount());
    for (ezGALSwapChainHandle hSwapChain : swapChains)
    {
      ezRenderGraphSwapChainSummary& swapChainSummary = out_summary.m_AvailableSwapChains.ExpandAndGetRef();
      swapChainSummary.m_uiSwapChainId = GetSwapChainId(hSwapChain);

      if (const ezGALSwapChain* pSwapChain = pDevice->GetSwapChain(hSwapChain))
      {
        const ezSizeU32 size = pSwapChain->GetCurrentSize();
        swapChainSummary.m_uiWidth = size.width;
        swapChainSummary.m_uiHeight = size.height;
      }
    }
  }

  for (ezUInt32 i = 0; i < 3; ++i)
  {
    for (ezRenderGraph* pGraph : s_AllRenderGraphs[i])
    {
      if (pGraph == s_pObserverGraph.Borrow())
        continue;
      ezRenderGraphExecutionSummary& summary = out_summary.m_RenderGraphs.ExpandAndGetRef();
      summary.m_uiRenderGraphId = GetRenderGraphId(pGraph);
      summary.m_sGraphName = pGraph->GetGraphName();
      summary.m_sUserName = pGraph->GetUserName();
      summary.m_Phase = pGraph->m_Phase;

      summary.m_uiExecutionOrder = -1;
      for (ezUInt32 j = 0; j < s_EnqueuedRenderGraphs[i].GetCount(); ++j)
      {
        if (s_EnqueuedRenderGraphs[i][j].Borrow() == pGraph)
        {
          summary.m_uiExecutionOrder = j;
          break;
        }
      }
    }
  }
}

ezResult ezRenderGraphManager::GetRenderGraphInspectionInfo(ezUInt64 uiRenderGraphId, ezRenderGraphInspectionInfo& out_inspectionInfo)
{
  if (ezRenderGraph* pGraph = GetRenderGraphById(uiRenderGraphId))
  {
    return pGraph->GetInspectionInfo(out_inspectionInfo);
  }
  return EZ_FAILURE;
}

ezUInt64 ezRenderGraphManager::GetRenderGraphId(ezRenderGraph* pGraph)
{
  return reinterpret_cast<ezUInt64>(pGraph);
}

ezRenderGraph* ezRenderGraphManager::GetRenderGraphById(ezUInt64 uiRenderGraphId)
{
  ezRenderGraph* pGraph = reinterpret_cast<ezRenderGraph*>(uiRenderGraphId);
  for (ezUInt32 i = 0; i < 3; ++i)
  {
    for (ezRenderGraph* pGraph2 : s_AllRenderGraphs[i])
    {
      if (pGraph == pGraph2)
      {
        return pGraph2;
      }
    }
  }
  return nullptr;
}

ezUInt32 ezRenderGraphManager::GetSwapChainId(ezGALSwapChainHandle hSwapChain)
{
  return hSwapChain.GetInternalID().m_Data;
}

ezGALSwapChainHandle ezRenderGraphManager::GetSwapChainById(ezUInt32 uiSwapChainId)
{
  ezGALSwapChainHandle hSwapChain{ezGALSwapChainHandle::IdType(uiSwapChainId)};
  if (ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice())
  {
    if (pDevice->GetSwapChain(hSwapChain) != nullptr)
    {
      return hSwapChain;
    }
  }
  return ezGALSwapChainHandle();
}

ezSharedPtr<ezRenderGraphPassObserver> ezRenderGraphManager::CreateObserver()
{
  ezSharedPtr<ezRenderGraphPassObserver> observer = EZ_DEFAULT_NEW(ezRenderGraphPassObserver, ezGALDevice::GetDefaultDevice());
  s_Observers.PushBack(observer);
  return observer;
}

void ezRenderGraphManager::OnGraphDestroyed(ezRenderGraph* pGraph)
{
  s_AllRenderGraphs[pGraph->m_Phase.GetValue()].RemoveAndSwap(pGraph);
}

ezRenderGraphResourcePool* ezRenderGraphManager::GetResourcePool()
{
  return s_pPool.Borrow();
}

namespace
{
  bool RangesOverlap(const ezGALTextureRange& a, const ezGALTextureSubresource& subResource)
  {
    const ezUInt32 uiSliceEnd = (ezUInt32)a.m_uiBaseArraySlice + (ezUInt32)a.m_uiArraySlices - 1;
    if (subResource.m_uiArraySlice < (ezUInt32)a.m_uiBaseArraySlice || subResource.m_uiArraySlice > uiSliceEnd)
      return false;

    const ezUInt32 uiMipEnd = (ezUInt32)a.m_uiBaseMipLevel + (ezUInt32)a.m_uiMipLevels - 1;
    if (subResource.m_uiMipLevel < (ezUInt32)a.m_uiBaseMipLevel || subResource.m_uiMipLevel > uiMipEnd)
      return false;

    return true;
  }
} // namespace

void ezRenderGraphManager::PrintTextureResourceHistory(const ezTextureValidationError& error)
{
  ezLog::Error("Bind group '{}' binding '{}': texture sub-resource [mip={}, slice={}] state mismatch. Tracked: {} [{}], Expected: {} [{}]",
    error.m_uiBindGroup, error.m_sBinding.GetData(), error.m_failedSubResource.m_uiMipLevel, error.m_failedSubResource.m_uiArraySlice, ezArgEnum(error.m_actualState), ezArgEnum(error.m_actualStages), ezArgEnum(error.m_expectedState), ezArgEnum(error.m_expectedStages));


  if (s_ExecutingGraphs.IsEmpty())
    return;

  // First check if the texture is known to any graphs.


  ezLog::Error("=== Texture Resource History (handle {}, range=[mip={}, slice={}]) ===", error.m_hTexture.GetInternalID().m_Data, error.m_failedSubResource.m_uiMipLevel, error.m_failedSubResource.m_uiArraySlice);

  for (ezUInt32 uiGraph = 0; uiGraph < s_ExecutingGraphs.GetCount(); ++uiGraph)
  {
    const ezRenderGraph* pGraph = s_ExecutingGraphs[uiGraph];

    auto it = pGraph->m_ImportTextureToHandle.Find(error.m_hTexture);
    if (!it.IsValid())
    {
      if (uiGraph == s_uiCurrentGraphIndex)
      {
        const auto& compiled = pGraph->m_CompiledPasses[s_uiCurrentPassIndex];
        const auto& pass = pGraph->m_Passes[compiled.m_uiOriginalPassIndex];
        const char* szPassName = pGraph->m_PassNames[compiled.m_uiOriginalPassIndex].GetData();
        ezLog::Error("{} Graph {} ({})", uiGraph == s_uiCurrentGraphIndex ? ">" : " ", pGraph->m_sGraphName, pGraph->m_sUserName);
        ezLog::Error("  {} Pass[{}] '{}' ", ">", s_uiCurrentPassIndex, szPassName);
      }
      continue;
    }
    ezStringBuilder sBlock;
    sBlock.SetFormat("{} Graph {} ({})", uiGraph == s_uiCurrentGraphIndex ? ">" : " ", pGraph->m_sGraphName, pGraph->m_sUserName);

    for (ezUInt32 uiPass = 0; uiPass < pGraph->m_CompiledPasses.GetCount(); ++uiPass)
    {
      const auto& compiled = pGraph->m_CompiledPasses[uiPass];
      const auto& pass = pGraph->m_Passes[compiled.m_uiOriginalPassIndex];
      const char* szPassName = pGraph->m_PassNames[compiled.m_uiOriginalPassIndex].GetData();

      bool bHasActivity = false;
      ezStringBuilder sOps;

      ezHybridArray<ezRenderGraph::TextureInfo, 1> overlapped;
      ezHybridArray<ezGALTextureBarrier, 1> overlappedBarriers;
      // Check read textures
      for (const ezRenderGraph::TextureInfo& info : pass.GetReadTextures(pGraph))
      {
        const ezUInt16 resolvedIdx = pGraph->m_TextureToResolvedTexture[info.m_hTexture.m_InternalId.m_InstanceIndex];
        if (pGraph->m_ResolvedTextures[resolvedIdx] == error.m_hTexture && RangesOverlap(info.m_range, error.m_failedSubResource))
        {
          overlapped.PushBack(info);
          bHasActivity = true;
        }
      }

      // Check write textures
      for (const ezRenderGraph::TextureInfo& info : pass.GetWriteTextures(pGraph))
      {
        const ezUInt16 resolvedIdx = pGraph->m_TextureToResolvedTexture[info.m_hTexture.m_InternalId.m_InstanceIndex];
        if (pGraph->m_ResolvedTextures[resolvedIdx] == error.m_hTexture && RangesOverlap(info.m_range, error.m_failedSubResource))
        {
          overlapped.PushBack(info);
          bHasActivity = true;
        }
      }

      // Check barriers for this pass
      for (const ezGALTextureBarrier& barrier : compiled.GetTextureBarriers(pGraph))
      {
        if (barrier.m_hTexture != error.m_hTexture)
          continue;

        if (barrier.m_bAllSubresources)
        {
          overlappedBarriers.PushBack(barrier);
          bHasActivity = true;
        }
        else
        {
          const ezGALTextureRange barrierRange = {
            static_cast<ezUInt16>(barrier.m_Subresource.m_uiArraySlice), 1,
            static_cast<ezUInt8>(barrier.m_Subresource.m_uiMipLevel), 1};
          if (RangesOverlap(barrierRange, error.m_failedSubResource))
          {
            overlappedBarriers.PushBack(barrier);
            bHasActivity = true;
          }
        }
      }

      if (bHasActivity || (uiGraph == s_uiCurrentGraphIndex) && s_uiCurrentPassIndex == uiPass)
      {
        if (!sBlock.IsEmpty())
        {
          ezLog::Error("{}", sBlock);
          sBlock.Clear();
        }
        // EZ_ASSERT_DEBUG(overlapped.GetCount() == 1, ""); // overlappedBarriers.GetCount(), "");
        ezStringBuilder sText;
        const bool bIsCurrent = (uiGraph == s_uiCurrentGraphIndex && uiPass == s_uiCurrentPassIndex);
        sText.SetFormat("  {} Pass[{}] '{}' ", bIsCurrent ? ">" : " ", uiPass, szPassName);
        for (const ezRenderGraph::TextureInfo& info : overlapped)
        {
          sText.AppendFormat(", OP: state={}, stage={}, range=[mip={}+{}, slice={}+{}]",
            ezArgEnum(info.m_access), ezArgEnum(info.m_stage),
            info.m_range.m_uiBaseMipLevel, info.m_range.m_uiMipLevels,
            info.m_range.m_uiBaseArraySlice, info.m_range.m_uiArraySlices);
        }
        for (const ezGALTextureBarrier& barrier : overlappedBarriers)
        {
          sText.AppendFormat(", BARRIER: {} -> {}, stages: {} -> {} (mip={}, slice={})",
            ezArgEnum(barrier.m_StateBefore), ezArgEnum(barrier.m_StateAfter),
            ezArgEnum(barrier.m_StagesBefore), ezArgEnum(barrier.m_StagesAfter),
            barrier.m_Subresource.m_uiMipLevel, barrier.m_Subresource.m_uiArraySlice);
        }

        ezLog::Error("{}", sText.GetView());
      }
    }


    if (!sBlock.IsEmpty())
    {
      ezLog::Error("{} : Imported but no barrier found", sBlock);
    }
  }

  ezLog::Error("=== End Texture Resource History ===");
}

void ezRenderGraphManager::PrintBufferResourceHistory(const ezBufferValidationError& error)
{
  ezLog::Error("Bind group '{}' binding '{}': buffer state mismatch. Tracked: {} [{}], Expected: {} [{}]",
    error.m_uiBindGroup, error.m_sBinding.GetData(), ezArgEnum(error.m_actualState), ezArgEnum(error.m_actualStages), ezArgEnum(error.m_expectedState), ezArgEnum(error.m_expectedStages));

  if (s_ExecutingGraphs.IsEmpty())
    return;

  ezLog::Error("=== Buffer Resource History (handle {}) ===", error.m_hBuffer.GetInternalID().m_Data);

  for (ezUInt32 uiGraph = 0; uiGraph < s_ExecutingGraphs.GetCount(); ++uiGraph)
  {
    const ezRenderGraph* pGraph = s_ExecutingGraphs[uiGraph];

    auto it = pGraph->m_ImportBufferToHandle.Find(error.m_hBuffer);
    if (!it.IsValid())
    {
      if (uiGraph == s_uiCurrentGraphIndex)
      {
        const auto& compiled = pGraph->m_CompiledPasses[s_uiCurrentPassIndex];
        const char* szPassName = pGraph->m_PassNames[compiled.m_uiOriginalPassIndex].GetData();
        ezLog::Error("{} Graph {} ({})", ">", pGraph->m_sGraphName, pGraph->m_sUserName);
        ezLog::Error("  {} Pass[{}] '{}' ", ">", s_uiCurrentPassIndex, szPassName);
      }
      continue;
    }

    ezStringBuilder sBlock;
    sBlock.SetFormat("{} Graph {} ({})", uiGraph == s_uiCurrentGraphIndex ? ">" : " ", pGraph->m_sGraphName, pGraph->m_sUserName);

    for (ezUInt32 uiPass = 0; uiPass < pGraph->m_CompiledPasses.GetCount(); ++uiPass)
    {
      const auto& compiled = pGraph->m_CompiledPasses[uiPass];
      const auto& pass = pGraph->m_Passes[compiled.m_uiOriginalPassIndex];
      const char* szPassName = pGraph->m_PassNames[compiled.m_uiOriginalPassIndex].GetData();

      bool bHasActivity = false;
      ezHybridArray<ezRenderGraph::BufferInfo, 1> overlapped;
      ezHybridArray<ezGALBufferBarrier, 1> overlappedBarriers;

      for (const auto& info : pass.GetReadBuffers(pGraph))
      {
        const ezUInt16 resolvedIdx = pGraph->m_BufferToResolvedBuffer[info.m_hBuffer.m_InternalId.m_InstanceIndex];
        if (pGraph->m_ResolvedBuffers[resolvedIdx] == error.m_hBuffer)
        {
          overlapped.PushBack(info);
          bHasActivity = true;
        }
      }

      for (const auto& info : pass.GetWriteBuffers(pGraph))
      {
        const ezUInt16 resolvedIdx = pGraph->m_BufferToResolvedBuffer[info.m_hBuffer.m_InternalId.m_InstanceIndex];
        if (pGraph->m_ResolvedBuffers[resolvedIdx] == error.m_hBuffer)
        {
          overlapped.PushBack(info);
          bHasActivity = true;
        }
      }

      for (const auto& barrier : compiled.GetBufferBarriers(pGraph))
      {
        if (barrier.m_hBuffer == error.m_hBuffer)
        {
          overlappedBarriers.PushBack(barrier);
          bHasActivity = true;
        }
      }

      if (bHasActivity || ((uiGraph == s_uiCurrentGraphIndex) && s_uiCurrentPassIndex == uiPass))
      {
        if (!sBlock.IsEmpty())
        {
          ezLog::Error("{}", sBlock);
          sBlock.Clear();
        }

        ezStringBuilder sText;
        const bool bIsCurrent = (uiGraph == s_uiCurrentGraphIndex && uiPass == s_uiCurrentPassIndex);
        sText.SetFormat("  {} Pass[{}] '{}' ", bIsCurrent ? ">" : " ", uiPass, szPassName);
        for (const auto& info : overlapped)
        {
          sText.AppendFormat(", OP: state={}, stage={}",
            ezArgEnum(info.m_access), ezArgEnum(info.m_stage));
        }
        for (const auto& barrier : overlappedBarriers)
        {
          sText.AppendFormat(", BARRIER: {} -> {}, stages: {} -> {}",
            ezArgEnum(barrier.m_StateBefore), ezArgEnum(barrier.m_StateAfter),
            ezArgEnum(barrier.m_StagesBefore), ezArgEnum(barrier.m_StagesAfter));
        }

        ezLog::Error("{}", sText.GetView());
      }
    }

    if (!sBlock.IsEmpty())
    {
      ezLog::Error("{} : Imported but no barrier found", sBlock);
    }
  }

  ezLog::Error("=== End Buffer Resource History ===");
}
