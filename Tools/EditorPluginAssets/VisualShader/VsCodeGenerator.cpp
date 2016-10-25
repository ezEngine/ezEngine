#include <PCH.h>
#include <EditorPluginAssets/VisualShader/VsCodeGenerator.h>
#include <Foundation/Types/ScopeExit.h>

ezVisualShaderCodeGenerator::ezVisualShaderCodeGenerator()
{

}

ezStatus ezVisualShaderCodeGenerator::GenerateVisualShader(const ezDocumentNodeManager* pNodeMaanger, const char* szPlatform)
{
  Clear();

  m_pNodeManager = pNodeMaanger;
  m_pTypeRegistry = ezVisualShaderTypeRegistry::GetSingleton();
  m_pNodeBaseRtti = m_pTypeRegistry->GetNodeBaseType();

  GatherAllNodes(m_pNodeManager->GetRootObject());

  if (m_Nodes.IsEmpty())
    return ezStatus("Visual Shader graph is empty");

  bool bAlreadyFoundMainNode = false;

  for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
  {
    // only start generating code at an output node
    // this will automatically discard all nodes that are not connected to the output
    auto pDesc = m_pTypeRegistry->GetDescriptorForType(it.Key()->GetType());
    if (pDesc->m_NodeType != ezVisualShaderNodeType::Main)
      continue;

    if (bAlreadyFoundMainNode)
      return ezStatus("Shader has multiple output nodes");

    bAlreadyFoundMainNode = true;
    ezStatus res = GenerateCode(it.Key(), it.Value());

    if (res.m_Result.Failed())
      return res;
  }

  m_sFinalShaderCode.Set("[PLATFORMS]\nALL\n\n");
  m_sFinalShaderCode.Append("[PERMUTATIONS]\n\n", m_sShaderPermutations, "\n");
  m_sFinalShaderCode.Append("[MATERIALPARAMETER]\n\n", m_sShaderMaterialParam, "\n");
  m_sFinalShaderCode.Append("[RENDERSTATE]\n\n", m_sShaderRenderState, "\n");
  m_sFinalShaderCode.Append("[VERTEXSHADER]\n\n", m_sShaderVertex, "\n");
  m_sFinalShaderCode.Append("[PIXELSHADER]\n\n", m_sShaderPixelDefines, "\n", m_sShaderPixelIncludes, "\n");
  m_sFinalShaderCode.Append(m_sShaderPixelConstants, "\n", m_sShaderPixelBody, "\n");

  return ezStatus(EZ_SUCCESS);
}

void ezVisualShaderCodeGenerator::GatherAllNodes(const ezDocumentObject* pRootObj)
{
  if (pRootObj->GetType()->IsDerivedFrom(m_pNodeBaseRtti))
  {
    NodeState& ns = m_Nodes[pRootObj];
    ns.m_uiNodeId = m_Nodes.GetCount(); // ID 0 is reserved
    ns.m_bCodeGenerated = false;
    ns.m_bInProgress = false;
  }

  const auto& children = pRootObj->GetChildren();
  for (ezUInt32 i = 0; i < children.GetCount(); ++i)
  {
    GatherAllNodes(children[i]);
  }
}

ezStatus ezVisualShaderCodeGenerator::GenerateCode(const ezDocumentObject* pNode, NodeState& state)
{
  if (state.m_bInProgress)
    return ezStatus("The shader graph has a circular dependency.");

  if (state.m_bCodeGenerated)
    return ezStatus(EZ_SUCCESS);

  state.m_bCodeGenerated = true;
  state.m_bInProgress = true;

  EZ_SCOPE_EXIT(state.m_bInProgress = false);

  const ezVisualShaderNodeDescriptor* pDesc = m_pTypeRegistry->GetDescriptorForType(pNode->GetType());

  ezStatus res = GenerateInputPinCode(pDesc->m_InputPins, m_pNodeManager->GetInputPins(pNode));
  if (res.m_Result.Failed())
    return res;

  {
    ezStringBuilder sConstantsCode, sBodyCode;

    sConstantsCode = pDesc->m_sShaderCodePixelConstants;
    sBodyCode = pDesc->m_sShaderCodePixelBody;

    res = InsertPropertyValues(pNode, state.m_uiNodeId, pDesc, sConstantsCode, sBodyCode);
    if (res.m_Result.Failed())
      return res;

    res = ReplaceOutputPinNames(pNode, state.m_uiNodeId, pDesc, sConstantsCode, sBodyCode);
    if (res.m_Result.Failed())
      return res;

    res = ReplaceInputPinNames(pNode, state.m_uiNodeId, pDesc, sConstantsCode, sBodyCode);
    if (res.m_Result.Failed())
      return res;

    m_sShaderPermutations.Append(pDesc->m_sShaderCodePermutations);
    m_sShaderRenderState.Append(pDesc->m_sShaderCodeRenderState);
    m_sShaderVertex.Append(pDesc->m_sShaderCodeVertexShader);
    m_sShaderMaterialParam.Append(pDesc->m_sShaderCodeMaterialParams);
    m_sShaderPixelDefines.Append(pDesc->m_sShaderCodePixelDefines);
    m_sShaderPixelIncludes.Append(pDesc->m_sShaderCodePixelIncludes);
    m_sShaderPixelBody.Append(sBodyCode);
    m_sShaderPixelConstants.Append(sConstantsCode);
  }

  return ezStatus(EZ_SUCCESS);
}


ezStatus ezVisualShaderCodeGenerator::GenerateInputPinCode(ezArrayPtr<const ezVisualShaderPinDescriptor> pinDesc, ezArrayPtr<ezPin* const> pins)
{
  EZ_ASSERT_DEBUG(pins.GetCount() == pinDesc.GetCount(), "Number of pins on node (%u) and pin descriptors for node (%u) does not match", pins.GetCount(), pinDesc.GetCount());

  for (ezUInt32 i = 0; i < pins.GetCount(); ++i)
  {
    const ezVisualShaderPin* pPin = ezDynamicCast<const ezVisualShaderPin*>(pins[i]);
    EZ_ASSERT_DEBUG(pPin != nullptr, "Invalid pin pointer: %p", pins[i]);

    auto connections = pPin->GetConnections();
    EZ_ASSERT_DEBUG(connections.GetCount() <= 1, "Input pin has %u connections", connections.GetCount());

    if (connections.IsEmpty())
      continue;

    const ezPin* pPinSource = connections[0]->GetSourcePin();
    EZ_ASSERT_DEBUG(pPinSource != nullptr, "Invalid connection");

    // recursively generate all dependent code
    const ezDocumentObject* pOwnerNode = pPinSource->GetParent();
    const ezStatus resNode = GenerateCode(pOwnerNode, m_Nodes[pOwnerNode]);

    if (resNode.m_Result.Failed())
      return resNode;
  }

  return ezStatus(EZ_SUCCESS);
}


static ezString ToShaderString(const ezVariant& value)
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
    {
      ezColor v = value.Get<ezColor>();
      temp.Format("float4(%f, %f, %f, %f)", v.r, v.g, v.b, v.a);
    }
    break;

  case ezVariantType::Vector4:
    {
      ezVec4 v = value.Get<ezVec4>();
      temp.Format("float4(%f, %f, %f, %f)", v.x, v.y, v.z, v.w);
    }
    break;

  case ezVariantType::Vector3:
    {
      ezVec3 v = value.Get<ezVec3>();
      temp.Format("float3(%f, %f, %f)", v.x, v.y, v.z);
    }
    break;

  case ezVariantType::Vector2:
    {
      ezVec2 v = value.Get<ezVec2>();
      temp.Format("float2(%f, %f)", v.x, v.y);
    }
    break;

  case ezVariantType::Float:
    {
      float v = value.Get<float>();
      temp.Format("%f", v);
    }
    break;

  case ezVariantType::Time:
    {
      float v = value.Get<ezTime>().GetSeconds();
      temp.Format("%f", v);
    }
    break;

  case ezVariantType::Angle:
    {
      float v = value.Get<ezAngle>().GetRadian();
      temp.Format("%f", v);
    }
    break;

  default:
    temp = "<Invalid Type>";
    break;
  }

  return temp;
}


ezStatus ezVisualShaderCodeGenerator::InsertPropertyValues(const ezDocumentObject* pNode, ezUInt16 uiNodeID, const ezVisualShaderNodeDescriptor* pDesc, ezStringBuilder& sShaderHeader, ezStringBuilder& sShaderBody) const
{
  ezStringBuilder sPropName, sPropValue;

  const auto& props = pDesc->m_Properties;
  for (ezUInt32 p = 0; p < props.GetCount(); ++p)
  {
    sPropName.Format("$prop%u", p);

    const ezVariant value = pNode->GetTypeAccessor().GetValue(props[p].m_sName);
    sPropValue = ToShaderString(value);

    ezUInt32 repl = 0;
    repl += sShaderHeader.ReplaceAll(sPropName, sPropValue);
    repl += sShaderBody.ReplaceAll(sPropName, sPropValue);

    if (repl == 0)
      return ezStatus("Property '%s' is not referenced in the shader code at all.", props[p].m_sName.GetData());
  }

  return ezStatus(EZ_SUCCESS);
}


ezStatus ezVisualShaderCodeGenerator::ReplaceOutputPinNames(const ezDocumentObject* pNode, ezUInt16 uiNodeId, const ezVisualShaderNodeDescriptor* pDesc, ezStringBuilder& sHeaderCode, ezStringBuilder& sBodyCode)
{
  ezStringBuilder name, pin;
  for (ezUInt32 p = 0; p < pDesc->m_OutputPins.GetCount(); ++p)
  {
    pin.Format("$out%u", p);
    name.Format("node%u_out%u", uiNodeId, p);

    ezUInt32 rep = 0;
    rep += sHeaderCode.ReplaceAll(pin, name);
    rep += sBodyCode.ReplaceAll(pin, name);

    if (rep == 0)
      return ezStatus("Output pin %u is not referenced in node code at all", p);
  }

  return ezStatus(EZ_SUCCESS);
}


ezStatus ezVisualShaderCodeGenerator::ReplaceInputPinNames(const ezDocumentObject* pNode, ezUInt16 uiNodeId, const ezVisualShaderNodeDescriptor* pDesc, ezStringBuilder& sHeaderCode, ezStringBuilder& sBodyCode)
{
  const auto InputPins = m_pNodeManager->GetInputPins(pNode);

  ezStringBuilder sValue, sPinName;

  for (ezUInt32 i = 0; i < InputPins.GetCount(); ++i)
  {
    sPinName.Format("$in%u", i);

    const auto connections = InputPins[i]->GetConnections();

    if (connections.IsEmpty())
    {
      if (pDesc->m_InputPins[i].m_bExposeAsProperty)
      {
        ezVariant val = pNode->GetTypeAccessor().GetValue(pDesc->m_InputPins[i].m_sName);
        sValue = ToShaderString(val);
      }
      else
      {
        sValue = ToShaderString(pDesc->m_InputPins[i].m_DefaultValue);
      }
    }
    else
    {
      const ezPin* pSourcePin = connections[0]->GetSourcePin();
      const ezDocumentObject* pOwner = pSourcePin->GetParent();

      const NodeState& state = m_Nodes[pOwner];
      const ezUInt16 uiSourceNodeID = state.m_uiNodeId;

      const ezUInt16 uiPinID = DeterminePinId(pOwner, pSourcePin);

      if (uiPinID == 0xFFFF)
        return ezStatus("Could not determine ID of source pin in connected input node");

      /// \todo Oh god, why is HLSL so shitty?! Need to pass along the input data through every single function!
      sValue.Format("node%u_out%u(Input)", uiSourceNodeID, uiPinID);
    }

    ezUInt32 rep = 0;
    rep += sHeaderCode.ReplaceAll(sPinName, sValue);
    rep += sBodyCode.ReplaceAll(sPinName, sValue);

    if (rep == 0)
      return ezStatus("Input pin %u is not referenced in node code at all", i);
  }

  return ezStatus(EZ_SUCCESS);
}


ezUInt16 ezVisualShaderCodeGenerator::DeterminePinId(const ezDocumentObject* pOwner, const ezPin* pPin) const
{
  const auto pins = m_pNodeManager->GetOutputPins(pOwner);

  for (ezUInt32 i = 0; i < pins.GetCount(); ++i)
  {
    if (pins[i] == pPin)
      return i;
  }

  return 0xFFFF;
}

void ezVisualShaderCodeGenerator::Clear()
{
  m_Nodes.Clear();
  m_sShaderPixelDefines.Clear();
  m_sShaderPixelIncludes.Clear();
  m_sShaderPixelConstants.Clear();
  m_sShaderPixelBody.Clear();
  m_sShaderVertex.Clear();
  m_sShaderMaterialParam.Clear();
  m_sShaderRenderState.Clear();
  m_sShaderPermutations.Clear();
  m_sFinalShaderCode.Clear();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


ezVisualShaderCodeGenerator2::ezVisualShaderCodeGenerator2()
{
  m_pNodeManager = nullptr;
  m_pTypeRegistry = nullptr;
  m_pNodeBaseRtti = nullptr;
  m_pMainNode = nullptr;
}

ezStatus ezVisualShaderCodeGenerator2::GatherAllNodes(const ezDocumentObject* pRootObj)
{
  if (pRootObj->GetType()->IsDerivedFrom(m_pNodeBaseRtti))
  {
    NodeState& ns = m_Nodes[pRootObj];
    ns.m_uiNodeId = m_Nodes.GetCount(); // ID 0 is reserved
    ns.m_bCodeGenerated = false;
    ns.m_bInProgress = false;

    auto pDesc = m_pTypeRegistry->GetDescriptorForType(pRootObj->GetType());
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
    ezStatus res = GatherAllNodes(children[i]);
    if (res.m_Result.Failed())
      return res;
  }

  return ezStatus(EZ_SUCCESS);
}

ezUInt16 ezVisualShaderCodeGenerator2::DeterminePinId(const ezDocumentObject* pOwner, const ezPin* pPin) const
{
  const auto pins = m_pNodeManager->GetOutputPins(pOwner);

  for (ezUInt32 i = 0; i < pins.GetCount(); ++i)
  {
    if (pins[i] == pPin)
      return i;
  }

  return 0xFFFF;
}

ezStatus ezVisualShaderCodeGenerator2::GenerateVisualShader(const ezDocumentNodeManager* pNodeMaanger, const char* szPlatform)
{
  EZ_ASSERT_DEBUG(m_pNodeManager == nullptr, "Shader Generator cannot be used twice");

  m_pNodeManager = pNodeMaanger;
  m_pTypeRegistry = ezVisualShaderTypeRegistry::GetSingleton();
  m_pNodeBaseRtti = m_pTypeRegistry->GetNodeBaseType();

  GatherAllNodes(m_pNodeManager->GetRootObject());

  if (m_Nodes.IsEmpty())
    return ezStatus("Visual Shader graph is empty");

  ezStatus res = GenerateNode(m_pMainNode);

  if (res.m_Result.Failed())
    return res;

  m_sFinalShaderCode.Set("[PLATFORMS]\nALL\n\n");
  m_sFinalShaderCode.Append("[PERMUTATIONS]\n\n", m_sShaderPermutations, "\n");
  m_sFinalShaderCode.Append("[MATERIALPARAMETER]\n\n", m_sShaderMaterialParam, "\n");
  m_sFinalShaderCode.Append("[RENDERSTATE]\n\n", m_sShaderRenderState, "\n");
  m_sFinalShaderCode.Append("[VERTEXSHADER]\n\n", m_sShaderVertex, "\n");
  m_sFinalShaderCode.Append("[PIXELSHADER]\n\n", m_sShaderPixelDefines, "\n", m_sShaderPixelIncludes, "\n");
  m_sFinalShaderCode.Append(m_sShaderPixelConstants, "\n", m_sShaderPixelBody, "\n");

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezVisualShaderCodeGenerator2::GenerateNode(const ezDocumentObject* pNode)
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

  ezStatus res = GenerateInputPinCode(pDesc->m_InputPins, m_pNodeManager->GetInputPins(pNode));
  if (res.m_Result.Failed())
    return res;

  ezStringBuilder sConstantsCode, sBodyCode;

  sConstantsCode = pDesc->m_sShaderCodePixelConstants;
  sBodyCode = pDesc->m_sShaderCodePixelBody;

  res = InsertPropertyValues(pNode, pDesc, sConstantsCode);
  if (res.m_Result.Failed())
    return res;
  res = InsertPropertyValues(pNode, pDesc, sBodyCode);
  if (res.m_Result.Failed())
    return res;

  //ReplaceInputPinsByCode(pNode, sConstantsCode); // probably not necessary
  ReplaceInputPinsByCode(pNode, pDesc, sBodyCode);


  /// \todo Backwards compatibility hack
  {
    static int unique = 1;
    ezStringBuilder name, repl;

    for (int i = 0; i < 8; ++i)
    {
      ++unique;

      name.Format("$out%i", i);
      repl.Format("ignore%i", unique);

      sConstantsCode.ReplaceAll(name, repl);
      sBodyCode.ReplaceAll(name, repl);
    }

    for (int i = 0; i < 8; ++i)
    {
      ++unique;

      name.Format("$in%i", i);
      repl.Format("ignore%i", unique);

      sConstantsCode.ReplaceAll(name, repl);
      sBodyCode.ReplaceAll(name, repl);
    }
  }

  m_sShaderPermutations.Append(pDesc->m_sShaderCodePermutations);
  m_sShaderRenderState.Append(pDesc->m_sShaderCodeRenderState);
  m_sShaderVertex.Append(pDesc->m_sShaderCodeVertexShader);
  m_sShaderMaterialParam.Append(pDesc->m_sShaderCodeMaterialParams);
  m_sShaderPixelDefines.Append(pDesc->m_sShaderCodePixelDefines);
  m_sShaderPixelIncludes.Append(pDesc->m_sShaderCodePixelIncludes);
  m_sShaderPixelBody.Append(sBodyCode);
  m_sShaderPixelConstants.Append(sConstantsCode);

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezVisualShaderCodeGenerator2::GenerateInputPinCode(ezArrayPtr<const ezVisualShaderPinDescriptor> pinDesc, ezArrayPtr<ezPin* const> pins)
{
  EZ_ASSERT_DEBUG(pins.GetCount() == pinDesc.GetCount(), "Number of pins on node (%u) and pin descriptors for node (%u) does not match", pins.GetCount(), pinDesc.GetCount());

  for (ezUInt32 i = 0; i < pins.GetCount(); ++i)
  {
    const ezVisualShaderPin* pPin = ezDynamicCast<const ezVisualShaderPin*>(pins[i]);
    EZ_ASSERT_DEBUG(pPin != nullptr, "Invalid pin pointer: %p", pins[i]);

    auto connections = pPin->GetConnections();
    EZ_ASSERT_DEBUG(connections.GetCount() <= 1, "Input pin has %u connections", connections.GetCount());

    if (connections.IsEmpty())
      continue;

    const ezPin* pPinSource = connections[0]->GetSourcePin();
    EZ_ASSERT_DEBUG(pPinSource != nullptr, "Invalid connection");

    // recursively generate all dependent code
    const ezDocumentObject* pOwnerNode = pPinSource->GetParent();
    const ezStatus resNode = GenerateOutputPinCode(pOwnerNode, pPinSource);

    if (resNode.m_Result.Failed())
      return resNode;
  }

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezVisualShaderCodeGenerator2::GenerateOutputPinCode(const ezDocumentObject* pOwnerNode, const ezPin* pPin)
{
  OutputPinState& ps = m_OutputPins[pPin];

  if (ps.m_bCodeGenerated)
    return ezStatus(EZ_SUCCESS);

  ps.m_bCodeGenerated = true;

  ezStatus res = GenerateNode(pOwnerNode);
  if (res.m_Result.Failed())
    return res;

  const ezVisualShaderNodeDescriptor* pDesc = m_pTypeRegistry->GetDescriptorForType(pOwnerNode->GetType());
  const ezUInt16 uiPinID = DeterminePinId(pOwnerNode, pPin);

  ezStringBuilder sInlineCode = pDesc->m_OutputPins[uiPinID].m_sShaderCodeInline;

  res = InsertPropertyValues(pOwnerNode, pDesc, sInlineCode);
  if (res.m_Result.Failed())
    return res;

  ReplaceInputPinsByCode(pOwnerNode, pDesc, sInlineCode);

  // store the result
  ps.m_sCodeAtPin = sInlineCode;

  return ezStatus(EZ_SUCCESS);
}



void ezVisualShaderCodeGenerator2::ReplaceInputPinsByCode(const ezDocumentObject* pOwnerNode, const ezVisualShaderNodeDescriptor* pNodeDesc, ezStringBuilder &sInlineCode)
{
  const ezArrayPtr<ezPin* const> inputPins = m_pNodeManager->GetInputPins(pOwnerNode);

  ezStringBuilder sPinName, sValue;

  for (ezUInt32 i = 0; i < inputPins.GetCount(); ++i)
  {
    sPinName.Format("$in%u", i);

    if (inputPins[i]->GetConnections().IsEmpty())
    {
      if (pNodeDesc->m_InputPins[i].m_bExposeAsProperty)
      {
        ezVariant val = pOwnerNode->GetTypeAccessor().GetValue(pNodeDesc->m_InputPins[i].m_sName);
        sValue = ToShaderString(val);
      }
      else
      {
        sValue = ToShaderString(pNodeDesc->m_InputPins[i].m_DefaultValue);
      }

      // replace all occurrences of the pin identifier with the code that was generate for the connected output pin
      sInlineCode.ReplaceAll(sPinName, sValue);
    }
    else
    {
      const ezPin* pOutputPin = inputPins[i]->GetConnections()[0]->GetSourcePin();

      const OutputPinState& pinState = m_OutputPins[pOutputPin];
      EZ_ASSERT_DEBUG(pinState.m_bCodeGenerated, "Pin code should have been generated at this point");

      // replace all occurrences of the pin identifier with the code that was generate for the connected output pin
      sInlineCode.ReplaceAll(sPinName, pinState.m_sCodeAtPin);
    }
  }
}

ezStatus ezVisualShaderCodeGenerator2::InsertPropertyValues(const ezDocumentObject* pNode, const ezVisualShaderNodeDescriptor* pDesc, ezStringBuilder& sString) const
{
  const auto& TypeAccess = pNode->GetTypeAccessor();

  ezStringBuilder sPropName, sPropValue;

  const auto& props = pDesc->m_Properties;
  for (ezUInt32 p = 0; p < props.GetCount(); ++p)
  {
    sPropName.Format("$prop%u", p);

    const ezVariant value = TypeAccess.GetValue(props[p].m_sName);
    sPropValue = ToShaderString(value);

    ezUInt32 repl = 0;
    repl += sString.ReplaceAll(sPropName, sPropValue);

    //if (repl == 0)
    //  return ezStatus("Property '%s' is not referenced in the shader code at all.", props[p].m_sName.GetData());
  }

  return ezStatus(EZ_SUCCESS);
}
