#include <RendererCore/Pipeline/Implementation/RenderPipelinePassGraph.h>

#include <Core/Graphics/Camera.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/ViewData.h>

ezRenderPipelinePassGraph::ezRenderPipelinePassGraph(ezDynamicArray<ezUniquePtr<ezRenderPipelinePass>>&& passes, ezDynamicArray<ezUniquePtr<ezExtractor>>&& extractors, ezArrayPtr<const ezRenderPipelineResourceLoaderConnection> connections)
  : m_Passes(std::move(passes))
  , m_Extractors(std::move(extractors))
{
  SortExtractors();

  m_PassInfos.Reserve(m_Passes.GetCount());
  ezUInt32 uiInputPinCount = 0;
  ezUInt32 uiOutputPinCount = 0;
  ezHashTable<const ezRenderPipelineNodePin*, ezUInt16, ezHashHelper<const ezRenderPipelineNodePin*>, ezTempAllocatorWrapper> PinToIndex;

  auto ResolvePin = [&](ezUInt32 uiPassIdx, const ezRenderPipelineNodePin* pPin) -> ezUInt16
  {
    bool bExisted = false;
    ezUInt16& uiPinIdx = PinToIndex.FindOrAdd(pPin, &bExisted);
    if (!bExisted)
    {
      uiPinIdx = m_Pins.GetCount();
      PinInfo& pinInfo = m_Pins.ExpandAndGetRef();
      pinInfo.m_uiPassIndex = uiPassIdx;
      pinInfo.m_uiInputPinIndex = pPin->m_uiInputIndex;
      pinInfo.m_uiOutputPinIndex = pPin->m_uiOutputIndex;
      pinInfo.m_Flags = pPin->m_Type;
      if (pinInfo.m_Flags.IsSet(ezRenderPipelineNodePin::Type::TextureProvider) && !pinInfo.m_Flags.IsSet(ezRenderPipelineNodePin::Type::Buffer))
      {
        m_TextureProviderPins.PushBack(uiPinIdx);
      }
    }
    return uiPinIdx;
  };

  // Calculate data sizes
  const ezUInt32 uiPassCount = m_Passes.GetCount();
  for (ezUInt32 uiPassIdx = 0; uiPassIdx < uiPassCount; ++uiPassIdx)
  {
    ezRenderPipelinePass* pPass = m_Passes[uiPassIdx].Borrow();
    pPass->InitializePins();
    uiInputPinCount += pPass->GetInputPins().GetCount();
    uiOutputPinCount += pPass->GetOutputPins().GetCount();

    if (ezSwitchBasePass* pSwitch = ezDynamicCast<ezSwitchBasePass*>(pPass))
    {
      SwitchInfo& switchInfo = m_Switches.ExpandAndGetRef();
      switchInfo.m_pSwitch = pSwitch;
      switchInfo.m_sBlackboardProperty.Assign(pSwitch->m_sBlackboardProperty);
    }
  }
  m_Pins.Reserve(uiInputPinCount + uiOutputPinCount);
  PinToIndex.Reserve(uiInputPinCount + uiOutputPinCount);
  m_InputConnectionsStorage.Reserve(uiInputPinCount);
  m_OutputConnectionsStorage.Reserve(uiOutputPinCount);
  m_InputPinsStorage.Reserve(connections.GetCount());

  // Fill m_NodeInfos, m_Pins and PinToIndex
  for (ezUInt32 uiPassIdx = 0; uiPassIdx < uiPassCount; ++uiPassIdx)
  {
    PassInfo& passInfo = m_PassInfos.ExpandAndGetRef();
    ezRenderPipelinePass* pPass = m_Passes[uiPassIdx].Borrow();

    const ezArrayPtr<const ezRenderPipelineNodePin* const> inputs = pPass->GetInputPins();
    ezUInt16 uiInputStartCount = m_InputConnectionsStorage.GetCount();
    for (ezUInt32 uiPinIdx = 0; uiPinIdx < inputs.GetCount(); ++uiPinIdx)
    {
      m_InputConnectionsStorage.PushBack(s_uiInvalidIndex);
      ResolvePin(uiPassIdx, inputs[uiPinIdx]);
    }
    passInfo.m_uiInputConnections = ezMakeArrayPtr<ezUInt16>(m_InputConnectionsStorage.GetData() + uiInputStartCount, inputs.GetCount());

    const ezArrayPtr<const ezRenderPipelineNodePin* const> outputs = pPass->GetOutputPins();
    ezUInt16 uiOutputStartCount = m_OutputConnectionsStorage.GetCount();
    for (ezUInt32 uiPinIdx = 0; uiPinIdx < outputs.GetCount(); ++uiPinIdx)
    {
      m_OutputConnectionsStorage.PushBack(s_uiInvalidIndex);
      ResolvePin(uiPassIdx, outputs[uiPinIdx]);
    }
    passInfo.m_uiOutputConnections = ezMakeArrayPtr<ezUInt16>(m_OutputConnectionsStorage.GetData() + uiOutputStartCount, outputs.GetCount());
  }

  struct Connection
  {
    EZ_DECLARE_POD_TYPE();
    ezUInt16 m_uiSource;
    ezUInt16 m_uiTarget;
    ezUInt16 m_uiSourcePinIndex;
    ezUInt16 m_uiTargetPinIndex;
  };
  ezDynamicArray<Connection> resolvedConnections;
  resolvedConnections.Reserve(connections.GetCount());
  // Resolve connections
  for (ezUInt32 uiConnectionIdx = 0; uiConnectionIdx < connections.GetCount(); ++uiConnectionIdx)
  {
    const ezRenderPipelineResourceLoaderConnection& loaderConn = connections[uiConnectionIdx];
    const ezRenderPipelineNodePin* pSourcePin = m_Passes[loaderConn.m_uiSource]->GetPinByName(ezTempHashedString(loaderConn.m_sSourcePin));
    const ezRenderPipelineNodePin* pTargetPin = m_Passes[loaderConn.m_uiTarget]->GetPinByName(ezTempHashedString(loaderConn.m_sTargetPin));
    if (pSourcePin == nullptr)
    {
      ezLog::Warning("Failed to resolve pin '{0}' on node of type '{1}'", loaderConn.m_sSourcePin, m_Passes[loaderConn.m_uiSource]->GetDynamicRTTI()->GetTypeName());
      continue;
    }
    if (pTargetPin == nullptr)
    {
      ezLog::Warning("Failed to resolve pin '{0}' on node of type '{1}'", loaderConn.m_sTargetPin, m_Passes[loaderConn.m_uiTarget]->GetDynamicRTTI()->GetTypeName());
      continue;
    }
    if (pSourcePin->m_uiOutputIndex == 0xFF)
    {
      ezLog::Error("Failed to connect pin '{0}' of node type '{1}', because it is not an output pin.", loaderConn.m_sSourcePin, m_Passes[loaderConn.m_uiSource]->GetDynamicRTTI()->GetTypeName());
      continue;
    }
    if (pTargetPin->m_uiInputIndex == 0xFF)
    {
      ezLog::Error("Failed to connect to pin '{0}' of node type '{1}', because it is not an input pin.", loaderConn.m_sTargetPin, m_Passes[loaderConn.m_uiTarget]->GetDynamicRTTI()->GetTypeName());
      continue;
    }
    if (pSourcePin->m_Type.IsSet(ezRenderPipelineNodePin::Type::Buffer) != pTargetPin->m_Type.IsSet(ezRenderPipelineNodePin::Type::Buffer))
    {
      ezLog::Error("Failed to connect pin '{0}' of node type '{1}' to pin '{2}' of node type '{3}', because texture pins can't be connected to buffer pins.", loaderConn.m_sSourcePin, m_Passes[loaderConn.m_uiSource]->GetDynamicRTTI()->GetTypeName(), loaderConn.m_sTargetPin, m_Passes[loaderConn.m_uiTarget]->GetDynamicRTTI()->GetTypeName());
      continue;
    }
    Connection& conn = resolvedConnections.ExpandAndGetRef();
    conn.m_uiSource = loaderConn.m_uiSource;
    conn.m_uiTarget = loaderConn.m_uiTarget;
    conn.m_uiSourcePinIndex = pSourcePin->m_uiOutputIndex;
    conn.m_uiTargetPinIndex = pTargetPin->m_uiInputIndex;
  }

  // Sort so we can create arrayPtr out of these.
  resolvedConnections.Sort([](const Connection& lhs, const Connection& rhs) -> bool
    {
      if (lhs.m_uiSource != rhs.m_uiSource)
        return lhs.m_uiSource < rhs.m_uiSource;

      if (lhs.m_uiSourcePinIndex != rhs.m_uiSourcePinIndex)
        return lhs.m_uiSourcePinIndex < rhs.m_uiSourcePinIndex;

      if (lhs.m_uiTarget != rhs.m_uiTarget)
        return lhs.m_uiTarget < rhs.m_uiTarget;

      return lhs.m_uiTargetPinIndex < rhs.m_uiTargetPinIndex; });

  // Add connections by iterating the contiguous chunks of connections
  ezUInt16 uiCurrentSource = s_uiInvalidIndex;
  ezUInt16 uiCurrentSourcePin = s_uiInvalidIndex;
  ezUInt32 uiStartIndex = m_InputPinsStorage.GetCount();
  auto FinishBlock = [&]()
  {
    const ezUInt16 uiConnection = m_Connections.GetCount();
    m_PassInfos[uiCurrentSource].m_uiOutputConnections[uiCurrentSourcePin] = uiConnection;
    const ezRenderPipelinePass* pSourcePass = m_Passes[uiCurrentSource].Borrow();
    ConnectionInfo& newConnection = m_Connections.ExpandAndGetRef();
    newConnection.m_uiOutputPin = ResolvePin(uiCurrentSource, pSourcePass->GetOutputPins()[uiCurrentSourcePin]);
    newConnection.m_uiInputPins = ezMakeArrayPtr<ezUInt16>(m_InputPinsStorage.GetData() + uiStartIndex, m_InputPinsStorage.GetCount() - uiStartIndex);

    for (ezUInt16 uiInputPin : newConnection.m_uiInputPins)
    {
      const PinInfo& inputPin = m_Pins[uiInputPin];
      m_PassInfos[inputPin.m_uiPassIndex].m_uiInputConnections[inputPin.m_uiInputPinIndex] = uiConnection;
    }
  };

  for (ezUInt32 uiConnectionIdx = 0; uiConnectionIdx < resolvedConnections.GetCount(); ++uiConnectionIdx)
  {
    const Connection& conn = resolvedConnections[uiConnectionIdx];
    if (uiCurrentSource != conn.m_uiSource || uiCurrentSourcePin != conn.m_uiSourcePinIndex)
    {
      // Finish old block
      if (uiCurrentSource != s_uiInvalidIndex)
      {
        FinishBlock();
      }

      // Start new block
      uiCurrentSource = conn.m_uiSource;
      uiCurrentSourcePin = conn.m_uiSourcePinIndex;
      uiStartIndex = m_InputPinsStorage.GetCount();
    }
    const ezRenderPipelinePass* pTargetPass = m_Passes[conn.m_uiTarget].Borrow();
    m_InputPinsStorage.PushBack(ResolvePin(conn.m_uiTarget, pTargetPass->GetInputPins()[conn.m_uiTargetPinIndex]));
  }

  // Finish last block
  if (uiCurrentSource != s_uiInvalidIndex)
  {
    FinishBlock();
  }

  m_ConnectionsRenderGraph.SetCount(m_Connections.GetCount());
  for (ezUInt32 i = 0; i < m_Connections.GetCount(); ++i)
  {
    const PinInfo& outputPin = m_Pins[m_Connections[i].m_uiOutputPin];
    const auto connectivity = outputPin.m_Flags.IsSet(ezRenderPipelineNodePin::Type::Buffer) ? ezRenderPipelinePinConnection::Connectivity::Buffer : ezRenderPipelinePinConnection::Connectivity::Texture;
    m_ConnectionsRenderGraph[i] = ezRenderPipelinePinConnection(connectivity);
  }
}

void ezRenderPipelinePassGraph::SortExtractors()
{
  ezDynamicArray<ezUniquePtr<ezExtractor>> sortedExtractors;
  sortedExtractors.Reserve(m_Extractors.GetCount());

  while (!m_Extractors.IsEmpty())
  {
    bool bMadeProgress = false;
    for (ezUInt32 i = 0; i < m_Extractors.GetCount(); ++i)
    {
      bool bDependenciesFound = true;
      for (const ezHashedString& sDependency : m_Extractors[i]->m_DependsOn)
      {
        bool bFound = false;
        for (const ezUniquePtr<ezExtractor>& pExtractor : sortedExtractors)
        {
          if (sDependency == ezTempHashedString(pExtractor->GetDynamicRTTI()->GetTypeNameHash()))
          {
            bFound = true;
            break;
          }
        }
        if (!bFound)
        {
          bDependenciesFound = false;
          break;
        }
      }

      if (bDependenciesFound)
      {
        sortedExtractors.PushBack(std::move(m_Extractors[i]));
        m_Extractors.RemoveAtAndCopy(i);
        bMadeProgress = true;
        break;
      }
    }

    if (!bMadeProgress)
    {
      ezLog::Error("GPU pipeline contains missing or cyclic extractor dependencies.");
      break;
    }
  }

  for (ezUniquePtr<ezExtractor>& pExtractor : m_Extractors)
  {
    sortedExtractors.PushBack(std::move(pExtractor));
  }
  m_Extractors = std::move(sortedExtractors);
}

ezRenderPipelinePass* ezRenderPipelinePassGraph::GetPassByName(ezStringView sName) const
{
  for (const ezUniquePtr<ezRenderPipelinePass>& pPass : m_Passes)
  {
    if (sName.IsEqual(pPass->GetName()))
      return pPass.Borrow();
  }
  return nullptr;
}

ezExtractor* ezRenderPipelinePassGraph::GetExtractorByName(ezStringView sName) const
{
  for (const ezUniquePtr<ezExtractor>& pExtractor : m_Extractors)
  {
    if (sName.IsEqual(pExtractor->GetName()))
      return pExtractor.Borrow();
  }
  return nullptr;
}

bool ezRenderPipelinePassGraph::IsPassAlive(const ezRenderPipelinePass* pPass) const
{
  for (ezUInt32 i = 0; i < m_Passes.GetCount(); ++i)
  {
    if (m_Passes[i].Borrow() == pPass)
      return i < m_AlivePasses.GetCount() && m_AlivePasses.IsBitSet(i);
  }
  return false;
}

bool ezRenderPipelinePassGraph::SetSwitchValue(ezUInt32 uiSwitchIndex, ezInt32 iValue)
{
  EZ_ASSERT_DEV(uiSwitchIndex < m_Switches.GetCount(), "Invalid GPU pipeline switch index");
  return m_Switches[uiSwitchIndex].m_pSwitch->SetSwitchValue(iValue);
}

bool ezRenderPipelinePassGraph::SetSwitchToDefault(ezUInt32 uiSwitchIndex)
{
  EZ_ASSERT_DEV(uiSwitchIndex < m_Switches.GetCount(), "Invalid GPU pipeline switch index");
  ezSwitchBasePass* pSwitch = m_Switches[uiSwitchIndex].m_pSwitch;
  if (pSwitch->m_Values.IsEmpty())
    return false;

  return pSwitch->SetSwitchValue(pSwitch->m_Values[0]);
}

ezResult ezRenderPipelinePassGraph::CullDeadPasses()
{
  m_AlivePasses.Clear();
  m_AlivePasses.SetCount(m_Passes.GetCount(), false);
  m_AliveConnections.Clear();
  m_AliveConnections.SetCount(m_Connections.GetCount(), false);

  ezTempArray<ezUInt16> stack;

  // Passes without outputs are the observable roots of the pipeline.
  for (ezUInt16 i = 0; i < m_PassInfos.GetCount(); ++i)
  {
    if (m_PassInfos[i].m_uiOutputConnections.IsEmpty())
    {
      m_AlivePasses.SetBit(i);
      stack.PushBack(i);
    }
  }

  // Walk backwards through all required input connections. Switch passes only keep the input
  // selected by m_uiSelectedValueIndex, so their other branches are never reached.
  while (!stack.IsEmpty())
  {
    const ezUInt16 uiPass = stack.PeekBack();
    stack.PopBack();

    const PassInfo& pass = m_PassInfos[uiPass];
    ezUInt32 uiFirstInput = 0;
    ezUInt32 uiInputCount = pass.m_uiInputConnections.GetCount();

    if (const ezSwitchBasePass* pSwitch = ezDynamicCast<const ezSwitchBasePass*>(m_Passes[uiPass].Borrow()))
    {
      if (pSwitch->m_uiSelectedValueIndex >= pSwitch->m_Values.GetCount() || pSwitch->m_uiSelectedValueIndex >= pass.m_uiInputConnections.GetCount())
      {
        ezLog::Error("Switch '{}' has an invalid selected value index {}.", pSwitch->GetName(), pSwitch->m_uiSelectedValueIndex);
        return EZ_FAILURE;
      }

      uiFirstInput = pSwitch->m_uiSelectedValueIndex;
      uiInputCount = 1;
    }

    for (ezUInt32 i = 0; i < uiInputCount; ++i)
    {
      const ezUInt16 uiConnection = pass.m_uiInputConnections[uiFirstInput + i];
      if (uiConnection == s_uiInvalidIndex)
        continue;

      m_AliveConnections.SetBit(uiConnection);
      const ezUInt16 uiSourcePass = m_Pins[m_Connections[uiConnection].m_uiOutputPin].m_uiPassIndex;
      if (!m_AlivePasses.IsBitSet(uiSourcePass))
      {
        m_AlivePasses.SetBit(uiSourcePass);
        stack.PushBack(uiSourcePass);
      }
    }
  }

  return EZ_SUCCESS;
}

ezResult ezRenderPipelinePassGraph::SortPasses()
{
  using TempBitfield = ezBitfield<ezTempArray<ezUInt32>>;

  m_SortedPasses.Clear();
  ezTempArray<ezUInt32> totalDependencies;
  ezTempArray<ezUInt32> fulfilledDependencies;
  totalDependencies.SetCount(m_Passes.GetCount(), 0);
  fulfilledDependencies.SetCount(m_Passes.GetCount(), 0);

  ezUInt32 uiAlivePassCount = 0;
  for (ezUInt32 uiPass = 0; uiPass < m_Passes.GetCount(); ++uiPass)
  {
    if (m_AlivePasses.IsBitSet(uiPass))
      ++uiAlivePassCount;
  }
  m_SortedPasses.Reserve(uiAlivePassCount);

  // Switches forward the resource of their selected input. Map every connection to the connection that actually produces its resource, so that pass-through inputs can be ordered after all readers of that resource.
  ezTempArray<const ezSwitchBasePass*> passSwitches;
  passSwitches.SetCount(m_Passes.GetCount());
  for (ezUInt32 uiPass = 0; uiPass < m_Passes.GetCount(); ++uiPass)
  {
    passSwitches[uiPass] = ezDynamicCast<const ezSwitchBasePass*>(m_Passes[uiPass].Borrow());
  }

  ezTempArray<ezUInt16> resourceConnections;
  resourceConnections.SetCount(m_Connections.GetCount());
  for (ezUInt16 uiConnection = 0; uiConnection < m_Connections.GetCount(); ++uiConnection)
  {
    ezUInt16 uiResource = uiConnection;

    // bounded, a cycle of switches is reported by the sorting below
    for (ezUInt32 uiDepth = 0; uiDepth < m_Connections.GetCount(); ++uiDepth)
    {
      const ezUInt16 uiSourcePass = m_Pins[m_Connections[uiResource].m_uiOutputPin].m_uiPassIndex;
      const ezSwitchBasePass* pSwitch = passSwitches[uiSourcePass];
      const ezArrayPtr<ezUInt16> switchInputs = m_PassInfos[uiSourcePass].m_uiInputConnections;
      if (pSwitch == nullptr || pSwitch->m_uiSelectedValueIndex >= switchInputs.GetCount() || switchInputs[pSwitch->m_uiSelectedValueIndex] == s_uiInvalidIndex)
        break;

      uiResource = switchInputs[pSwitch->m_uiSelectedValueIndex];
    }

    resourceConnections[uiConnection] = uiResource;
  }

  // input pins of alive non-switch passes, indexed by resource connection
  ezDynamicArray<ezHybridArray<ezUInt16, 4>> resourceConsumers;
  resourceConsumers.SetCount(m_Connections.GetCount());
  for (ezUInt16 uiConnection = 0; uiConnection < m_Connections.GetCount(); ++uiConnection)
  {
    if (!m_AliveConnections.IsBitSet(uiConnection))
      continue;

    for (ezUInt16 uiInputPin : m_Connections[uiConnection].m_uiInputPins)
    {
      const ezUInt16 uiPass = m_Pins[uiInputPin].m_uiPassIndex;
      if (m_AlivePasses.IsBitSet(uiPass) && passSwitches[uiPass] == nullptr)
        resourceConsumers[resourceConnections[uiConnection]].PushBack(uiInputPin);
    }
  }

  bool bConflictingPassThrough = false;

  // Count source dependencies and the additional dependencies of pass-through inputs on all other consumers of the same resource.
  for (ezUInt16 uiConnection = 0; uiConnection < m_Connections.GetCount(); ++uiConnection)
  {
    if (!m_AliveConnections.IsBitSet(uiConnection))
      continue;

    for (ezUInt16 uiInputPin : m_Connections[uiConnection].m_uiInputPins)
    {
      const ezUInt16 uiTargetPass = m_Pins[uiInputPin].m_uiPassIndex;
      if (!m_AlivePasses.IsBitSet(uiTargetPass))
        continue;

      ++totalDependencies[uiTargetPass];

      if (m_Pins[uiInputPin].m_Flags.IsSet(ezRenderPipelineNodePin::Type::PassThrough) && passSwitches[uiTargetPass] == nullptr)
      {
        // the pass modifies the resource in place, all other readers have to run first
        for (ezUInt16 uiConsumerPin : resourceConsumers[resourceConnections[uiConnection]])
        {
          const ezUInt16 uiConsumerPass = m_Pins[uiConsumerPin].m_uiPassIndex;
          if (uiConsumerPass == uiTargetPass)
            continue;

          ++totalDependencies[uiTargetPass];

          // each pair is seen from both sides
          if (m_Pins[uiConsumerPin].m_Flags.IsSet(ezRenderPipelineNodePin::Type::PassThrough) && uiConsumerPin > uiInputPin)
          {
            ezLog::Error("Passes '{}' and '{}' both modify the same resource in place.", m_Passes[uiTargetPass]->GetName(), m_Passes[uiConsumerPass]->GetName());
            bConflictingPassThrough = true;
          }
        }
      }
    }
  }

  if (bConflictingPassThrough)
    return EZ_FAILURE;

  TempBitfield done;
  done.SetCount(m_Passes.GetCount(), false);
  ezTempArray<ezUInt16> usable;
  usable.Reserve(m_Passes.GetCount());
  for (ezUInt32 uiPass = 0; uiPass < m_Passes.GetCount(); ++uiPass)
  {
    if (m_AlivePasses.IsBitSet(uiPass) && totalDependencies[uiPass] == 0)
      usable.PushBack(uiPass);
  }

  auto DependencyFulfilled = [&](ezUInt16 uiPass)
  {
    EZ_ASSERT_DEBUG(fulfilledDependencies[uiPass] < totalDependencies[uiPass], "GPU pipeline dependency counted more than once");
    ++fulfilledDependencies[uiPass];
    if (fulfilledDependencies[uiPass] == totalDependencies[uiPass])
      usable.PushBack(uiPass);
  };

  while (!usable.IsEmpty())
  {
    const ezUInt16 uiPass = usable.PeekBack();
    usable.PopBack();

    EZ_ASSERT_DEBUG(!done.IsBitSet(uiPass), "GPU pipeline pass was queued more than once");
    done.SetBit(uiPass);
    m_SortedPasses.PushBack(uiPass);

    // Completing the source pass fulfills the regular dependency of every alive consumer.
    for (ezUInt16 uiConnection : m_PassInfos[uiPass].m_uiOutputConnections)
    {
      if (uiConnection == s_uiInvalidIndex || !m_AliveConnections.IsBitSet(uiConnection))
        continue;

      for (ezUInt16 uiInputPin : m_Connections[uiConnection].m_uiInputPins)
      {
        const ezUInt16 uiTargetPass = m_Pins[uiInputPin].m_uiPassIndex;
        if (m_AlivePasses.IsBitSet(uiTargetPass))
          DependencyFulfilled(uiTargetPass);
      }
    }

    // Fulfills the additional dependency of the pass-through consumer of the same resource. There is at most one, which is checked above.
    if (passSwitches[uiPass] == nullptr)
    {
      for (ezUInt16 uiConnection : m_PassInfos[uiPass].m_uiInputConnections)
      {
        if (uiConnection == s_uiInvalidIndex || !m_AliveConnections.IsBitSet(uiConnection))
          continue;

        for (ezUInt16 uiConsumerPin : resourceConsumers[resourceConnections[uiConnection]])
        {
          const ezUInt16 uiConsumerPass = m_Pins[uiConsumerPin].m_uiPassIndex;
          if (uiConsumerPass != uiPass && m_Pins[uiConsumerPin].m_Flags.IsSet(ezRenderPipelineNodePin::Type::PassThrough))
            DependencyFulfilled(uiConsumerPass);
        }
      }
    }
  }

  if (m_SortedPasses.GetCount() != uiAlivePassCount)
  {
    ezLog::Error("GPU pipeline contains a cycle or unresolved alive dependency.");
    for (ezUInt16 i = 0; i < m_Passes.GetCount(); ++i)
    {
      if (m_AlivePasses.IsBitSet(i) && !done.IsBitSet(i))
        ezLog::Error("Failed to sort pass '{}' of type '{}'.", m_Passes[i]->GetName(), m_Passes[i]->GetDynamicRTTI()->GetTypeName());
    }
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezStatus ezRenderPipelinePassGraph::AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph)
{
  ezHybridArray<ezRenderPipelinePinConnection, 10> inputs(ezFrameAllocator::GetCurrentAllocator());
  ezHybridArray<ezRenderPipelinePinConnection, 10> outputs(ezFrameAllocator::GetCurrentAllocator());

  for (ezUInt16 uiPass : m_SortedPasses)
  {
    ezRenderPipelinePass* pPass = m_Passes[uiPass].Borrow();
    const PassInfo& passInfo = m_PassInfos[uiPass];

    if (camera.IsStereoscopic() && !pPass->IsStereoAware())
    {
      ezLog::Error("View '{0}' uses a stereoscopic camera, but the render pass '{1}' does not support stereo rendering!", viewData.m_sName, pPass->GetName());
    }

    inputs.SetCount(passInfo.m_uiInputConnections.GetCount());
    for (ezUInt32 i = 0; i < inputs.GetCount(); ++i)
    {
      const ezUInt16 uiConnection = passInfo.m_uiInputConnections[i];
      inputs[i] = uiConnection != s_uiInvalidIndex && m_AliveConnections.IsBitSet(uiConnection) ? m_ConnectionsRenderGraph[uiConnection] : ezRenderPipelinePinConnection();
    }

    outputs.SetCount(passInfo.m_uiOutputConnections.GetCount());
    for (ezUInt32 i = 0; i < outputs.GetCount(); ++i)
    {
      const ezUInt16 uiConnection = passInfo.m_uiOutputConnections[i];
      if (uiConnection == s_uiInvalidIndex || !m_AliveConnections.IsBitSet(uiConnection))
      {
        outputs[i] = ezRenderPipelinePinConnection();
        continue;
      }

      outputs[i] = m_ConnectionsRenderGraph[uiConnection];
      const ezRenderPipelineNodePin* pPin = pPass->GetOutputPins()[i];
      if (pPin->m_Type.IsSet(ezRenderPipelineNodePin::Type::PassThrough))
      {
        outputs[i] = inputs[pPin->m_uiInputIndex];
      }
    }

    ref_graph.PushMarker(pPass->GetName());
    const ezStatus result = pPass->AddRenderPasses(viewData, camera, ref_graph, inputs, outputs);
    ref_graph.PopMarker();
    if (result.Failed())
      return ezStatus(ezFmt("Pass '{}' ({}): {}", pPass->GetName(), pPass->GetDynamicRTTI()->GetTypeName(), result.GetMessageString()));

    for (ezUInt32 i = 0; i < passInfo.m_uiOutputConnections.GetCount(); ++i)
    {
      const ezUInt16 uiConnection = passInfo.m_uiOutputConnections[i];
      if (uiConnection != s_uiInvalidIndex && m_AliveConnections.IsBitSet(uiConnection))
      {
        m_ConnectionsRenderGraph[uiConnection] = outputs[i];
      }
    }
  }

  return EZ_SUCCESS;
}

ezStatus ezRenderPipelinePassGraph::UpdateTextureProviders(ezRenderGraph& ref_graph)
{
  ezHashTable<ezRenderGraphTextureHandle, ezUInt16> updates(ezFrameAllocator::GetCurrentAllocator());
  updates.Reserve(m_TextureProviderPins.GetCount());

  for (ezUInt16 uiPin : m_TextureProviderPins)
  {
    const PinInfo& pin = m_Pins[uiPin];
    const PassInfo& pass = m_PassInfos[pin.m_uiPassIndex];
    const ezUInt16 uiConnection = pin.m_uiInputPinIndex != 0xFF ? pass.m_uiInputConnections[pin.m_uiInputPinIndex] : pass.m_uiOutputConnections[pin.m_uiOutputPinIndex];
    if (uiConnection == s_uiInvalidIndex || !m_AliveConnections.IsBitSet(uiConnection))
      continue;

    const ezRenderGraphTextureHandle hTexture = m_ConnectionsRenderGraph[uiConnection].m_TextureHandle;
    if (updates.Contains(hTexture))
      return ezStatus("Two texture provider pins reference the same texture.");
    updates.Insert(hTexture, uiPin);
  }

  for (auto it : updates)
  {
    const PinInfo& pin = m_Pins[it.Value()];
    ezRenderPipelinePass* pPass = m_Passes[pin.m_uiPassIndex].Borrow();
    const ezRenderPipelineNodePin* pNodePin = pin.m_uiInputPinIndex != 0xFF ? pPass->GetInputPins()[pin.m_uiInputPinIndex] : pPass->GetOutputPins()[pin.m_uiOutputPinIndex];
    const ezGALTextureCreationDescription& desc = ref_graph.GetTextureDesc(it.Key());
    const ezGALTextureHandle hTexture = pPass->QueryTextureProvider(pNodePin, desc);
    if (!hTexture.IsInvalidated())
    {
      EZ_SUCCEED_OR_RETURN(ref_graph.ReplaceImportedTexture(it.Key(), hTexture));
    }
  }

  return EZ_SUCCESS;
}
