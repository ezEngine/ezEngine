#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/VisualShader/VsCodeGenerator.h>

namespace
{
  // Returns the dimension of a type: 1 for float, 2 for ezVec2, 3 for ezVec3, 4 for ezVec4/ezColor, 0 for unknown
  ezUInt8 GetTypeDimension(const ezRTTI* pType)
  {
    if (pType == ezGetStaticRTTI<float>())
      return 1;
    if (pType == ezGetStaticRTTI<ezVec2>())
      return 2;
    if (pType == ezGetStaticRTTI<ezVec3>())
      return 3;
    if (pType == ezGetStaticRTTI<ezVec4>() || pType == ezGetStaticRTTI<ezColor>())
      return 4;
    // We treat 'auto' as float as all the default values for auto fields are float.
    if (pType == nullptr)
      return 1;

    EZ_REPORT_FAILURE("Unknown RTTI type found in VSE type");
    return 0;
  }

  // Returns the HLSL type string for a given dimension
  const char* DimensionToHlslType(ezUInt8 uiDimension)
  {
    switch (uiDimension)
    {
      case 1:
        return "float";
      case 2:
        return "float2";
      case 3:
        return "float3";
      case 4:
        return "float4";
      default:
        EZ_REPORT_FAILURE("Unknown vector dimension found in HLSL type");
        return "float4";
    }
  }

  ezString ToShaderString(const ezVariant& value)
  {
    ezStringBuilder temp;

    switch (value.GetType())
    {
      case ezVariantType::String:
      {
        temp = value.Get<ezString>();
      }
      break;

      case ezVariantType::Color:
      case ezVariantType::ColorGamma:
      {
        ezColor v = value.ConvertTo<ezColor>();
        temp.SetFormat("float4({0}, {1}, {2}, {3})", v.r, v.g, v.b, v.a);
      }
      break;

      case ezVariantType::Vector4:
      {
        ezVec4 v = value.Get<ezVec4>();
        temp.SetFormat("float4({0}, {1}, {2}, {3})", v.x, v.y, v.z, v.w);
      }
      break;

      case ezVariantType::Vector3:
      {
        ezVec3 v = value.Get<ezVec3>();
        temp.SetFormat("float3({0}, {1}, {2})", v.x, v.y, v.z);
      }
      break;

      case ezVariantType::Vector2:
      {
        ezVec2 v = value.Get<ezVec2>();
        temp.SetFormat("float2({0}, {1})", v.x, v.y);
      }
      break;

      case ezVariantType::Float:
      case ezVariantType::Int32:
      case ezVariantType::Bool:
      {
        temp.SetFormat("{0}", value);
      }
      break;

      case ezVariantType::Time:
      {
        float v = value.Get<ezTime>().GetSeconds();
        temp.SetFormat("{0}", v);
      }
      break;

      case ezVariantType::Angle:
      {
        float v = value.Get<ezAngle>().GetRadian();
        temp.SetFormat("{0}", v);
      }
      break;

      default:
        temp = "<Invalid Type>";
        break;
    }

    return temp;
  }
} // namespace

ezVisualShaderCodeGenerator::ezVisualShaderCodeGenerator()
{
  m_pNodeManager = nullptr;
  m_pTypeRegistry = nullptr;
  m_pNodeBaseRtti = nullptr;
  m_pMainNode = nullptr;
}

void ezVisualShaderCodeGenerator::DetermineConfigFileDependencies(const ezDocumentNodeManager* pNodeManager, ezSet<ezString>& out_cfgFiles)
{
  out_cfgFiles.Clear();

  m_pNodeManager = pNodeManager;
  m_pTypeRegistry = ezVisualShaderTypeRegistry::GetSingleton();
  m_pNodeBaseRtti = m_pTypeRegistry->GetNodeBaseType();

  if (GatherAllNodes(pNodeManager->GetRootObject()).Failed())
    return;

  for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
  {
    auto pDesc = m_pTypeRegistry->GetDescriptorForType(it.Key()->GetType());

    out_cfgFiles.Insert(pDesc->m_sCfgFile);
  }
}

void ezVisualShaderCodeGenerator::CollectReachableNodes(const ezDocumentObject* pRootNode, ezHashSet<const ezDocumentObject*>& out_Nodes) const
{
  out_Nodes.Clear();
  ezHybridArray<const ezDocumentObject*, 64> nodeStack;
  nodeStack.PushBack(pRootNode);

  while (!nodeStack.IsEmpty())
  {
    const ezDocumentObject* pNode = nodeStack.PeekBack();
    nodeStack.PopBack();
    out_Nodes.Insert(pNode);

    ezArrayPtr<const ezUniquePtr<const ezPin>> inputPins = m_pNodeManager->GetInputPins(pNode);
    for (ezUInt32 i = 0; i < inputPins.GetCount(); ++i)
    {
      const ezUniquePtr<const ezPin>& pInputPin = inputPins[i];
      ezArrayPtr<const ezConnection* const> connections = m_pNodeManager->GetConnections(*pInputPin);
      for (const auto* pConnection : connections)
      {
        const ezDocumentObject* pParentNode = pConnection->GetSourcePin().GetParent();
        if (out_Nodes.Contains(pParentNode))
          continue;

        if (!nodeStack.Contains(pParentNode))
          nodeStack.PushBack(pParentNode);
      }
    }
  }
}

ezString ezVisualShaderCodeGenerator::GetInputPinDefaultValue(const ezDocumentObject* pNode, const ezVisualShaderPinDescriptor& pinDesc, ezStringBuilder* pDefinesOut)
{
  if (pinDesc.m_bExposeAsProperty)
  {
    ezVariant val = pNode->GetTypeAccessor().GetValue(pinDesc.m_sName);
    return ToShaderString(val);
  }
  else
  {
    if (pDefinesOut != nullptr)
    {
      for (const auto& sDefine : pinDesc.m_sDefinesWhenUsingDefaultValue)
      {
        pDefinesOut->Append("#if !defined(", sDefine, ")\n");
        pDefinesOut->Append("  #define ", sDefine, "\n");
        pDefinesOut->Append("#endif\n");
      }
    }
    return pinDesc.m_sDefaultValue;
  }
}

ezResult ezVisualShaderCodeGenerator::CollectNodesInTopologicalOrder(const ezDocumentObject* pRootNode, ezDynamicArray<const ezDocumentObject*>& out_Sorted) const
{
  // We need the reachable nodes to ignore connections that go out of this subtree as they are not relevant.
  ezHashSet<const ezDocumentObject*> reachableNodes;
  CollectReachableNodes(pRootNode, reachableNodes);

  ezHashSet<const ezDocumentObject*> visitedNodes;
  visitedNodes.Reserve(reachableNodes.GetCount());
  ezHybridArray<const ezDocumentObject*, 64> nodeStack;
  nodeStack.PushBack(pRootNode);

  while (!nodeStack.IsEmpty())
  {
    // Find the next node in which all outgoing connections are visited nodes or nodes outside the reachable nodes.
    const ezDocumentObject* pNode = nullptr;
    for (ezUInt32 i = nodeStack.GetCount(); i-- > 0;)
    {
      const ezDocumentObject* pCandidateNode = nodeStack[i];
      bool bAllVisited = true;
      ezArrayPtr<const ezUniquePtr<const ezPin>> outputPins = m_pNodeManager->GetOutputPins(pCandidateNode);
      for (auto& pOutputPin : outputPins)
      {
        ezArrayPtr<const ezConnection* const> connections = m_pNodeManager->GetConnections(*pOutputPin);
        for (const auto* pConnection : connections)
        {
          const ezDocumentObject* pParentNode = pConnection->GetTargetPin().GetParent();
          if (reachableNodes.Contains(pParentNode) && !visitedNodes.Contains(pParentNode))
          {
            bAllVisited = false;
            break;
          }
        }
        if (!bAllVisited)
          break;
      }

      if (bAllVisited)
      {
        pNode = pCandidateNode;
        nodeStack.RemoveAtAndCopy(i);
        break;
      }
    }

    if (pNode == nullptr)
    {
      EZ_REPORT_FAILURE("Execution connection corrupted or loop detected");
      return EZ_FAILURE;
    }

    EZ_VERIFY(visitedNodes.Insert(pNode) == false, "Every node should only be visited once");
    out_Sorted.PushBack(pNode);

    // Add all incoming connections of the node to the nodeStack.
    ezArrayPtr<const ezUniquePtr<const ezPin>> inputPins = m_pNodeManager->GetInputPins(pNode);
    for (ezUInt32 i = 0; i < inputPins.GetCount(); ++i)
    {
      const ezUniquePtr<const ezPin>& pInputPin = inputPins[i];
      ezArrayPtr<const ezConnection* const> connections = m_pNodeManager->GetConnections(*pInputPin);
      for (const auto* pConnection : connections)
      {
        const ezDocumentObject* pParentNode = pConnection->GetSourcePin().GetParent();
        if (nodeStack.Contains(pParentNode) == false)
          nodeStack.PushBack(pParentNode);
      }
    }
  }

  return EZ_SUCCESS;
}

void ezVisualShaderCodeGenerator::ComputeOutputPinDimensions()
{
  m_OutputPinDimensions.Clear();

  // Find all root nodes (nodes with no outputs connected)
  ezHybridArray<const ezDocumentObject*, 16> rootNodes;
  for (auto& nodeIt : m_Nodes)
  {
    const ezDocumentObject* pNode = nodeIt.Key();
    auto outputPins = m_pNodeManager->GetOutputPins(pNode);

    bool bHasConnectedOutput = false;
    for (auto& pOutPin : outputPins)
    {
      if (!m_pNodeManager->GetConnections(*pOutPin).IsEmpty())
      {
        bHasConnectedOutput = true;
        break;
      }
    }

    if (!bHasConnectedOutput)
    {
      rootNodes.PushBack(pNode);
    }
  }

  // Process each root node's subgraph in topological order
  for (const ezDocumentObject* pRootNode : rootNodes)
  {
    ezDynamicArray<const ezDocumentObject*> sortedNodes;
    if (CollectNodesInTopologicalOrder(pRootNode, sortedNodes).Failed())
      continue;

    // Process nodes in reverse topological order (leaves first, root last) so that input dimensions are computed before nodes that depend on them
    for (ezUInt32 i = sortedNodes.GetCount(); i-- > 0;)
    {
      const ezDocumentObject* pNode = sortedNodes[i];
      const ezVisualShaderNodeDescriptor* pDesc = m_pTypeRegistry->GetDescriptorForType(pNode->GetType());

      auto outputPins = m_pNodeManager->GetOutputPins(pNode);
      for (ezUInt32 pinIdx = 0; pinIdx < outputPins.GetCount(); ++pinIdx)
      {
        const ezPin* pOutPin = outputPins[pinIdx].Borrow();

        // Skip if already computed (node may be shared between subgraphs)
        if (m_OutputPinDimensions.Contains(pOutPin))
          continue;

        // If output type is explicit, use it directly
        if (pDesc->m_OutputPins[pinIdx].m_pDataType != nullptr)
        {
          ezUInt8 dim = GetTypeDimension(pDesc->m_OutputPins[pinIdx].m_pDataType);
          if (dim == 0)
            dim = 1;
          m_OutputPinDimensions[pOutPin] = dim;
          continue;
        }

        // "auto" type - compute from max of input dimensions. Since we're in topological order, all inputs should already be computed.
        ezUInt8 effectiveDim = 0;
        auto inputPins = m_pNodeManager->GetInputPins(pNode);
        for (ezUInt32 inIdx = 0; inIdx < inputPins.GetCount(); ++inIdx)
        {
          auto inConnections = m_pNodeManager->GetConnections(*inputPins[inIdx]);
          if (!inConnections.IsEmpty())
          {
            const ezPin* pConnectedPin = &inConnections[0]->GetSourcePin();
            EZ_ASSERT_DEV(m_OutputPinDimensions.Contains(pConnectedPin), "Topological order should have processed all inputs first");
            effectiveDim = ezMath::Max(effectiveDim, m_OutputPinDimensions[pConnectedPin]);
          }
          else
          {
            // No connection - use declared input type
            effectiveDim = ezMath::Max(effectiveDim, GetTypeDimension(pDesc->m_InputPins[inIdx].m_pDataType));
          }
        }

        if (effectiveDim == 0)
          effectiveDim = 1;
        m_OutputPinDimensions[pOutPin] = effectiveDim;
      }
    }
  }
}

void ezVisualShaderCodeGenerator::GenerateInputHelperFunction(const ezPin* pInputPin, ezUInt32 uiInputIndex, ezStringBuilder& out_sFunctionCode, ezStringBuilder& out_sFunctionCall)
{
  out_sFunctionCode.Clear();
  out_sFunctionCall.Clear();

  // Generate linearized code for a main node input: local variables for each node in the subgraph
  auto connections = m_pNodeManager->GetConnections(*pInputPin);
  if (connections.IsEmpty())
    return;

  const ezPin* pSourcePin = &connections[0]->GetSourcePin();
  ezDynamicArray<const ezDocumentObject*> sortedNodes;
  CollectNodesInTopologicalOrder(pSourcePin->GetParent(), sortedNodes).AssertSuccess();

  // Map from output pin to local variable name
  ezMap<const ezPin*, ezString> pinToVarName;
  ezStringBuilder sLocalVars;

  // Generate local variable for each node's outputs in topological order (sortedNodes needs to be reversed as the array starts at pSourcePin's node)
  for (ezUInt32 i = sortedNodes.GetCount(); i-- > 0;)
  {
    const ezDocumentObject* pNode = sortedNodes[i];
    const ezVisualShaderNodeDescriptor* pDesc = m_pTypeRegistry->GetDescriptorForType(pNode->GetType());
    const NodeState& nodeState = m_Nodes[pNode];

    auto outputPins = m_pNodeManager->GetOutputPins(pNode);
    for (ezUInt32 pinIdx = 0; pinIdx < outputPins.GetCount(); ++pinIdx)
    {
      const ezPin* pOutPin = outputPins[pinIdx].Borrow();

      // Skip pins that aren't connected within the subgraph
      if (!m_pNodeManager->HasConnections(*pOutPin))
        continue;

      ezUInt8 effectiveDim = m_OutputPinDimensions[pOutPin];
      // Generate the expression for this output pin, using local variables for inputs
      ezStringBuilder sExpr = pDesc->m_OutputPins[pinIdx].m_sShaderCodeInline;

      // Replace input pin placeholders with local variable names or values
      auto inputPins = m_pNodeManager->GetInputPins(pNode);
      for (ezInt32 inIdx = (ezInt32)inputPins.GetCount() - 1; inIdx >= 0; --inIdx)
      {
        ezStringBuilder sPinPlaceholder;
        sPinPlaceholder.SetFormat("$in{0}", inIdx);

        auto inConnections = m_pNodeManager->GetConnections(*inputPins[inIdx]);
        if (inConnections.IsEmpty())
        {
          ezString sValue = GetInputPinDefaultValue(pNode, pDesc->m_InputPins[inIdx]);
          sExpr.ReplaceAll(sPinPlaceholder, sValue);
        }
        else
        {
          // Use local variable from connected output pin
          const ezPin* pConnectedPin = &inConnections[0]->GetSourcePin();
          EZ_ASSERT_DEV(pinToVarName.Contains(pConnectedPin), "Topological sort should have processed this pin already");
          sExpr.ReplaceAll(sPinPlaceholder, pinToVarName[pConnectedPin].GetData());
        }
      }

      // Replace property placeholders
      InsertPropertyValues(pNode, pDesc, sExpr).IgnoreResult();

      // Generate local variable with descriptive name including node type
      ezStringBuilder sVarName;
      sVarName.SetFormat("_{0}_{1}_{2}", pDesc->m_sName, nodeState.m_uiNodeId, pOutPin->GetName());
      pinToVarName[pOutPin] = sVarName;

      // Add local variable declaration with the computed type
      sLocalVars.AppendFormat("  {0} {1} = {2};\n", DimensionToHlslType(effectiveDim), sVarName, sExpr);
    }
  }

  // Get the final variable name for the source pin
  EZ_ASSERT_DEV(pinToVarName.Contains(pSourcePin), "Topological sort should have processed the source pin");
  const ezString& sFinalVar = pinToVarName[pSourcePin];

  // Generate the complete helper function
  out_sFunctionCall.SetFormat("_{0}_{1}()", pInputPin->GetName(), uiInputIndex);
  out_sFunctionCode.SetFormat("float4 {0} {\n{1}  return ToFloat4Direction({2});\n}\n\n",
    out_sFunctionCall, sLocalVars, sFinalVar);
}

ezStatus ezVisualShaderCodeGenerator::GatherAllNodes(const ezDocumentObject* pRootObj)
{
  if (pRootObj->GetType()->IsDerivedFrom(m_pNodeBaseRtti))
  {
    NodeState& ns = m_Nodes[pRootObj];
    ns.m_uiNodeId = m_Nodes.GetCount(); // ID 0 is reserved
    ns.m_bCodeGenerated = false;
    ns.m_bInProgress = false;

    auto pDesc = m_pTypeRegistry->GetDescriptorForType(pRootObj->GetType());

    if (pDesc == nullptr)
      return ezStatus("Node type of root node is unknown");

    if (pDesc->m_NodeType == ezVisualShaderNodeType::Main)
    {
      if (m_pMainNode != nullptr)
        return ezStatus("Shader has multiple output nodes");

      m_pMainNode = pRootObj;
    }
  }

  const auto& children = pRootObj->GetChildren();
  for (ezUInt32 i = 0; i < children.GetCount(); ++i)
  {
    EZ_SUCCEED_OR_RETURN(GatherAllNodes(children[i]));
  }

  return ezStatus(EZ_SUCCESS);
}

ezUInt16 ezVisualShaderCodeGenerator::DeterminePinId(const ezDocumentObject* pOwner, const ezPin& pin) const
{
  const auto pins = m_pNodeManager->GetOutputPins(pOwner);

  for (ezUInt32 i = 0; i < pins.GetCount(); ++i)
  {
    if (pins[i] == &pin)
      return i;
  }

  return 0xFFFF;
}

ezStatus ezVisualShaderCodeGenerator::GenerateVisualShader(const ezDocumentNodeManager* pNodeManager, ezStringBuilder& out_sCheckPerms)
{
  out_sCheckPerms.Clear();

  EZ_ASSERT_DEBUG(m_pNodeManager == nullptr, "Shader Generator cannot be used twice");

  m_pNodeManager = pNodeManager;
  m_pTypeRegistry = ezVisualShaderTypeRegistry::GetSingleton();
  m_pNodeBaseRtti = m_pTypeRegistry->GetNodeBaseType();

  EZ_SUCCEED_OR_RETURN(GatherAllNodes(m_pNodeManager->GetRootObject()));

  if (m_Nodes.IsEmpty())
    return ezStatus("Visual Shader graph is empty");

  if (m_pMainNode == nullptr)
    return ezStatus("Visual Shader does not contain an output node");

  // Compute effective output type dimensions for all output pins (needed for "auto" types and GUI)
  ComputeOutputPinDimensions();

  EZ_SUCCEED_OR_RETURN(GenerateNode(m_pMainNode));

  // now also generate code for certain nodes, even if they have no connections
  // ShaderState nodes generally have no connections
  // Parameter and Texture nodes are user inputs, if we don't output them, the UI won't show them
  // and previously selected values get lost. That's very undesirable, so we always add them to the shader,
  // even if they are currently not used.
  for (auto itNode : m_Nodes)
  {
    if (itNode.Value().m_bCodeGenerated)
      continue;

    auto pDesc = m_pTypeRegistry->GetDescriptorForType(itNode.Key()->GetType());

    if (pDesc->m_NodeType == ezVisualShaderNodeType::ShaderState || pDesc->m_NodeType == ezVisualShaderNodeType::Parameter || pDesc->m_NodeType == ezVisualShaderNodeType::Texture)
    {
      EZ_SUCCEED_OR_RETURN(GenerateNode(itNode.Key()));
    }
  }

  ezStringBuilder sMaterialConstants = m_sShaderMaterialConstants;
  sMaterialConstants.ReplaceAll("VSE_CONSTANTS", m_sShaderMaterialCB);

  m_sFinalShaderCode.Set("[PLATFORMS]\nALL\n\n");
  m_sFinalShaderCode.Append("[PERMUTATIONS]\n\n", m_sShaderPermutations, "\n");
  m_sFinalShaderCode.Append("[MATERIALPARAMETER]\n\n", m_sShaderMaterialParam, "\n");
  for (auto it : m_MaterialParameter)
  {
    m_sFinalShaderCode.Append(it.Value());
  }
  m_sFinalShaderCode.Append("\n");

  m_sFinalShaderCode.Append("[RENDERSTATE]\n\n", m_sShaderRenderState, "\n");
  m_sFinalShaderCode.Append("[MATERIALCONSTANTS]\n\n", sMaterialConstants, "\n");

  m_sFinalShaderCode.Append("[VERTEXSHADER]\n\n", m_sShaderVertexDefines, "\n", m_sShaderVertexIncludes, "\n");
  m_sFinalShaderCode.Append(m_sShaderVertexBody, "\n");

  m_sFinalShaderCode.Append("[PIXELSHADER]\n\n", m_sShaderPixelDefines, "\n", m_sShaderPixelIncludes, "\n");
  m_sFinalShaderCode.Append(m_sShaderPixelConstants, "\n", m_sShaderPixelSamplers, "\n");
  m_sFinalShaderCode.Append(m_sShaderPixelBody, "\n");

  for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
  {
    auto pDesc = m_pTypeRegistry->GetDescriptorForType(it.Key()->GetType());
    out_sCheckPerms.Append("\n", pDesc->m_sCheckPermutations);
  }

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezVisualShaderCodeGenerator::GenerateNode(const ezDocumentObject* pNode)
{
  NodeState& state = m_Nodes[pNode];

  if (state.m_bInProgress)
    return ezStatus("The shader graph has a circular dependency.");

  if (state.m_bCodeGenerated)
    return ezStatus(EZ_SUCCESS);

  state.m_bCodeGenerated = true;
  state.m_bInProgress = true;

  EZ_SCOPE_EXIT(state.m_bInProgress = false);

  const ezVisualShaderNodeDescriptor* pDesc = m_pTypeRegistry->GetDescriptorForType(pNode->GetType());

  EZ_SUCCEED_OR_RETURN(GenerateInputPinCode(m_pNodeManager->GetInputPins(pNode)));

  ezStringBuilder sPixelConstantsCode, sPixelBody, sMaterialParamCode, sMaterialConstantsCode, sPixelSamplersCode, sMaterialCB, sPermutations, sRenderStates, sPixelDefines, sPixelIncludes, sVertexDefines, sVertexIncludes, sVertexBody;

  // Pixel shader sections
  sPixelDefines = pDesc->m_sShaderCodeShaderShared;
  sPixelDefines.Append(pDesc->m_sShaderCodePixelDefines);
  sPixelIncludes = pDesc->m_sShaderCodePixelIncludes;
  sPixelConstantsCode = pDesc->m_sShaderCodePixelConstants;
  sPixelBody = pDesc->m_sShaderCodePixelBody;
  sPixelSamplersCode = pDesc->m_sShaderCodePixelSamplers;

  // Vertex shader sections
  sVertexDefines = pDesc->m_sShaderCodeShaderShared;
  sVertexDefines.Append(pDesc->m_sShaderCodeVertexDefines);
  sVertexIncludes = pDesc->m_sShaderCodeVertexIncludes;
  sVertexBody = pDesc->m_sShaderCodeVertexBody;

  // Material and other sections
  sMaterialParamCode = pDesc->m_sShaderCodeMaterialParams;
  sMaterialConstantsCode = pDesc->m_sShaderCodeMaterialConstants;
  sMaterialCB = pDesc->m_sShaderCodeMaterialCB;
  sPermutations = pDesc->m_sShaderCodePermutations;
  sRenderStates = pDesc->m_sShaderCodeRenderState;

  // For the main node, use linearized code generation for both pixel and vertex body
  if (pNode == m_pMainNode)
  {
    ezStringBuilder sPsHelperFunctions;
    ezStringBuilder sVsHelperFunctions;
    ReplaceMainNodeInputPins(pNode, pDesc, sPixelBody, sPixelDefines, sPsHelperFunctions);
    ReplaceMainNodeInputPins(pNode, pDesc, sVertexBody, sVertexDefines, sVsHelperFunctions);

    AppendStringIfUnique(m_sShaderPixelBody, sPsHelperFunctions);
    AppendStringIfUnique(m_sShaderVertexBody, sVsHelperFunctions);
  }
  else
  {
    EZ_SUCCEED_OR_RETURN(ReplaceInputPinsByCode(pNode, pDesc, sPixelBody, sPixelDefines));
    EZ_SUCCEED_OR_RETURN(ReplaceInputPinsByCode(pNode, pDesc, sVertexBody, sVertexDefines));
  }

  EZ_SUCCEED_OR_RETURN(CheckPropertyValues(pNode, pDesc));
  EZ_SUCCEED_OR_RETURN(InsertPropertyValues(pNode, pDesc, sPixelConstantsCode));
  EZ_SUCCEED_OR_RETURN(InsertPropertyValues(pNode, pDesc, sVertexBody));
  EZ_SUCCEED_OR_RETURN(InsertPropertyValues(pNode, pDesc, sPixelBody));
  EZ_SUCCEED_OR_RETURN(InsertPropertyValues(pNode, pDesc, sMaterialParamCode));
  EZ_SUCCEED_OR_RETURN(InsertPropertyValues(pNode, pDesc, sMaterialConstantsCode));
  EZ_SUCCEED_OR_RETURN(InsertPropertyValues(pNode, pDesc, sPixelDefines));
  EZ_SUCCEED_OR_RETURN(InsertPropertyValues(pNode, pDesc, sVertexDefines));
  EZ_SUCCEED_OR_RETURN(InsertPropertyValues(pNode, pDesc, sMaterialCB));
  EZ_SUCCEED_OR_RETURN(InsertPropertyValues(pNode, pDesc, sPixelSamplersCode));
  EZ_SUCCEED_OR_RETURN(InsertPropertyValues(pNode, pDesc, sRenderStates));

  SetPinDefines(pNode, sPermutations);
  SetPinDefines(pNode, sRenderStates);
  SetPinDefines(pNode, sVertexBody);
  SetPinDefines(pNode, sVertexDefines);
  SetPinDefines(pNode, sMaterialParamCode);
  SetPinDefines(pNode, sMaterialConstantsCode);
  SetPinDefines(pNode, sPixelDefines);
  SetPinDefines(pNode, sPixelIncludes);
  SetPinDefines(pNode, sPixelBody);
  SetPinDefines(pNode, sPixelConstantsCode);
  SetPinDefines(pNode, sPixelSamplersCode);
  SetPinDefines(pNode, sMaterialCB);

  {
    AppendStringIfUnique(m_sShaderPermutations, sPermutations);
    AppendStringIfUnique(m_sShaderRenderState, sRenderStates);
    AppendStringIfUnique(m_sShaderVertexDefines, sVertexDefines);
    AppendStringIfUnique(m_sShaderVertexIncludes, sVertexIncludes);
    AppendStringIfUnique(m_sShaderMaterialConstants, sMaterialConstantsCode);
    AppendStringIfUnique(m_sShaderPixelDefines, sPixelDefines);
    AppendStringIfUnique(m_sShaderPixelIncludes, sPixelIncludes);
    AppendStringIfUnique(m_sShaderPixelBody, sPixelBody);
    AppendStringIfUnique(m_sShaderVertexBody, sVertexBody);
    AppendStringIfUnique(m_sShaderPixelConstants, sPixelConstantsCode);
    AppendStringIfUnique(m_sShaderPixelSamplers, sPixelSamplersCode);
    AppendStringIfUnique(m_sShaderMaterialCB, sMaterialCB);
  }

  if (pDesc->m_NodeType == ezVisualShaderNodeType::Texture || pDesc->m_NodeType == ezVisualShaderNodeType::Parameter)
  {
    const ezStringView sPropertyName = (pDesc->m_NodeType == ezVisualShaderNodeType::Texture) ? "Name" : "ParamName";
    const ezVariant value = pNode->GetTypeAccessor().GetValue(sPropertyName);
    if (value.IsString() || value.IsHashedString())
    {
      m_MaterialParameter.Insert(value.ConvertTo<ezString>(), sMaterialParamCode);
    }
  }
  else
  {
    AppendStringIfUnique(m_sShaderMaterialParam, sMaterialParamCode);
  }

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezVisualShaderCodeGenerator::GenerateInputPinCode(ezArrayPtr<const ezUniquePtr<const ezPin>> pins)
{
  for (auto& pPin : pins)
  {
    auto connections = m_pNodeManager->GetConnections(*pPin);
    EZ_ASSERT_DEBUG(connections.GetCount() <= 1, "Input pin has {0} connections", connections.GetCount());

    if (connections.IsEmpty())
      continue;

    const ezPin& pinSource = connections[0]->GetSourcePin();

    // recursively generate all dependent code
    const ezDocumentObject* pOwnerNode = pinSource.GetParent();
    const ezStatus resNode = GenerateOutputPinCode(pOwnerNode, pinSource);

    if (resNode.Failed())
      return resNode;
  }

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezVisualShaderCodeGenerator::GenerateOutputPinCode(const ezDocumentObject* pOwnerNode, const ezPin& pin)
{
  OutputPinState& ps = m_OutputPins[&pin];

  if (ps.m_bCodeGenerated)
    return ezStatus(EZ_SUCCESS);

  ps.m_bCodeGenerated = true;

  EZ_SUCCEED_OR_RETURN(GenerateNode(pOwnerNode));

  const ezVisualShaderNodeDescriptor* pDesc = m_pTypeRegistry->GetDescriptorForType(pOwnerNode->GetType());
  const ezUInt16 uiPinID = DeterminePinId(pOwnerNode, pin);

  ezStringBuilder sInlineCode = pDesc->m_OutputPins[uiPinID].m_sShaderCodeInline;
  ezStringBuilder ignore; // DefineWhenUsingDefaultValue not used for output pins

  EZ_SUCCEED_OR_RETURN(ReplaceInputPinsByCode(pOwnerNode, pDesc, sInlineCode, ignore));

  EZ_SUCCEED_OR_RETURN(InsertPropertyValues(pOwnerNode, pDesc, sInlineCode));

  // store the result
  ps.m_sCodeAtPin = sInlineCode;

  return ezStatus(EZ_SUCCESS);
}



void ezVisualShaderCodeGenerator::ReplaceMainNodeInputPins(const ezDocumentObject* pMainNode, const ezVisualShaderNodeDescriptor* pNodeDesc, ezStringBuilder& sInlineCode, ezStringBuilder& sCodeForPlacingDefines, ezStringBuilder& out_sHelperFunctions)
{
  out_sHelperFunctions.Clear();

  auto inputPins = m_pNodeManager->GetInputPins(pMainNode);

  for (ezInt32 i = (ezInt32)inputPins.GetCount() - 1; i >= 0; --i)
  {
    ezStringBuilder sPinPlaceholder;
    sPinPlaceholder.SetFormat("$in{0}", i);

    // Check if this shader section uses this placeholder
    const bool bPresentInCode = sInlineCode.FindSubString(sPinPlaceholder) != nullptr;
    // Even if this shader section is not using this placeholder, we need to insert the defines into sCodeForPlacingDefines as these must be equal between shader stages.
    auto connections = m_pNodeManager->GetConnections(*inputPins[i]);
    if (connections.IsEmpty())
    {
      ezString sValue = GetInputPinDefaultValue(pMainNode, pNodeDesc->m_InputPins[i], &sCodeForPlacingDefines);
      if (bPresentInCode)
        sInlineCode.ReplaceAll(sPinPlaceholder, sValue);
      continue;
    }

    // Do generate and add the helper function to a shader stage that does not call the function.
    if (!bPresentInCode)
      continue;

    // Generate the helper function for this input's subgraph
    ezStringBuilder sHelperFunc, sFuncCall;
    GenerateInputHelperFunction(inputPins[i].Borrow(), i, sHelperFunc, sFuncCall);

    out_sHelperFunctions.Append(sHelperFunc);
    sInlineCode.ReplaceAll(sPinPlaceholder, sFuncCall);
  }
}

ezStatus ezVisualShaderCodeGenerator::ReplaceInputPinsByCode(
  const ezDocumentObject* pOwnerNode, const ezVisualShaderNodeDescriptor* pNodeDesc, ezStringBuilder& sInlineCode, ezStringBuilder& sCodeForPlacingDefines)
{
  auto inputPins = m_pNodeManager->GetInputPins(pOwnerNode);

  ezStringBuilder sPinName, sValue;

  for (ezUInt32 i0 = inputPins.GetCount(); i0 > 0; --i0)
  {
    const ezUInt32 i = i0 - 1;

    sPinName.SetFormat("$in{0}", i);

    auto connections = m_pNodeManager->GetConnections(*inputPins[i]);
    if (connections.IsEmpty())
    {
      sValue = GetInputPinDefaultValue(pOwnerNode, pNodeDesc->m_InputPins[i], &sCodeForPlacingDefines);

      if (sValue.IsEmpty())
      {
        return ezStatus(ezFmt("Not all required input pins on a '{0}' node are connected.", pNodeDesc->m_sName));
      }

      // replace all occurrences of the pin identifier with the code that was generate for the connected output pin
      sInlineCode.ReplaceAll(sPinName, sValue);
    }
    else
    {
      const ezPin& outputPin = connections[0]->GetSourcePin();

      const OutputPinState& pinState = m_OutputPins[&outputPin];
      EZ_ASSERT_DEBUG(pinState.m_bCodeGenerated, "Pin code should have been generated at this point");

      // replace all occurrences of the pin identifier with the code that was generate for the connected output pin
      sInlineCode.ReplaceAll(sPinName, pinState.m_sCodeAtPin);
    }
  }

  return ezStatus(EZ_SUCCESS);
}


void ezVisualShaderCodeGenerator::SetPinDefines(const ezDocumentObject* pOwnerNode, ezStringBuilder& sInlineCode)
{
  ezStringBuilder sDefineName;

  {
    auto pins = m_pNodeManager->GetInputPins(pOwnerNode);

    for (ezUInt32 i = 0; i < pins.GetCount(); ++i)
    {
      sDefineName.SetFormat("INPUT_PIN_{0}_CONNECTED", i);

      if (m_pNodeManager->HasConnections(*pins[i]) == false)
      {
        sInlineCode.ReplaceAll(sDefineName, "0");
      }
      else
      {
        sInlineCode.ReplaceAll(sDefineName, "1");
      }
    }
  }

  {
    auto pins = m_pNodeManager->GetOutputPins(pOwnerNode);

    for (ezUInt32 i = 0; i < pins.GetCount(); ++i)
    {
      sDefineName.SetFormat("OUTPUT_PIN_{0}_CONNECTED", i);

      if (m_pNodeManager->HasConnections(*pins[i]) == false)
      {
        sInlineCode.ReplaceAll(sDefineName, "0");
      }
      else
      {
        sInlineCode.ReplaceAll(sDefineName, "1");
      }
    }
  }
}

void ezVisualShaderCodeGenerator::AppendStringIfUnique(ezStringBuilder& inout_String, ezStringView sAppend)
{
  if (sAppend.IsEmpty() || inout_String.FindSubString(sAppend) != nullptr)
    return;

  inout_String.Append(sAppend);
}

ezStatus ezVisualShaderCodeGenerator::CheckPropertyValues(const ezDocumentObject* pNode, const ezVisualShaderNodeDescriptor* pDesc)
{
  const auto& TypeAccess = pNode->GetTypeAccessor();

  ezStringBuilder sPropValue;

  const auto& props = pDesc->m_Properties;
  for (ezUInt32 p = 0; p < props.GetCount(); ++p)
  {
    const ezVariant value = TypeAccess.GetValue(props[p].m_sName);
    sPropValue = ToShaderString(value);


    const ezInt8 iUniqueValueGroup = pDesc->m_UniquePropertyValueGroups[p];
    if (iUniqueValueGroup > 0)
    {
      if (sPropValue.IsEmpty())
      {
        return ezStatus(ezFmt("A '{0}' node has an empty '{1}' property.", pDesc->m_sName, props[p].m_sName));
      }

      if (!ezStringUtils::IsValidIdentifierName(sPropValue))
      {
        return ezStatus(ezFmt("A '{0}' node has a '{1}' property that is not a valid identifier: '{2}'. Only letters, digits and _ are allowed.",
          pDesc->m_sName, props[p].m_sName, sPropValue));
      }

      // Check if this identifier is already used by a different node type
      ezString* pExistingNodeType = m_UsedIdentifiers.GetValue(sPropValue);
      if (pExistingNodeType != nullptr)
      {
        if (*pExistingNodeType != pDesc->m_sName)
        {
          return ezStatus(ezFmt("Identifier '{0}' is being used both by a '{1}' node and a '{2}' node.",
            sPropValue, *pExistingNodeType, pDesc->m_sName));
        }
        // Same node type is allowed to reuse the identifier
      }
      else
      {
        // First time seeing this identifier, store the node type
        m_UsedIdentifiers.Insert(sPropValue, pDesc->m_sName);
      }
    }
  }

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezVisualShaderCodeGenerator::InsertPropertyValues(
  const ezDocumentObject* pNode, const ezVisualShaderNodeDescriptor* pDesc, ezStringBuilder& sString)
{
  const auto& TypeAccess = pNode->GetTypeAccessor();

  ezStringBuilder sPropName, sPropValue;

  const auto& props = pDesc->m_Properties;
  for (ezUInt32 p0 = props.GetCount(); p0 > 0; --p0)
  {
    const ezUInt32 p = p0 - 1;

    sPropName.SetFormat("$prop{0}", p);

    const ezVariant value = TypeAccess.GetValue(props[p].m_sName);
    sPropValue = ToShaderString(value);

    sString.ReplaceAll(sPropName, sPropValue);
  }

  return ezStatus(EZ_SUCCESS);
}
