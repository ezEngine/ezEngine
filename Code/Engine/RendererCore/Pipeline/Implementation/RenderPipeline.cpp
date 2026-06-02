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

ezRenderPipeline::ezRenderPipeline()
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  m_AverageCullingTime = ezTime::MakeFromSeconds(0.1f);
#endif

  ezRenderGraphManager::s_RenderEvent.AddEventHandler(ezMakeDelegate(&ezRenderPipeline::OnRenderEvent, this));
}

ezRenderPipeline::~ezRenderPipeline()
{
  ezRenderGraphManager::s_RenderEvent.RemoveEventHandler(ezMakeDelegate(&ezRenderPipeline::OnRenderEvent, this));

  ezGALDevice::GetDefaultDevice()->DestroyTexture(m_hOcclusionDebugViewTexture);

  m_Data[0].Clear();
  m_Data[1].Clear();

  while (!m_Passes.IsEmpty())
  {
    RemovePass(m_Passes.PeekBack().Borrow());
  }
}

void ezRenderPipeline::OnRenderEvent(const ezRenderGraphRenderEvent& e)
{
  if (e.m_Type == ezRenderGraphRenderEvent::Type::BeforeGraphExecution && e.m_pGraph == m_pRenderGraph)
  {
    UpdateRenderContext(*e.m_pContext);
  }
  else if (e.m_Type == ezRenderGraphRenderEvent::Type::AfterGraphExecution && e.m_pGraph == m_pRenderGraph)
  {
    auto& data = m_Data[ezRenderWorld::GetDataIndexForRendering()];
    data.Clear();
    m_CurrentRenderThread = (ezThreadID)0;
  }
}

void ezRenderPipeline::AddPass(ezUniquePtr<ezRenderPipelinePass>&& pPass)
{
  m_PipelineState = PipelineState::Uninitialized;
  pPass->m_pPipeline = this;
  pPass->InitializePins();

  auto it = m_Connections.Insert(pPass.Borrow(), ConnectionData());
  it.Value().m_Inputs.SetCount(pPass->GetInputPins().GetCount());
  it.Value().m_Outputs.SetCount(pPass->GetOutputPins().GetCount());
  m_Passes.PushBack(std::move(pPass));
}

void ezRenderPipeline::RemovePass(ezRenderPipelinePass* pPass)
{
  for (ezUInt32 i = 0; i < m_Passes.GetCount(); ++i)
  {
    if (m_Passes[i].Borrow() == pPass)
    {
      m_PipelineState = PipelineState::Uninitialized;
      RemoveConnections(pPass);
      m_Connections.Remove(pPass);
      pPass->m_pPipeline = nullptr;
      m_Passes.RemoveAtAndCopy(i);
      break;
    }
  }
}

void ezRenderPipeline::GetPasses(ezDynamicArray<const ezRenderPipelinePass*>& ref_passes) const
{
  ref_passes.Reserve(m_Passes.GetCount());

  for (auto& pPass : m_Passes)
  {
    ref_passes.PushBack(pPass.Borrow());
  }
}

void ezRenderPipeline::GetPasses(ezDynamicArray<ezRenderPipelinePass*>& ref_passes)
{
  ref_passes.Reserve(m_Passes.GetCount());

  for (auto& pPass : m_Passes)
  {
    ref_passes.PushBack(pPass.Borrow());
  }
}

ezRenderPipelinePass* ezRenderPipeline::GetPassByName(const ezStringView& sPassName)
{
  for (auto& pPass : m_Passes)
  {
    if (sPassName.IsEqual(pPass->GetName()))
    {
      return pPass.Borrow();
    }
  }

  return nullptr;
}

ezHashedString ezRenderPipeline::GetViewName() const
{
  return m_sName;
}

bool ezRenderPipeline::Connect(ezRenderPipelinePass* pOutputNode, const char* szOutputPinName, ezRenderPipelinePass* pInputNode, const char* szInputPinName)
{
  ezHashedString sOutputPinName;
  sOutputPinName.Assign(szOutputPinName);
  ezHashedString sInputPinName;
  sInputPinName.Assign(szInputPinName);
  return Connect(pOutputNode, sOutputPinName, pInputNode, sInputPinName);
}

bool ezRenderPipeline::Connect(ezRenderPipelinePass* pOutputNode, ezHashedString sOutputPinName, ezRenderPipelinePass* pInputNode, ezHashedString sInputPinName)
{
  ezLogBlock b("ezRenderPipeline::Connect");

  auto itOut = m_Connections.Find(pOutputNode);
  if (!itOut.IsValid())
  {
    ezLog::Error("Output node '{0}' not added to pipeline!", pOutputNode->GetName());
    return false;
  }
  auto itIn = m_Connections.Find(pInputNode);
  if (!itIn.IsValid())
  {
    ezLog::Error("Input node '{0}' not added to pipeline!", pInputNode->GetName());
    return false;
  }
  const ezRenderPipelineNodePin* pPinSource = pOutputNode->GetPinByName(sOutputPinName);
  if (!pPinSource)
  {
    ezLog::Error("Source pin '{0}::{1}' does not exist!", pOutputNode->GetName(), sOutputPinName);
    return false;
  }
  const ezRenderPipelineNodePin* pPinTarget = pInputNode->GetPinByName(sInputPinName);
  if (!pPinTarget)
  {
    ezLog::Error("Target pin '{0}::{1}' does not exist!", pInputNode->GetName(), sInputPinName);
    return false;
  }
  if (itIn.Value().m_Inputs[pPinTarget->m_uiInputIndex] != nullptr)
  {
    ezLog::Error("Pins already connected: '{0}::{1}' -> '{2}::{3}'!", pOutputNode->GetName(), sOutputPinName, pInputNode->GetName(), sInputPinName);
    return false;
  }

  // Add at output
  ezRenderPipelinePassConnection* pConnection = itOut.Value().m_Outputs[pPinSource->m_uiOutputIndex];
  if (pConnection == nullptr)
  {
    pConnection = EZ_DEFAULT_NEW(ezRenderPipelinePassConnection);
    pConnection->m_pOutput = pPinSource;
    pConnection->m_Connection = ezRenderPipelinePinConnection(ezRenderPipelinePinConnection::Connectivity::Texture);
    itOut.Value().m_Outputs[pPinSource->m_uiOutputIndex] = pConnection;
  }
  else
  {
    // Check that only one passthrough is connected
    if (pPinTarget->m_Type.IsSet(ezRenderPipelineNodePin::Type::PassThrough))
    {
      for (const ezRenderPipelineNodePin* pPin : pConnection->m_Inputs)
      {
        if (pPin->m_Type.IsSet(ezRenderPipelineNodePin::Type::PassThrough))
        {
          ezLog::Error("A pass through pin is already connected to the '{0}' pin!", sOutputPinName);
          return false;
        }
      }
    }
  }

  // Add at input
  pConnection->m_Inputs.PushBack(pPinTarget);
  itIn.Value().m_Inputs[pPinTarget->m_uiInputIndex] = pConnection;
  m_PipelineState = PipelineState::Uninitialized;

  if (pPinSource->m_Type.IsSet(ezRenderPipelineNodePin::Type::TextureProvider))
  {
    m_TextureProviderPins.Insert(pPinSource);
  }
  if (pPinTarget->m_Type.IsSet(ezRenderPipelineNodePin::Type::TextureProvider))
  {
    m_TextureProviderPins.Insert(pPinTarget);
  }

  return true;
}

bool ezRenderPipeline::Disconnect(ezRenderPipelinePass* pOutputNode, ezHashedString sOutputPinName, ezRenderPipelinePass* pInputNode, ezHashedString sInputPinName)
{
  ezLogBlock b("ezRenderPipeline::Connect");

  auto itOut = m_Connections.Find(pOutputNode);
  if (!itOut.IsValid())
  {
    ezLog::Error("Output node '{0}' not added to pipeline!", pOutputNode->GetName());
    return false;
  }
  auto itIn = m_Connections.Find(pInputNode);
  if (!itIn.IsValid())
  {
    ezLog::Error("Input node '{0}' not added to pipeline!", pInputNode->GetName());
    return false;
  }
  const ezRenderPipelineNodePin* pPinSource = pOutputNode->GetPinByName(sOutputPinName);
  if (!pPinSource)
  {
    ezLog::Error("Source pin '{0}::{1}' does not exist!", pOutputNode->GetName(), sOutputPinName);
    return false;
  }
  const ezRenderPipelineNodePin* pPinTarget = pInputNode->GetPinByName(sInputPinName);
  if (!pPinTarget)
  {
    ezLog::Error("Target pin '{0}::{1}' does not exist!", pInputNode->GetName(), sInputPinName);
    return false;
  }
  if (itIn.Value().m_Inputs[pPinTarget->m_uiInputIndex] == nullptr || itIn.Value().m_Inputs[pPinTarget->m_uiInputIndex] != itOut.Value().m_Outputs[pPinSource->m_uiOutputIndex])
  {
    ezLog::Error("Pins not connected: '{0}::{1}' -> '{2}::{3}'!", pOutputNode->GetName(), sOutputPinName, pInputNode->GetName(), sInputPinName);
    return false;
  }

  // Remove at input
  ezRenderPipelinePassConnection* pConnection = itOut.Value().m_Outputs[pPinSource->m_uiOutputIndex];
  pConnection->m_Inputs.RemoveAndCopy(pPinTarget);
  itIn.Value().m_Inputs[pPinTarget->m_uiInputIndex] = nullptr;

  if (pConnection->m_Inputs.IsEmpty())
  {
    // Remove at output
    itOut.Value().m_Outputs[pPinSource->m_uiOutputIndex] = nullptr;
    EZ_DEFAULT_DELETE(pConnection);
  }

  if (pPinSource->m_Type.IsSet(ezRenderPipelineNodePin::Type::TextureProvider))
  {
    m_TextureProviderPins.Remove(pPinSource);
  }
  if (pPinTarget->m_Type.IsSet(ezRenderPipelineNodePin::Type::TextureProvider))
  {
    m_TextureProviderPins.Remove(pPinTarget);
  }

  m_PipelineState = PipelineState::Uninitialized;
  return true;
}

const ezRenderPipelinePassConnection* ezRenderPipeline::GetInputConnection(const ezRenderPipelinePass* pPass, ezHashedString sInputPinName) const
{
  auto it = m_Connections.Find(pPass);
  if (!it.IsValid())
    return nullptr;

  auto& data = it.Value();
  const ezRenderPipelineNodePin* pPin = pPass->GetPinByName(sInputPinName);
  if (!pPin || pPin->m_uiInputIndex == 0xFF)
    return nullptr;

  return data.m_Inputs[pPin->m_uiInputIndex];
}

const ezRenderPipelinePassConnection* ezRenderPipeline::GetOutputConnection(const ezRenderPipelinePass* pPass, ezHashedString sOutputPinName) const
{
  auto it = m_Connections.Find(pPass);
  if (!it.IsValid())
    return nullptr;

  auto& data = it.Value();
  const ezRenderPipelineNodePin* pPin = pPass->GetPinByName(sOutputPinName);
  if (!pPin)
    return nullptr;

  return data.m_Outputs[pPin->m_uiOutputIndex];
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
  if (!SortPasses())
    return false;
  SortExtractors();
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

bool ezRenderPipeline::SortPasses()
{
  ezLogBlock b("Sort Passes");
  ezTempHybridArray<ezRenderPipelinePass*, 32> done;
  done.Reserve(m_Passes.GetCount());

  ezTempHybridArray<ezRenderPipelinePass*, 8> usable;     // Stack of passes with all connections setup, they can be asked for descriptions.
  ezTempHybridArray<ezRenderPipelinePass*, 8> candidates; // Not usable yet, but all input connections are available

  // Find all source passes from which we can start the output description propagation.
  for (auto& pPass : m_Passes)
  {
    // if (std::all_of(cbegin(it.Value().m_Inputs), cend(it.Value().m_Inputs), [](ezRenderPipelinePassConnection* pConn){return pConn ==
    // nullptr; }))
    if (AreInputDescriptionsAvailable(pPass.Borrow(), done))
    {
      usable.PushBack(pPass.Borrow());
    }
  }

  // Via a depth first traversal, order the passes
  while (!usable.IsEmpty())
  {
    ezRenderPipelinePass* pPass = usable.PeekBack();
    ezLogBlock b2("Traverse", pPass->GetName());

    usable.PopBack();
    ConnectionData& data = m_Connections[pPass];

    EZ_ASSERT_DEBUG(data.m_Inputs.GetCount() == pPass->GetInputPins().GetCount(), "Input pin count missmatch!");
    EZ_ASSERT_DEBUG(data.m_Outputs.GetCount() == pPass->GetOutputPins().GetCount(), "Output pin count missmatch!");

    // Check for new candidate passes. Can't be done in the previous loop as multiple connections may be required by a node.
    for (ezUInt32 i = 0; i < data.m_Outputs.GetCount(); i++)
    {
      if (data.m_Outputs[i] != nullptr)
      {
        // Go through all inputs this connection is connected to and test the corresponding node for availability
        for (const ezRenderPipelineNodePin* pPin : data.m_Outputs[i]->m_Inputs)
        {
          EZ_ASSERT_DEBUG(pPin->m_pParent != nullptr, "Pass was not initialized!");
          ezRenderPipelinePass* pTargetPass = static_cast<ezRenderPipelinePass*>(pPin->m_pParent);
          if (done.Contains(pTargetPass))
          {
            ezLog::Error("Loop detected, graph not supported!");
            return false;
          }

          if (!usable.Contains(pTargetPass) && !candidates.Contains(pTargetPass))
          {
            candidates.PushBack(pTargetPass);
          }
        }
      }
    }

    done.PushBack(pPass);

    // Check for usable candidates. Reverse order for depth first traversal.
    for (ezInt32 i = (ezInt32)candidates.GetCount() - 1; i >= 0; i--)
    {
      ezRenderPipelinePass* pCandidatePass = candidates[i];
      if (AreInputDescriptionsAvailable(pCandidatePass, done) && ArePassThroughInputsDone(pCandidatePass, done))
      {
        usable.PushBack(pCandidatePass);
        candidates.RemoveAtAndCopy(i);
      }
    }
  }

  if (done.GetCount() < m_Passes.GetCount())
  {
    ezLog::Error("Pipeline: Not all nodes could be initialized");
    for (auto& pass : m_Passes)
    {
      if (!done.Contains(pass.Borrow()))
      {
        ezLog::Error("Failed to initialize node: {} - {}", pass->GetName(), pass->GetDynamicRTTI()->GetTypeName());
      }
    }
    return false;
  }

  struct ezPipelineSorter
  {
    /// \brief Returns true if a is less than b
    EZ_FORCE_INLINE bool Less(const ezUniquePtr<ezRenderPipelinePass>& a, const ezUniquePtr<ezRenderPipelinePass>& b) const { return m_pDone->IndexOf(a.Borrow()) < m_pDone->IndexOf(b.Borrow()); }

    /// \brief Returns true if a is equal to b
    EZ_ALWAYS_INLINE bool Equal(const ezUniquePtr<ezRenderPipelinePass>& a, const ezUniquePtr<ezRenderPipelinePass>& b) const { return a.Borrow() == b.Borrow(); }

    ezTempHybridArray<ezRenderPipelinePass*, 32>* m_pDone;
  };

  ezPipelineSorter sorter;
  sorter.m_pDone = &done;
  m_Passes.Sort(sorter);
  return true;
}

bool ezRenderPipeline::AddRenderPasses(const ezViewData& viewData, const ezCamera& camera)
{
  ezLogBlock b("RebuildRenderGraph");
  ezTempHybridArray<ezRenderPipelinePinConnection, 10> inputs;
  ezTempHybridArray<ezRenderPipelinePinConnection, 10> outputs;

  for (auto& pPass : m_Passes)
  {
    ezLogBlock b2("AddRenderPasses", pPass->GetName());

    if (camera.IsStereoscopic() && !pPass->IsStereoAware())
    {
      ezLog::Error("View '{0}' uses a stereoscopic camera, but the render pass '{1}' does not support stereo rendering!", viewData.m_sName, pPass->GetName());
    }

    ConnectionData& data = m_Connections[pPass.Borrow()];

    EZ_ASSERT_DEBUG(data.m_Inputs.GetCount() == pPass->GetInputPins().GetCount(), "Input pin count missmatch!");
    EZ_ASSERT_DEBUG(data.m_Outputs.GetCount() == pPass->GetOutputPins().GetCount(), "Output pin count missmatch!");

    inputs.SetCount(data.m_Inputs.GetCount());
    outputs.Clear();
    outputs.SetCount(data.m_Outputs.GetCount());
    // Fill inputs array
    for (ezUInt32 i = 0; i < data.m_Inputs.GetCount(); i++)
    {
      if (data.m_Inputs[i] != nullptr)
      {
        inputs[i] = data.m_Inputs[i]->m_Connection;
      }
      else
      {
        inputs[i] = ezRenderPipelinePinConnection(ezRenderPipelinePinConnection::Connectivity::None);
      }
    }
    // Fill the outputs array
    for (ezUInt32 i = 0; i < data.m_Outputs.GetCount(); i++)
    {
      ezRenderPipelinePinConnection& out = outputs[i];
      const ezRenderPipelineNodePin* pPin = pPass->GetOutputPins()[i];
      if (data.m_Outputs[i] != nullptr)
      {
        out = data.m_Outputs[i]->m_Connection;
        if (pPin->m_Type.IsSet(ezRenderPipelineNodePin::Type::PassThrough))
        {
          out = inputs[pPin->m_uiInputIndex];
        }
      }
      else
      {
        out = ezRenderPipelinePinConnection(ezRenderPipelinePinConnection::Connectivity::None);
      }
    }
    m_pRenderGraph->PushMarker(pPass->GetName());
    ezStatus result = EZ_SUCCESS;
    if (pPass->m_bActive)
      result = pPass->AddRenderPasses(viewData, camera, *m_pRenderGraph, inputs, outputs);
    else
      result = pPass->AddRenderPassesInactive(viewData, camera, *m_pRenderGraph, inputs, outputs);

    m_pRenderGraph->PopMarker();
    if (result.Failed())
    {
      ezLog::Error("Pass '{}' pf type '{}' failed: {}", pPass->GetName(), pPass->GetDynamicRTTI()->GetTypeName(), result.GetMessageString());
      return false;
    }

    // Check that all connected output pins have been written to.
    for (ezUInt32 i = 0; i < outputs.GetCount(); i++)
    {
      ezRenderPipelinePinConnection& out = outputs[i];
      const ezRenderPipelineNodePin* pPin = pPass->GetOutputPins()[i];
      if (out.m_Connectivity == ezRenderPipelinePinConnection::Connectivity::Texture && out.m_TextureHandle.IsInvalidated())
      {
        // ezLog::Error("Pass '{}' of type {}: connected output pin '{}' was not written to by {}", pPass->GetName(), pPass->GetDynamicRTTI()->GetTypeName(), pPass->GetPinName(pPin), pPass->IsActive() ? "AddRenderPasses" : "AddRenderPassesInactive");
        // return false;
      }
      if (pPin->m_Type.IsSet(ezRenderPipelineNodePin::Type::PassThrough))
      {
        ezRenderPipelinePinConnection& in = inputs[pPin->m_uiInputIndex];
        if (out.m_TextureHandle != in.m_TextureHandle)
        {
          ezLog::Error("Pass '{}' of type {}: connected output pin '{}' has passthrough set but the output does not match the input", pPass->GetName(), pPass->GetDynamicRTTI()->GetTypeName(), pPass->GetPinName(pPin));
          return false;
        }
      }
    }

    // Copy queried outputs into the output connections.
    for (ezUInt32 i = 0; i < data.m_Outputs.GetCount(); i++)
    {
      if (data.m_Outputs[i] != nullptr)
      {
        data.m_Outputs[i]->m_Connection = outputs[i];
      }
    }
  }
  return true;
}

bool ezRenderPipeline::UpdateTextureProviders()
{
  ezMap<ezRenderGraphTextureHandle, const ezRenderPipelineNodePin*> updates;
  for (const ezRenderPipelineNodePin* pPin : m_TextureProviderPins)
  {
    const ezRenderPipelinePass* pPass = static_cast<const ezRenderPipelinePass*>(pPin->m_pParent);
    ConnectionData& data = m_Connections[pPass];

    ezRenderGraphTextureHandle hTexture = {};
    if (pPin->m_uiInputIndex != 0xFF)
    {
      ezRenderPipelinePassConnection* pConnection = data.m_Inputs[pPin->m_uiInputIndex];
      EZ_ASSERT_DEBUG(pConnection->m_Connection.m_Connectivity == ezRenderPipelinePinConnection::Connectivity::Texture, "Wrong connectivity for a texture provider pin!");
      hTexture = pConnection->m_Connection.m_TextureHandle;
    }
    else if (pPin->m_uiOutputIndex != 0xFF)
    {
      ezRenderPipelinePassConnection* pConnection = data.m_Outputs[pPin->m_uiOutputIndex];
      EZ_ASSERT_DEBUG(pConnection->m_Connection.m_Connectivity == ezRenderPipelinePinConnection::Connectivity::Texture, "Wrong connectivity for a texture provider pin!");
      hTexture = pConnection->m_Connection.m_TextureHandle;
    }
    else
    {
      EZ_REPORT_FAILURE("Texture provider pin is invalid!");
    }

    if (updates.Contains(hTexture))
    {
      ezLog::Error("Two texture provider pins found for the same texture handle - render pipeline is invalid!");
      return false;
    }
    updates.Insert(hTexture, pPin);
  }

  for (auto it : updates)
  {
    ezRenderGraphTextureHandle hTexture = it.Key();
    const ezRenderPipelineNodePin* pPin = it.Value();
    ezRenderPipelinePass* pPass = static_cast<ezRenderPipelinePass*>(pPin->m_pParent);
    const ezGALTextureCreationDescription& desc = m_pRenderGraph->GetTextureDesc(hTexture);
    ezGALTextureHandle hTextureHandle = pPass->QueryTextureProvider(pPin, desc);
    if (!hTextureHandle.IsInvalidated())
    {
      ezStatus result = m_pRenderGraph->ReplaceImportedTexture(hTexture, hTextureHandle);
      if (result.Failed() && !result.GetMessageString().IsEmpty())
      {
        ezLog::Error("Failed to replace texture from node '{}' of type '{}' at pin '{}': {}", pPass->GetName(), pPass->GetDynamicRTTI()->GetTypeName(), pPass->GetPinName(pPin), result.GetMessageString());
        return false;
      }
    }
  }

  return true;
}


void ezRenderPipeline::SortExtractors()
{
  struct Helper
  {
    static bool FindDependency(const ezHashedString& sDependency, ezArrayPtr<ezUniquePtr<ezExtractor>> container)
    {
      for (auto& extractor : container)
      {
        if (sDependency == ezTempHashedString(extractor->GetDynamicRTTI()->GetTypeNameHash()))
        {
          return true;
        }
      }

      return false;
    }
  };

  m_SortedExtractors.Clear();
  m_SortedExtractors.Reserve(m_Extractors.GetCount());

  ezUInt32 uiIndex = 0;
  while (!m_Extractors.IsEmpty())
  {
    ezUniquePtr<ezExtractor>& extractor = m_Extractors[uiIndex];

    bool allDependenciesFound = true;
    for (auto& sDependency : extractor->m_DependsOn)
    {
      if (!Helper::FindDependency(sDependency, m_SortedExtractors))
      {
        allDependenciesFound = false;
        break;
      }
    }

    if (allDependenciesFound)
    {
      m_SortedExtractors.PushBack(std::move(extractor));
      m_Extractors.RemoveAtAndCopy(uiIndex);
    }
    else
    {
      ++uiIndex;
    }

    if (uiIndex >= m_Extractors.GetCount())
    {
      uiIndex = 0;
    }
  }

  m_Extractors.Swap(m_SortedExtractors);
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

void ezRenderPipeline::AddExtractor(ezUniquePtr<ezExtractor>&& pExtractor)
{
  m_Extractors.PushBack(std::move(pExtractor));
}

void ezRenderPipeline::RemoveExtractor(ezExtractor* pExtractor)
{
  for (ezUInt32 i = 0; i < m_Extractors.GetCount(); ++i)
  {
    if (m_Extractors[i].Borrow() == pExtractor)
    {
      m_Extractors.RemoveAtAndCopy(i);
      break;
    }
  }
}

void ezRenderPipeline::GetExtractors(ezDynamicArray<const ezExtractor*>& ref_extractors) const
{
  ref_extractors.Reserve(m_Extractors.GetCount());

  for (auto& pExtractor : m_Extractors)
  {
    ref_extractors.PushBack(pExtractor.Borrow());
  }
}

void ezRenderPipeline::GetExtractors(ezDynamicArray<ezExtractor*>& ref_extractors)
{
  ref_extractors.Reserve(m_Extractors.GetCount());

  for (auto& pExtractor : m_Extractors)
  {
    ref_extractors.PushBack(pExtractor.Borrow());
  }
}


ezExtractor* ezRenderPipeline::GetExtractorByName(const ezStringView& sExtractorName)
{
  for (auto& pExtractor : m_Extractors)
  {
    if (sExtractorName.IsEqual(pExtractor->GetName()))
    {
      return pExtractor.Borrow();
    }
  }

  return nullptr;
}

void ezRenderPipeline::RemoveConnections(ezRenderPipelinePass* pPass)
{
  auto it = m_Connections.Find(pPass);
  if (!it.IsValid())
    return;

  ConnectionData& data = it.Value();
  for (ezUInt32 i = 0; i < data.m_Inputs.GetCount(); i++)
  {
    ezRenderPipelinePassConnection* pConn = data.m_Inputs[i];
    if (pConn != nullptr)
    {
      ezRenderPipelinePass* pSource = static_cast<ezRenderPipelinePass*>(pConn->m_pOutput->m_pParent);
      bool bRes = Disconnect(pSource, pSource->GetPinName(pConn->m_pOutput), pPass, pPass->GetPinName(pPass->GetInputPins()[i]));
      EZ_IGNORE_UNUSED(bRes);
      EZ_ASSERT_DEBUG(bRes, "ezRenderPipeline::RemoveConnections should not fail to disconnect pins!");
    }
  }
  for (ezUInt32 i = 0; i < data.m_Outputs.GetCount(); i++)
  {
    ezRenderPipelinePassConnection* pConn = data.m_Outputs[i];
    while (pConn != nullptr)
    {
      ezRenderPipelinePass* pTarget = static_cast<ezRenderPipelinePass*>(pConn->m_Inputs[0]->m_pParent);
      bool bRes = Disconnect(pPass, pPass->GetPinName(pConn->m_pOutput), pTarget, pTarget->GetPinName(pConn->m_Inputs[0]));
      EZ_IGNORE_UNUSED(bRes);
      EZ_ASSERT_DEBUG(bRes, "ezRenderPipeline::RemoveConnections should not fail to disconnect pins!");

      pConn = data.m_Outputs[i];
    }
  }
}

bool ezRenderPipeline::AreInputDescriptionsAvailable(const ezRenderPipelinePass* pPass, const ezHybridArray<ezRenderPipelinePass*, 32>& done) const
{
  auto it = m_Connections.Find(pPass);
  const ConnectionData& data = it.Value();
  for (ezUInt32 i = 0; i < data.m_Inputs.GetCount(); i++)
  {
    const ezRenderPipelinePassConnection* pConn = data.m_Inputs[i];
    if (pConn != nullptr)
    {
      // If the connections source is not done yet, the connections output is undefined yet and the inputs can't be processed yet.
      if (!done.Contains(static_cast<ezRenderPipelinePass*>(pConn->m_pOutput->m_pParent)))
      {
        return false;
      }
    }
  }

  return true;
}

bool ezRenderPipeline::ArePassThroughInputsDone(const ezRenderPipelinePass* pPass, const ezHybridArray<ezRenderPipelinePass*, 32>& done) const
{
  auto it = m_Connections.Find(pPass);
  const ConnectionData& data = it.Value();
  auto inputs = pPass->GetInputPins();
  for (ezUInt32 i = 0; i < inputs.GetCount(); i++)
  {
    const ezRenderPipelineNodePin* pPin = inputs[i];
    if (pPin->m_Type.IsSet(ezRenderPipelineNodePin::Type::PassThrough))
    {
      const ezRenderPipelinePassConnection* pConn = data.m_Inputs[pPin->m_uiInputIndex];
      if (pConn != nullptr)
      {
        for (const ezRenderPipelineNodePin* pInputPin : pConn->m_Inputs)
        {
          // Any input that is also connected to the source of pPin must be done before we can use the pass through input
          if (pInputPin != pPin && !done.Contains(static_cast<ezRenderPipelinePass*>(pInputPin->m_pParent)))
          {
            return false;
          }
        }
      }
    }
  }
  return true;
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

  // Store camera and viewdata
  data.SetCamera(*view.GetCamera());
  data.SetViewData(view.GetData());
  data.SetWorldTime(view.GetWorld()->GetClock().GetAccumulatedTime());
  data.SetWorldDebugContext(view.GetWorld());
  data.SetViewDebugContext(view.GetHandle());

  // Extract object render data
  for (auto& pExtractor : m_Extractors)
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

  for (auto& pExtractor : m_Extractors)
  {
    if (pExtractor->m_bActive)
    {
      EZ_PROFILE_SCOPE(pExtractor->m_sName.GetData());

      pExtractor->PostSortAndBatch(view, m_VisibleObjects, data);
    }
  }

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
  for (const ezViewDependency& dep : data.GetViewDependencies())
  {
    m_pRenderGraph->ImportTexture(dep.m_hTexture, dep.m_RequiredState, dep.m_Stage);
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
}

const ezExtractedRenderData& ezRenderPipeline::GetRenderData() const
{
  return m_Data[ezRenderWorld::GetDataIndexForRendering()];
}

void ezRenderPipeline::AddViewDependency(ezGALTextureHandle hTexture, ezBitflags<ezGALResourceState> requiredState, ezBitflags<ezGALShaderStageFlags> stage)
{
  m_Data[ezRenderWorld::GetDataIndexForExtraction()].AddViewDependency(hTexture, requiredState, stage);
}

ezRenderDataBatchList ezRenderPipeline::GetRenderDataBatchesWithCategory(ezRenderData::Category category) const
{
  auto& data = m_Data[ezRenderWorld::GetDataIndexForRendering()];
  return data.GetRenderDataBatchesWithCategory(category);
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
