#include <RendererCorePCH.h>

#include <Core/World/World.h>
#include <Foundation/Time/Clock.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/FrameDataProvider.h>
#include <RendererCore/Pipeline/Passes/TargetPass.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Profiling/Profiling.h>

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
ezCVarBool ezRenderPipeline::s_DebugCulling("r_DebugCulling", false, ezCVarFlags::Default,
                                            "Enables debug visualization of visibility culling");

ezCVarBool CVarCullingStats("r_CullingStats", false, ezCVarFlags::Default, "Display some stats of the visibility culling");
#endif

ezRenderPipeline::ezRenderPipeline()
    : m_PipelineState(PipelineState::Uninitialized)
{
  m_CurrentExtractThread = (ezThreadID)0;
  m_CurrentRenderThread = (ezThreadID)0;
  m_uiLastExtractionFrame = -1;
  m_uiLastRenderFrame = -1;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  m_AverageCullingTime = ezTime::Seconds(0.1f);
#endif
}

ezRenderPipeline::~ezRenderPipeline()
{
  m_Data[0].Clear();
  m_Data[1].Clear();

  ClearRenderPassGraphTextures();
  while (!m_Passes.IsEmpty())
  {
    RemovePass(m_Passes.PeekBack().Borrow());
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

void ezRenderPipeline::GetPasses(ezHybridArray<const ezRenderPipelinePass*, 16>& passes) const
{
  passes.Reserve(m_Passes.GetCount());

  for (auto& pPass : m_Passes)
  {
    passes.PushBack(pPass.Borrow());
  }
}

void ezRenderPipeline::GetPasses(ezHybridArray<ezRenderPipelinePass*, 16>& passes)
{
  passes.Reserve(m_Passes.GetCount());

  for (auto& pPass : m_Passes)
  {
    passes.PushBack(pPass.Borrow());
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


bool ezRenderPipeline::Connect(ezRenderPipelinePass* pOutputNode, const char* szOutputPinName, ezRenderPipelinePass* pInputNode,
                               const char* szInputPinName)
{
  ezHashedString sOutputPinName;
  sOutputPinName.Assign(szOutputPinName);
  ezHashedString sInputPinName;
  sInputPinName.Assign(szInputPinName);
  return Connect(pOutputNode, sOutputPinName, pInputNode, sInputPinName);
}

bool ezRenderPipeline::Connect(ezRenderPipelinePass* pOutputNode, ezHashedString sOutputPinName, ezRenderPipelinePass* pInputNode,
                               ezHashedString sInputPinName)
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
  const ezNodePin* pPinSource = pOutputNode->GetPinByName(sOutputPinName);
  if (!pPinSource)
  {
    ezLog::Error("Source pin '{0}::{1}' does not exist!", pOutputNode->GetName(), sOutputPinName);
    return false;
  }
  const ezNodePin* pPinTarget = pInputNode->GetPinByName(sInputPinName);
  if (!pPinTarget)
  {
    ezLog::Error("Target pin '{0}::{1}' does not exist!", pInputNode->GetName(), sInputPinName);
    return false;
  }
  if (itIn.Value().m_Inputs[pPinTarget->m_uiInputIndex] != nullptr)
  {
    ezLog::Error("Pins already connected: '{0}::{1}' -> '{2}::{3}'!", pOutputNode->GetName(), sOutputPinName, pInputNode->GetName(),
                 sInputPinName);
    return false;
  }

  // Add at output
  ezRenderPipelinePassConnection* pConnection = itOut.Value().m_Outputs[pPinSource->m_uiOutputIndex];
  if (pConnection == nullptr)
  {
    pConnection = EZ_DEFAULT_NEW(ezRenderPipelinePassConnection);
    pConnection->m_pOutput = pPinSource;
    itOut.Value().m_Outputs[pPinSource->m_uiOutputIndex] = pConnection;
  }
  else
  {
    // Check that only one passthrough is connected
    if (pPinTarget->m_Type == ezNodePin::Type::PassThrough)
    {
      for (const ezNodePin* pPin : pConnection->m_Inputs)
      {
        if (pPin->m_Type == ezNodePin::Type::PassThrough)
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
  return true;
}

bool ezRenderPipeline::Disconnect(ezRenderPipelinePass* pOutputNode, ezHashedString sOutputPinName, ezRenderPipelinePass* pInputNode,
                                  ezHashedString sInputPinName)
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
  const ezNodePin* pPinSource = pOutputNode->GetPinByName(sOutputPinName);
  if (!pPinSource)
  {
    ezLog::Error("Source pin '{0}::{1}' does not exist!", pOutputNode->GetName(), sOutputPinName);
    return false;
  }
  const ezNodePin* pPinTarget = pInputNode->GetPinByName(sInputPinName);
  if (!pPinTarget)
  {
    ezLog::Error("Target pin '{0}::{1}' does not exist!", pInputNode->GetName(), sInputPinName);
    return false;
  }
  if (itIn.Value().m_Inputs[pPinTarget->m_uiInputIndex] == nullptr ||
      itIn.Value().m_Inputs[pPinTarget->m_uiInputIndex] != itOut.Value().m_Outputs[pPinSource->m_uiOutputIndex])
  {
    ezLog::Error("Pins not connected: '{0}::{1}' -> '{2}::{3}'!", pOutputNode->GetName(), sOutputPinName, pInputNode->GetName(),
                 sInputPinName);
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

  m_PipelineState = PipelineState::Uninitialized;
  return true;
}

const ezRenderPipelinePassConnection* ezRenderPipeline::GetInputConnection(ezRenderPipelinePass* pPass, ezHashedString sInputPinName) const
{
  auto it = m_Connections.Find(pPass);
  if (!it.IsValid())
    return nullptr;

  auto& data = it.Value();
  const ezNodePin* pPin = pPass->GetPinByName(sInputPinName);
  if (!pPin || pPin->m_uiInputIndex == 0xFF)
    return nullptr;

  return data.m_Inputs[pPin->m_uiInputIndex];
}

const ezRenderPipelinePassConnection* ezRenderPipeline::GetOutputConnection(ezRenderPipelinePass* pPass,
                                                                            ezHashedString sOutputPinName) const
{
  auto it = m_Connections.Find(pPass);
  if (!it.IsValid())
    return nullptr;

  auto& data = it.Value();
  const ezNodePin* pPin = pPass->GetPinByName(sOutputPinName);
  if (!pPin)
    return nullptr;

  return data.m_Outputs[pPin->m_uiOutputIndex];
}

ezRenderPipeline::PipelineState ezRenderPipeline::Rebuild(const ezView& view)
{
  ezLogBlock b("ezRenderPipeline::Rebuild");

  ClearRenderPassGraphTextures();

  bool bRes = RebuildInternal(view);
  if (!bRes)
  {
    ClearRenderPassGraphTextures();
  }
  else
  {
    // make sure the renderdata stores the updated view data
    UpdateViewData(view, ezRenderWorld::GetDataIndexForRendering());
  }

  m_PipelineState = bRes ? PipelineState::Initialized : PipelineState::RebuildError;
  return m_PipelineState;
}

bool ezRenderPipeline::RebuildInternal(const ezView& view)
{
  if (!SortPasses())
    return false;
  if (!InitRenderTargetDescriptions(view))
    return false;
  if (!CreateRenderTargetUsage(view))
    return false;
  if (!InitRenderPipelinePasses())
    return false;

  SortExtractors();

  return true;
}

bool ezRenderPipeline::SortPasses()
{
  ezLogBlock b("Sort Passes");
  ezHybridArray<ezRenderPipelinePass*, 32> done;
  done.Reserve(m_Passes.GetCount());

  ezHybridArray<ezRenderPipelinePass*, 8> usable;     // Stack of passes with all connections setup, they can be asked for descriptions.
  ezHybridArray<ezRenderPipelinePass*, 8> candidates; // Not usable yet, but all input connections are available

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
    ezLogBlock b("Traverse", pPass->GetName());

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
        for (const ezNodePin* pPin : data.m_Outputs[i]->m_Inputs)
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
    return false;
  }

  struct ezPipelineSorter
  {
    /// \brief Returns true if a is less than b
    EZ_FORCE_INLINE bool Less(const ezUniquePtr<ezRenderPipelinePass>& a, const ezUniquePtr<ezRenderPipelinePass>& b) const
    {
      return m_pDone->IndexOf(a.Borrow()) < m_pDone->IndexOf(b.Borrow());
    }

    /// \brief Returns true if a is equal to b
    EZ_ALWAYS_INLINE bool Equal(const ezUniquePtr<ezRenderPipelinePass>& a, const ezUniquePtr<ezRenderPipelinePass>& b) const
    {
      return a.Borrow() == b.Borrow();
    }

    ezHybridArray<ezRenderPipelinePass*, 32>* m_pDone;
  };

  ezPipelineSorter sorter;
  sorter.m_pDone = &done;
  m_Passes.Sort(sorter);
  return true;
}

bool ezRenderPipeline::InitRenderTargetDescriptions(const ezView& view)
{
  ezLogBlock b("Init Render Target Descriptions");
  ezHybridArray<ezGALTextureCreationDescription*, 10> inputs;
  ezHybridArray<ezGALTextureCreationDescription, 10> outputs;

  for (auto& pPass : m_Passes)
  {
    ezLogBlock b("InitPass", pPass->GetName());

    if (view.GetCamera()->IsStereoscopic() && !pPass->IsStereoAware())
    {
      ezLog::Error("View '{0}' uses a stereoscopic camera, but the render pass '{1}' does not support stereo rendering!", view.GetName(),
                   pPass->GetName());
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
        inputs[i] = &data.m_Inputs[i]->m_Desc;
      }
      else
      {
        inputs[i] = nullptr;
      }
    }

    bool bRes = pPass->GetRenderTargetDescriptions(view, inputs, outputs);
    if (!bRes)
    {
      ezLog::Error("The pass could not be successfully queried for render target descriptions.");
      return false;
    }

    // Copy queried outputs into the output connections.
    for (ezUInt32 i = 0; i < data.m_Outputs.GetCount(); i++)
    {
      if (data.m_Outputs[i] != nullptr)
      {
        data.m_Outputs[i]->m_Desc = outputs[i];
      }
    }

    // Check pass-through consistency of input / output target desc.
    auto inputPins = pPass->GetInputPins();
    for (const ezNodePin* pPin : inputPins)
    {
      if (pPin->m_Type == ezNodePin::Type::PassThrough)
      {
        if (data.m_Outputs[pPin->m_uiOutputIndex] != nullptr)
        {
          if (data.m_Inputs[pPin->m_uiInputIndex] == nullptr)
          {
            // ezLog::Error("The pass of type '{0}' has a pass through pin '{1}' that has an output but no input!",
            // pPass->GetDynamicRTTI()->GetTypeName(), pPass->GetPinName(pPin));  return false;
          }
          else if (data.m_Outputs[pPin->m_uiOutputIndex]->m_Desc.CalculateHash() !=
                   data.m_Inputs[pPin->m_uiInputIndex]->m_Desc.CalculateHash())
          {
            ezLog::Error("The pass has a pass through pin '{0}' that has different descriptors for input and output!",
                         pPass->GetPinName(pPin));
            return false;
          }
        }
      }
    }
  }
  return true;
}

bool ezRenderPipeline::CreateRenderTargetUsage(const ezView& view)
{
  ezLogBlock b("Create Render Target Usage Data");
  EZ_ASSERT_DEBUG(m_TextureUsage.IsEmpty(), "Need to call ClearRenderPassGraphTextures before re-creating the pipeline.");

  m_ConnectionToTextureIndex.Clear();

  // Gather all connections that share the same path-through texture and their first and last usage pass index.
  for (ezUInt32 i = 0; i < m_Passes.GetCount(); i++)
  {
    const auto& pPass = m_Passes[i].Borrow();
    ConnectionData& data = m_Connections[pPass];
    for (ezRenderPipelinePassConnection* pConn : data.m_Inputs)
    {
      if (pConn != nullptr)
      {
        ezUInt32 uiDataIdx = m_ConnectionToTextureIndex[pConn];
        m_TextureUsage[uiDataIdx].m_uiLastUsageIdx = i;
      }
    }

    for (ezRenderPipelinePassConnection* pConn : data.m_Outputs)
    {
      if (pConn != nullptr)
      {
        if (pConn->m_pOutput->m_Type == ezNodePin::Type::PassThrough && data.m_Inputs[pConn->m_pOutput->m_uiInputIndex] != nullptr)
        {
          ezRenderPipelinePassConnection* pCorrespondingInputConn = data.m_Inputs[pConn->m_pOutput->m_uiInputIndex];
          EZ_ASSERT_DEV(m_ConnectionToTextureIndex.Contains(pCorrespondingInputConn), "");
          ezUInt32 uiDataIdx = m_ConnectionToTextureIndex[pCorrespondingInputConn];
          m_TextureUsage[uiDataIdx].m_UsedBy.PushBack(pConn);
          m_TextureUsage[uiDataIdx].m_uiLastUsageIdx = i;

          EZ_ASSERT_DEV(!m_ConnectionToTextureIndex.Contains(pConn), "");
          m_ConnectionToTextureIndex[pConn] = uiDataIdx;
        }
        else
        {
          m_ConnectionToTextureIndex[pConn] = m_TextureUsage.GetCount();
          TextureUsageData& data = m_TextureUsage.ExpandAndGetRef();

          data.m_bTargetTexture = false;
          data.m_uiFirstUsageIdx = i;
          data.m_uiLastUsageIdx = i;
          data.m_UsedBy.PushBack(pConn);
        }
      }
    }
  }

  // Set view's render target textures to target pass connections.
  for (ezUInt32 i = 0; i < m_Passes.GetCount(); i++)
  {
    const auto& pPass = m_Passes[i].Borrow();
    if (pPass->IsInstanceOf<ezTargetPass>())
    {
      ezTargetPass* pTargetPass = static_cast<ezTargetPass*>(pPass);
      ConnectionData& data = m_Connections[pPass];
      for (ezUInt32 j = 0; j < data.m_Inputs.GetCount(); j++)
      {
        ezRenderPipelinePassConnection* pConn = data.m_Inputs[j];
        if (pConn != nullptr)
        {
          ezGALTextureHandle hTexture = pTargetPass->GetTextureHandle(view, pPass->GetInputPins()[j]);
          EZ_ASSERT_DEV(m_ConnectionToTextureIndex.Contains(pConn), "");
          ezUInt32 uiDataIdx = m_ConnectionToTextureIndex[pConn];
          m_TextureUsage[uiDataIdx].m_bTargetTexture = true;
          for (auto pConn : m_TextureUsage[uiDataIdx].m_UsedBy)
          {
            pConn->m_TextureHandle = hTexture;
          }
        }
      }
    }
  }

  // Stupid loop to gather all TextureUsageData indices that are not view render target textures.
  for (ezUInt32 i = 0; i < m_TextureUsage.GetCount(); i++)
  {
    TextureUsageData& data = m_TextureUsage[i];
    if (data.m_bTargetTexture)
      continue;

    m_TextureUsageIdxSortedByFirstUsage.PushBack((ezUInt16)i);
    m_TextureUsageIdxSortedByLastUsage.PushBack((ezUInt16)i);
  }

  // Sort first and last usage arrays, these will determine the lifetime of the pool textures.
  // TODO: Lambda sort function PLOX!
  struct FirstUsageComparer
  {
    FirstUsageComparer(ezDynamicArray<TextureUsageData>& textureUsage)
        : m_TextureUsage(textureUsage)
    {
    }

    EZ_ALWAYS_INLINE bool Less(ezUInt16 a, ezUInt16 b) const
    {
      return m_TextureUsage[a].m_uiFirstUsageIdx < m_TextureUsage[b].m_uiFirstUsageIdx;
    }

    ezDynamicArray<TextureUsageData>& m_TextureUsage;
  };

  struct LastUsageComparer
  {
    LastUsageComparer(ezDynamicArray<TextureUsageData>& textureUsage)
        : m_TextureUsage(textureUsage)
    {
    }

    EZ_ALWAYS_INLINE bool Less(ezUInt16 a, ezUInt16 b) const
    {
      return m_TextureUsage[a].m_uiLastUsageIdx < m_TextureUsage[b].m_uiLastUsageIdx;
    }

    ezDynamicArray<TextureUsageData>& m_TextureUsage;
  };

  m_TextureUsageIdxSortedByFirstUsage.Sort(FirstUsageComparer(m_TextureUsage));
  m_TextureUsageIdxSortedByLastUsage.Sort(LastUsageComparer(m_TextureUsage));

  return true;
}

bool ezRenderPipeline::InitRenderPipelinePasses()
{
  ezLogBlock b("Init Render Pipeline Passes");
  // Init every pass now.
  for (auto& pPass : m_Passes)
  {
    ConnectionData& data = m_Connections[pPass.Borrow()];
    pPass->InitRenderPipelinePass(data.m_Inputs, data.m_Outputs);
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
        if (sDependency == extractor->GetDynamicRTTI()->GetTypeNameHash())
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
    auto& extractor = m_Extractors[uiIndex];

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

void ezRenderPipeline::GetExtractors(ezHybridArray<const ezExtractor*, 16>& extractors) const
{
  extractors.Reserve(m_Extractors.GetCount());

  for (auto& pExtractor : m_Extractors)
  {
    extractors.PushBack(pExtractor.Borrow());
  }
}

void ezRenderPipeline::GetExtractors(ezHybridArray<ezExtractor*, 16>& extractors)
{
  extractors.Reserve(m_Extractors.GetCount());

  for (auto& pExtractor : m_Extractors)
  {
    extractors.PushBack(pExtractor.Borrow());
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

void ezRenderPipeline::ClearRenderPassGraphTextures()
{
  m_TextureUsage.Clear();
  m_TextureUsageIdxSortedByFirstUsage.Clear();
  m_TextureUsageIdxSortedByLastUsage.Clear();

  // ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  for (auto it = m_Connections.GetIterator(); it.IsValid(); ++it)
  {
    auto& conn = it.Value();
    for (auto pConn : conn.m_Outputs)
    {
      if (pConn)
      {
        pConn->m_Desc = ezGALTextureCreationDescription();
        if (!pConn->m_TextureHandle.IsInvalidated())
        {
          pConn->m_TextureHandle.Invalidate();
        }
      }
    }
  }
}

bool ezRenderPipeline::AreInputDescriptionsAvailable(const ezRenderPipelinePass* pPass,
                                                     const ezHybridArray<ezRenderPipelinePass*, 32>& done) const
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
    const ezNodePin* pPin = inputs[i];
    if (pPin->m_Type == ezNodePin::Type::PassThrough)
    {
      const ezRenderPipelinePassConnection* pConn = data.m_Inputs[pPin->m_uiInputIndex];
      if (pConn != nullptr)
      {
        for (const ezNodePin* pInputPin : pConn->m_Inputs)
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

      pExtractor->Extract(view, m_visibleObjects, data);
    }
  }

  data.SortAndBatch();

  for (auto& pExtractor : m_Extractors)
  {
    if (pExtractor->m_bActive)
    {
      EZ_PROFILE_SCOPE(pExtractor->m_sName.GetData());

      pExtractor->PostSortAndBatch(view, m_visibleObjects, data);
    }
  }

  m_CurrentExtractThread = (ezThreadID)0;
}

void ezRenderPipeline::FindVisibleObjects(const ezView& view)
{
  EZ_PROFILE_SCOPE("Visibility Culling");

  m_visibleObjects.Clear();

  ezFrustum frustum;
  view.ComputeCullingFrustum(frustum);

  EZ_LOCK(view.GetWorld()->GetReadMarker());

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  const bool bIsMainView =
      (view.GetCameraUsageHint() == ezCameraUsageHint::MainView || view.GetCameraUsageHint() == ezCameraUsageHint::EditorView);
  const bool bRecordStats = CVarCullingStats && bIsMainView;
  ezSpatialSystem::QueryStats stats;

  view.GetWorld()->GetSpatialSystem().FindVisibleObjects(frustum, m_visibleObjects, bRecordStats ? &stats : nullptr);

  ezViewHandle hView = view.GetHandle();

  if (s_DebugCulling && bIsMainView)
  {
    ezDebugRenderer::DrawLineFrustum(view.GetWorld(), frustum, ezColor::LimeGreen, false);
  }

  if (bRecordStats)
  {
    ezStringBuilder sb;

    ezDebugRenderer::Draw2DText(hView, "Visibility Culling Stats", ezVec2I32(10, 200), ezColor::LimeGreen);

    sb.Format("Total Num Objects: {0}", stats.m_uiTotalNumObjects);
    ezDebugRenderer::Draw2DText(hView, sb, ezVec2I32(10, 220), ezColor::LimeGreen);

    sb.Format("Num Objects Tested: {0}", stats.m_uiNumObjectsTested);
    ezDebugRenderer::Draw2DText(hView, sb, ezVec2I32(10, 240), ezColor::LimeGreen);

    sb.Format("Num Objects Passed: {0}", stats.m_uiNumObjectsPassed);
    ezDebugRenderer::Draw2DText(hView, sb, ezVec2I32(10, 260), ezColor::LimeGreen);

    // Exponential moving average for better readability.
    m_AverageCullingTime = ezMath::Lerp(m_AverageCullingTime, stats.m_TimeTaken, 0.05f);

    sb.Format("Time Taken: {0}ms", m_AverageCullingTime.GetMilliseconds());
    ezDebugRenderer::Draw2DText(hView, sb, ezVec2I32(10, 280), ezColor::LimeGreen);
  }
#else
  view.GetWorld()->GetSpatialSystem().FindVisibleObjects(frustum, m_visibleObjects, nullptr);
#endif
}

void ezRenderPipeline::Render(ezRenderContext* pRenderContext)
{
  EZ_PROFILE_AND_MARKER(pRenderContext->GetGALContext(), m_sName.GetData());

  EZ_ASSERT_DEV(m_PipelineState != PipelineState::Uninitialized, "Pipeline must be rebuild before rendering.");
  if (m_PipelineState == PipelineState::RebuildError)
  {
    return;
  }

  EZ_ASSERT_DEV(m_CurrentRenderThread == (ezThreadID)0, "Render must not be called from multiple threads.");
  m_CurrentRenderThread = ezThreadUtils::GetCurrentThreadID();

  EZ_ASSERT_DEV(m_uiLastRenderFrame != ezRenderWorld::GetFrameCounter(), "Render must not be called multiple times per frame.");
  m_uiLastRenderFrame = ezRenderWorld::GetFrameCounter();


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

  // Wrap around to prevent floating point issues. Wrap around is dividable by all whole numbers up to 11.
  gc.DeltaTime = (float)ezClock::GetGlobalClock()->GetTimeDiff().GetSeconds();
  gc.GlobalTime = (float)ezMath::Mod(ezClock::GetGlobalClock()->GetAccumulatedTime().GetSeconds(), 20790.0);
  gc.WorldTime = (float)ezMath::Mod(data.GetWorldTime().GetSeconds(), 20790.0);

  gc.Exposure = pCamera->GetExposure();
  gc.RenderPass = ezViewRenderMode::GetRenderPassForShader(pViewData->m_ViewRenderMode);

  ezRenderViewContext renderViewContext;
  renderViewContext.m_pCamera = pCamera;
  renderViewContext.m_pViewData = pViewData;
  renderViewContext.m_pRenderContext = pRenderContext;
  renderViewContext.m_pWorldDebugContext = &data.GetWorldDebugContext();
  renderViewContext.m_pViewDebugContext = &data.GetViewDebugContext();

  // Set camera mode permutation variable here since it doesn't change throughout the frame
  static ezHashedString sCameraMode = ezMakeHashedString("CAMERA_MODE");
  static ezHashedString sOrtho = ezMakeHashedString("CAMERA_MODE_ORTHO");
  static ezHashedString sPerspective = ezMakeHashedString("CAMERA_MODE_PERSPECTIVE");
  static ezHashedString sStereo = ezMakeHashedString("CAMERA_MODE_STEREO");

  if (pCamera->IsOrthographic())
    pRenderContext->SetShaderPermutationVariable(sCameraMode, sOrtho);
  else if (pCamera->IsStereoscopic())
    pRenderContext->SetShaderPermutationVariable(sCameraMode, sStereo);
  else
    pRenderContext->SetShaderPermutationVariable(sCameraMode, sPerspective);

  ezUInt32 uiCurrentFirstUsageIdx = 0;
  ezUInt32 uiCurrentLastUsageIdx = 0;
  for (ezUInt32 i = 0; i < m_Passes.GetCount(); ++i)
  {
    auto& pPass = m_Passes[i];

    // Create pool textures
    for (; uiCurrentFirstUsageIdx < m_TextureUsageIdxSortedByFirstUsage.GetCount();)
    {
      ezUInt16 uiCurrentUsageData = m_TextureUsageIdxSortedByFirstUsage[uiCurrentFirstUsageIdx];
      TextureUsageData& usageData = m_TextureUsage[uiCurrentUsageData];
      if (usageData.m_uiFirstUsageIdx == i)
      {
        ezGALTextureHandle hTexture = ezGPUResourcePool::GetDefaultInstance()->GetRenderTarget(usageData.m_UsedBy[0]->m_Desc);
        EZ_ASSERT_DEV(!hTexture.IsInvalidated(), "GPU pool returned an invalidated texture!");
        for (ezRenderPipelinePassConnection* pConn : usageData.m_UsedBy)
        {
          pConn->m_TextureHandle = hTexture;
        }
        ++uiCurrentFirstUsageIdx;
      }
      else
      {
        // The current usage data blocks m_uiFirstUsageIdx isn't reached yet so wait.
        break;
      }
    }

    // Execute pass block
    {
      EZ_PROFILE_AND_MARKER(pRenderContext->GetGALContext(), pPass->m_sName.GetData());

      ConnectionData& data = m_Connections[pPass.Borrow()];
      if (pPass->m_bActive)
      {
        pPass->Execute(renderViewContext, data.m_Inputs, data.m_Outputs);
      }
      else
      {
        pPass->ExecuteInactive(renderViewContext, data.m_Inputs, data.m_Outputs);
      }
    }

    // Release pool textures
    for (; uiCurrentLastUsageIdx < m_TextureUsageIdxSortedByLastUsage.GetCount();)
    {
      ezUInt16 uiCurrentUsageData = m_TextureUsageIdxSortedByLastUsage[uiCurrentLastUsageIdx];
      TextureUsageData& usageData = m_TextureUsage[uiCurrentUsageData];
      if (usageData.m_uiLastUsageIdx == i)
      {
        ezGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(usageData.m_UsedBy[0]->m_TextureHandle);
        for (ezRenderPipelinePassConnection* pConn : usageData.m_UsedBy)
        {
          pConn->m_TextureHandle.Invalidate();
        }
        ++uiCurrentLastUsageIdx;
      }
      else
      {
        // The current usage data blocks m_uiLastUsageIdx isn't reached yet so wait.
        break;
      }
    }
  }
  EZ_ASSERT_DEV(uiCurrentFirstUsageIdx == m_TextureUsageIdxSortedByFirstUsage.GetCount(),
                "Rendering all passes should have moved us through all texture usage blocks!");
  EZ_ASSERT_DEV(uiCurrentLastUsageIdx == m_TextureUsageIdxSortedByLastUsage.GetCount(),
                "Rendering all passes should have moved us through all texture usage blocks!");

  data.Clear();

  m_CurrentRenderThread = (ezThreadID)0;
}

const ezExtractedRenderData& ezRenderPipeline::GetRenderData() const
{
  return m_Data[ezRenderWorld::GetDataIndexForRendering()];
}

ezRenderDataBatchList ezRenderPipeline::GetRenderDataBatchesWithCategory(ezRenderData::Category category,
                                                                         ezRenderDataBatch::Filter filter) const
{
  auto& data = m_Data[ezRenderWorld::GetDataIndexForRendering()];
  return data.GetRenderDataBatchesWithCategory(category, filter);
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderPipeline);

