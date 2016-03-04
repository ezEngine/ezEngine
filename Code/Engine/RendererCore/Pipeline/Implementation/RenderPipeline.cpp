#include <RendererCore/PCH.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Pipeline/TargetPass.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderLoop/RenderLoop.h>

#include <Foundation/Configuration/CVar.h>

#include <Core/World/World.h>

#include <RendererFoundation/Context/Profiling.h>

extern ezCVarBool CVarMultithreadedRendering;

ezRenderPipeline::ezRenderPipeline() : m_PipelineState(PipelineState::Uninitialized)
{
  m_CurrentExtractThread = (ezThreadID)0;
  m_CurrentRenderThread = (ezThreadID)0;
  m_uiLastExtractionFrame = -1;
  m_uiLastRenderFrame = -1;
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

void ezRenderPipeline::ResetPipelineState()
{
  m_PipelineState = PipelineState::Uninitialized;
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
      m_Passes.RemoveAt(i);
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

ezRenderPipelinePass* ezRenderPipeline::GetPassByName(const char* szPassName) const
{
  for (auto& pPass : m_Passes)
  {
    if (ezStringUtils::IsEqual(pPass->GetName(), szPassName))
    {
      return pPass.Borrow();
    }
  }

  return nullptr;
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
    ezLog::Error("Output node '%s' not added to pipeline!", pOutputNode->GetName());
    return false;
  }
  auto itIn = m_Connections.Find(pInputNode);
  if (!itIn.IsValid())
  {
    ezLog::Error("Input node '%s' not added to pipeline!", pInputNode->GetName());
    return false;
  }
  const ezNodePin* pPinSource = pOutputNode->GetPinByName(sOutputPinName);
  if (!pPinSource)
  {
    ezLog::Error("Source pin '%s::%s' does not exist!", pOutputNode->GetName(), sOutputPinName.GetData());
    return false;
  }
  const ezNodePin* pPinTarget = pInputNode->GetPinByName(sInputPinName);
  if (!pPinTarget)
  {
    ezLog::Error("Target pin '%s::%s' does not exist!", pInputNode->GetName(), sInputPinName.GetData());
    return false;
  }
  if (itIn.Value().m_Inputs[pPinTarget->m_uiInputIndex] != nullptr)
  {
    ezLog::Error("Pins already connected: '%s::%s' -> '%s::%s'!", pOutputNode->GetName(), sOutputPinName.GetData(), pInputNode->GetName(), sInputPinName.GetData());
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
          ezLog::Error("A pass through pin is already connected to the '%s' pin!", sOutputPinName.GetData());
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

bool ezRenderPipeline::Disconnect(ezRenderPipelinePass* pOutputNode, ezHashedString sOutputPinName, ezRenderPipelinePass* pInputNode, ezHashedString sInputPinName)
{
  ezLogBlock b("ezRenderPipeline::Connect");

  auto itOut = m_Connections.Find(pOutputNode);
  if (!itOut.IsValid())
  {
    ezLog::Error("Output node '%s' not added to pipeline!", pOutputNode->GetName());
    return false;
  }
  auto itIn = m_Connections.Find(pInputNode);
  if (!itIn.IsValid())
  {
    ezLog::Error("Input node '%s' not added to pipeline!", pInputNode->GetName());
    return false;
  }
  const ezNodePin* pPinSource = pOutputNode->GetPinByName(sOutputPinName);
  if (!pPinSource)
  {
    ezLog::Error("Source pin '%s::%s' does not exist!", pOutputNode->GetName(), sOutputPinName.GetData());
    return false;
  }
  const ezNodePin* pPinTarget = pInputNode->GetPinByName(sInputPinName);
  if (!pPinTarget)
  {
    ezLog::Error("Target pin '%s::%s' does not exist!", pInputNode->GetName(), sInputPinName.GetData());
    return false;
  }
  if (itIn.Value().m_Inputs[pPinTarget->m_uiInputIndex] == nullptr || itIn.Value().m_Inputs[pPinTarget->m_uiInputIndex] != itOut.Value().m_Outputs[pPinSource->m_uiOutputIndex])
  {
    ezLog::Error("Pins not connected: '%s::%s' -> '%s::%s'!", pOutputNode->GetName(), sOutputPinName.GetData(), pInputNode->GetName(), sInputPinName.GetData());
    return false;
  }

  // Remove at input
  ezRenderPipelinePassConnection* pConnection = itOut.Value().m_Outputs[pPinSource->m_uiOutputIndex];
  pConnection->m_Inputs.Remove(pPinTarget);
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
  if (!pPin || pPin->m_uiInputIndex == -1)
    return nullptr;

  return data.m_Inputs[pPin->m_uiInputIndex];
}

const ezRenderPipelinePassConnection* ezRenderPipeline::GetOutputConnection(ezRenderPipelinePass* pPass, ezHashedString sOutputPinName) const
{
  auto it = m_Connections.Find(pPass);
  if (!it.IsValid())
    return nullptr;

  auto& data = it.Value();
  const ezNodePin* pPin = pPass->GetPinByName(sOutputPinName);
  if (!pPin || pPin->m_uiOutputIndex == -1)
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
  m_PipelineState = bRes ? PipelineState::Initialized : PipelineState::RebuildError;
  return m_PipelineState;
}

bool ezRenderPipeline::RebuildInternal(const ezView& view)
{
  if (!SortPasses())
    return false;
  if (!InitRenderTargetDescriptions(view))
    return false;
  if (!CreateRenderTargets(view))
    return false;
  if (!SetRenderTargets())
    return false;
  return true;
}

bool ezRenderPipeline::SortPasses()
{
  ezLogBlock b("Sort Passes");
  ezDynamicArray<ezRenderPipelinePass*> done;
  done.Reserve(m_Passes.GetCount());

  ezHybridArray<ezRenderPipelinePass*, 8> usable; // Stack of passes with all connections setup, they can be asked for descriptions.
  ezHybridArray<ezRenderPipelinePass*, 8> candidates; // Not usable yet, but all input connections are available

  // Find all source passes from which we can start the output description propagation.
  for (auto& pPass : m_Passes)
  {
    auto it = m_Connections.Find(pPass.Borrow());
    //if (std::all_of(cbegin(it.Value().m_Inputs), cend(it.Value().m_Inputs), [](ezRenderPipelinePassConnection* pConn){return pConn == nullptr; }))
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

          if (!usable.Contains(pTargetPass) && AreInputDescriptionsAvailable(pTargetPass, done))
          {
            usable.PushBack(pTargetPass);
          }
        }
      }
    }

    // Check for usable candidates. Reverse order for depth first traversal.
    for (ezInt32 i = (ezInt32)candidates.GetCount() - 1; i >= 0; i--)
    {
      ezRenderPipelinePass* pCandidatePass = candidates[i];
      if (ArePassThroughInputsDone(pCandidatePass, done))
      {
        usable.PushBack(pCandidatePass);
        candidates.RemoveAt(i);
      }
    }

    done.PushBack(pPass);
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
    EZ_FORCE_INLINE bool Equal(const ezUniquePtr<ezRenderPipelinePass>& a, const ezUniquePtr<ezRenderPipelinePass>& b) const
    {
      return a.Borrow() == b.Borrow();
    }

    ezDynamicArray<ezRenderPipelinePass*>* m_pDone;
  };

  ezPipelineSorter sorter;
  sorter.m_pDone = &done;
  m_Passes.Sort(sorter);
  return true;
}

bool ezRenderPipeline::InitRenderTargetDescriptions(const ezView& view)
{
  ezLogBlock b("Init Render Target Descriptions");
  ezHybridArray<ezGALTextureCreationDescription*, 8> inputs;
  ezHybridArray<ezGALTextureCreationDescription, 8> outputs;

  for (auto& pPass : m_Passes)
  {
    ezLogBlock b("InitPass", pPass->GetName());
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
      ezLog::Error("The pass could not be successfully queried for render target descs.");
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
    
    // Check passthrough consistency of input / output target desc.
    auto inputPins = pPass->GetInputPins();
    for (const ezNodePin* pPin : inputPins)
    {
      if (pPin->m_Type == ezNodePin::Type::PassThrough)
      {
        if (data.m_Outputs[pPin->m_uiOutputIndex] != nullptr)
        {
          if (data.m_Inputs[pPin->m_uiInputIndex] == nullptr)
          {
            //ezLog::Error("The pass of type '%s' has a pass through pin '%s' that has an output but no input!", pPass->GetDynamicRTTI()->GetTypeName(), pPass->GetPinName(pPin).GetData());
            //return false;
          }
          else if (data.m_Outputs[pPin->m_uiOutputIndex]->m_Desc.CalculateHash() != data.m_Inputs[pPin->m_uiInputIndex]->m_Desc.CalculateHash())
          {
            ezLog::Error("The pass has a pass through pin '%s' that has different descriptors for input and putput!", pPass->GetPinName(pPin).GetData());
            return false;
          }
        }
      }
    }
  }
  return true;
}

bool ezRenderPipeline::CreateRenderTargets(const ezView& view)
{
  ezLogBlock b("Create Render Targets");
  // The 'done' array has now the correct order, create textures.
  struct TextureUsageData
  {
    bool m_bTargetTexture;
    ezUInt32 m_uiHash;
    ezUInt32 m_uiFirstUsageIdx;
    ezUInt32 m_uiLastUsageIdx;
    ezHybridArray<ezRenderPipelinePassConnection*, 4> m_UsedBy;
  };
  ezDynamicArray<TextureUsageData> usedTextures;
  ezMap<ezRenderPipelinePassConnection*, ezUInt32> connectionToTextureIndex;
  for (ezUInt32 i = 0; i < m_Passes.GetCount(); i++)
  {
    const auto& pPass = m_Passes[i].Borrow();
    ConnectionData& data = m_Connections[pPass];
    for (ezRenderPipelinePassConnection* pConn : data.m_Inputs)
    {
      if (pConn != nullptr)
      {
        ezUInt32 uiDataIdx = connectionToTextureIndex[pConn];
        usedTextures[uiDataIdx].m_uiLastUsageIdx = i;
      }
    }

    for (ezRenderPipelinePassConnection* pConn : data.m_Outputs)
    {
      if (pConn != nullptr)
      {
        if (pConn->m_pOutput->m_Type == ezNodePin::Type::PassThrough && data.m_Inputs[pConn->m_pOutput->m_uiInputIndex] != nullptr)
        {
          ezRenderPipelinePassConnection* pCorrespondingInputConn = data.m_Inputs[pConn->m_pOutput->m_uiInputIndex];
          //EZ_ASSERT_DEBUG(pCorrespondingInputConn != nullptr, "Pass through input missing for output!");
          ezUInt32 uiDataIdx = connectionToTextureIndex[pCorrespondingInputConn];
          usedTextures[uiDataIdx].m_UsedBy.PushBack(pConn);
          usedTextures[uiDataIdx].m_uiLastUsageIdx = i;
        }
        else
        {
          connectionToTextureIndex[pConn] = usedTextures.GetCount();
          TextureUsageData& data = usedTextures.ExpandAndGetRef();

          data.m_bTargetTexture = false;
          data.m_uiHash = pConn->m_Desc.CalculateHash();
          data.m_uiFirstUsageIdx = i;
          data.m_uiLastUsageIdx = i;
          data.m_UsedBy.PushBack(pConn);
        }
      }
    }
  }

  // TODO: Merge ranges of same textures that do not have overlapping usage ranges.
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

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
          ezUInt32 uiDataIdx = connectionToTextureIndex[pConn];
          usedTextures[uiDataIdx].m_bTargetTexture = true;
          for (auto pConn : usedTextures[uiDataIdx].m_UsedBy)
          {
            pConn->m_TextureHandle = hTexture;
          }
        }
      }
    }
  }

  m_Textures.Reserve(usedTextures.GetCount());
  for (ezUInt32 i = 0; i < usedTextures.GetCount(); i++)
  {
    TextureUsageData& data = usedTextures[i];
    if (data.m_bTargetTexture)
      continue;

    // TODO: What if create texture fails?
    ezGALTextureHandle hTexture = pDevice->CreateTexture(data.m_UsedBy[0]->m_Desc);
    m_Textures.PushBack(hTexture);
    for (auto pConn : data.m_UsedBy)
    {
      pConn->m_TextureHandle = hTexture;
    }
  }

  return true;
}

bool ezRenderPipeline::SetRenderTargets()
{
  ezLogBlock b("Set Render Targets");
  // Init every pass now.
  for (const auto& pPass : m_Passes)
  {
    ConnectionData& data = m_Connections[pPass.Borrow()];
    pPass->SetRenderTargets(data.m_Inputs, data.m_Outputs);
  }

  return true;
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
      m_Extractors.RemoveAt(i);
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


ezExtractor* ezRenderPipeline::GetExtractorByName(const char* szPassName) const
{
  for (auto& pExtractor : m_Extractors)
  {
    if (ezStringUtils::IsEqual(pExtractor->GetName(), szPassName))
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
      EZ_ASSERT_DEBUG(bRes, "ezRenderPipeline::RemoveConnections should not fail to disconnect pins!");

      pConn = data.m_Outputs[i];
    }
  }
}

void ezRenderPipeline::ClearRenderPassGraphTextures()
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  
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

  for (ezUInt32 i = 0; i < m_Textures.GetCount(); i++)
  {
    pDevice->DestroyTexture(m_Textures[i]);
  }
  m_Textures.Clear();
}

bool ezRenderPipeline::AreInputDescriptionsAvailable(const ezRenderPipelinePass* pPass, const ezDynamicArray<ezRenderPipelinePass*>& done) const
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

bool ezRenderPipeline::ArePassThroughInputsDone(const ezRenderPipelinePass* pPass, const ezDynamicArray<ezRenderPipelinePass*>& done) const
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

void ezRenderPipeline::ExtractData(const ezView& view)
{
  EZ_ASSERT_DEV(m_CurrentExtractThread == (ezThreadID)0, "Extract must not be called from multiple threads.");
  m_CurrentExtractThread = ezThreadUtils::GetCurrentThreadID();

  // Is this view already extracted?
  if (m_uiLastExtractionFrame == ezRenderLoop::GetFrameCounter())
    return;

  m_uiLastExtractionFrame = ezRenderLoop::GetFrameCounter();
  
  auto& data = GetDataForExtraction();

  // Usually clear is not needed, only if the multithreading flag is switched during runtime.
  data.Clear();

  // Store camera and viewdata
  data.m_Camera = *view.GetRenderCamera();
  data.m_ViewData = view.GetData();

  // Extract object render data
  for (auto& pExtractor : m_Extractors)
  {
    pExtractor->Extract(view, &data);
  }

  data.SortAndBatch();

  m_CurrentExtractThread = (ezThreadID)0;
}

void ezRenderPipeline::Render(ezRenderContext* pRendererContext)
{
  EZ_PROFILE_AND_MARKER(pRendererContext->GetGALContext(), m_RenderProfilingID);

  EZ_ASSERT_DEV(m_PipelineState != PipelineState::Uninitialized, "Pipeline must be rebuild before rendering.");
  if (m_PipelineState == PipelineState::RebuildError)
  {
    return;
  }

  EZ_ASSERT_DEV(m_CurrentRenderThread == (ezThreadID)0, "Render must not be called from multiple threads.");
  m_CurrentRenderThread = ezThreadUtils::GetCurrentThreadID();

  EZ_ASSERT_DEV(m_uiLastRenderFrame != ezRenderLoop::GetFrameCounter(), "Render must not be called multiple times per frame.");
  m_uiLastRenderFrame = ezRenderLoop::GetFrameCounter();


  auto& data = GetDataForRendering();
  const ezCamera* pCamera = &data.m_Camera;
  const ezViewData* pViewData = &data.m_ViewData;

  auto& gc = pRendererContext->WriteGlobalConstants();
  gc.CameraPosition = pCamera->GetPosition();
  gc.CameraDirForwards = pCamera->GetDirForwards();
  gc.CameraDirRight = pCamera->GetDirRight();
  gc.CameraDirUp = pCamera->GetDirUp();
  gc.CameraToScreenMatrix = pViewData->m_ProjectionMatrix;
  gc.ScreenToCameraMatrix = pViewData->m_InverseProjectionMatrix;
  gc.WorldToCameraMatrix = pViewData->m_ViewMatrix;
  gc.CameraToWorldMatrix = pViewData->m_InverseViewMatrix;
  gc.WorldToScreenMatrix = pViewData->m_ViewProjectionMatrix;
  gc.ScreenToWorldMatrix = pViewData->m_InverseViewProjectionMatrix;
  gc.Viewport = ezVec4(pViewData->m_ViewPortRect.x, pViewData->m_ViewPortRect.y, pViewData->m_ViewPortRect.width, pViewData->m_ViewPortRect.height);

  ezRenderViewContext renderViewContext;
  renderViewContext.m_pCamera = pCamera;
  renderViewContext.m_pViewData = pViewData;
  renderViewContext.m_pRenderContext = pRendererContext;

  for (ezUInt32 i = 0; i < m_Passes.GetCount(); ++i)
  {
    {
      EZ_PROFILE_AND_MARKER(pRendererContext->GetGALContext(), m_Passes[i]->m_ProfilingID);

      m_Passes[i]->Execute(renderViewContext);
    }
  }

  data.Clear();

  m_CurrentRenderThread = (ezThreadID)0;
}

ezBatchedRenderData& ezRenderPipeline::GetDataForExtraction()
{
  return m_Data[ezRenderLoop::GetFrameCounter() & 1];
}

ezBatchedRenderData& ezRenderPipeline::GetDataForRendering()
{
  const ezUInt32 uiFrameCounter = ezRenderLoop::GetFrameCounter() + (CVarMultithreadedRendering ? 1 : 0);
  return m_Data[uiFrameCounter & 1];
}

const ezBatchedRenderData& ezRenderPipeline::GetDataForRendering() const
{
  const ezUInt32 uiFrameCounter = ezRenderLoop::GetFrameCounter() + (CVarMultithreadedRendering ? 1 : 0);
  return m_Data[uiFrameCounter & 1];
}

ezArrayPtr< const ezArrayPtr<const ezRenderData*> > ezRenderPipeline::GetRenderDataBatchesWithCategory(ezRenderData::Category category) const
{
  auto& data = GetDataForRendering();
  if (data.m_DataPerCategory.GetCount() > category)
  {
    return data.m_DataPerCategory[category].m_Batches;
  }

  return ezArrayPtr< const ezArrayPtr<const ezRenderData*> >();
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderPipeline);

