#include <PCH.h>
#include <EditorPluginAssets/VisualShader/VsCodeGenerator.h>
#include <Foundation/Types/ScopeExit.h>

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

ezVisualShaderCodeGenerator::ezVisualShaderCodeGenerator()
{
  m_pNodeManager = nullptr;
  m_pTypeRegistry = nullptr;
  m_pNodeBaseRtti = nullptr;
  m_pMainNode = nullptr;
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

ezStatus ezVisualShaderCodeGenerator::GenerateVisualShader(const ezDocumentNodeManager* pNodeMaanger, const char* szPlatform)
{
  EZ_ASSERT_DEBUG(m_pNodeManager == nullptr, "Shader Generator cannot be used twice");

  m_pNodeManager = pNodeMaanger;
  m_pTypeRegistry = ezVisualShaderTypeRegistry::GetSingleton();
  m_pNodeBaseRtti = m_pTypeRegistry->GetNodeBaseType();

  EZ_SUCCEED_OR_RETURN(GatherAllNodes(m_pNodeManager->GetRootObject()));

  if (m_Nodes.IsEmpty())
    return ezStatus("Visual Shader graph is empty");

  EZ_SUCCEED_OR_RETURN(GenerateNode(m_pMainNode));

  m_sFinalShaderCode.Set("[PLATFORMS]\nALL\n\n");
  m_sFinalShaderCode.Append("[PERMUTATIONS]\n\n", m_sShaderPermutations, "\n");
  m_sFinalShaderCode.Append("[MATERIALPARAMETER]\n\n", m_sShaderMaterialParam, "\n");
  m_sFinalShaderCode.Append("[RENDERSTATE]\n\n", m_sShaderRenderState, "\n");
  m_sFinalShaderCode.Append("[VERTEXSHADER]\n\n", m_sShaderVertex, "\n");
  m_sFinalShaderCode.Append("[PIXELSHADER]\n\n", m_sShaderPixelDefines, "\n", m_sShaderPixelIncludes, "\n");
  m_sFinalShaderCode.Append(m_sShaderPixelConstants, "\n", m_sShaderPixelSamplers, "\n", m_sShaderPixelBody, "\n");

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

  EZ_SUCCEED_OR_RETURN(GenerateInputPinCode(pDesc->m_InputPins, m_pNodeManager->GetInputPins(pNode)));

  ezStringBuilder sConstantsCode, sBodyCode, sMaterialParamCode, sPixelSamplersCode;

  sConstantsCode = pDesc->m_sShaderCodePixelConstants;
  sBodyCode = pDesc->m_sShaderCodePixelBody;
  sMaterialParamCode = pDesc->m_sShaderCodeMaterialParams;
  sPixelSamplersCode = pDesc->m_sShaderCodePixelSamplers;

  ReplaceInputPinsByCode(pNode, pDesc, sBodyCode);

  EZ_SUCCEED_OR_RETURN(InsertPropertyValues(pNode, pDesc, sConstantsCode));
  EZ_SUCCEED_OR_RETURN(InsertPropertyValues(pNode, pDesc, sBodyCode));
  EZ_SUCCEED_OR_RETURN(InsertPropertyValues(pNode, pDesc, sMaterialParamCode));
  EZ_SUCCEED_OR_RETURN(InsertPropertyValues(pNode, pDesc, sPixelSamplersCode));

  {
    AppendStringIfUnique(m_sShaderPermutations, pDesc->m_sShaderCodePermutations);
    AppendStringIfUnique(m_sShaderRenderState, pDesc->m_sShaderCodeRenderState);
    AppendStringIfUnique(m_sShaderVertex, pDesc->m_sShaderCodeVertexShader);
    AppendStringIfUnique(m_sShaderMaterialParam, sMaterialParamCode);
    AppendStringIfUnique(m_sShaderPixelDefines, pDesc->m_sShaderCodePixelDefines);
    AppendStringIfUnique(m_sShaderPixelIncludes, pDesc->m_sShaderCodePixelIncludes);
    AppendStringIfUnique(m_sShaderPixelBody, sBodyCode);
    AppendStringIfUnique(m_sShaderPixelConstants, sConstantsCode);
    AppendStringIfUnique(m_sShaderPixelSamplers, sPixelSamplersCode);
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
    const ezStatus resNode = GenerateOutputPinCode(pOwnerNode, pPinSource);

    if (resNode.m_Result.Failed())
      return resNode;
  }

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezVisualShaderCodeGenerator::GenerateOutputPinCode(const ezDocumentObject* pOwnerNode, const ezPin* pPin)
{
  OutputPinState& ps = m_OutputPins[pPin];

  if (ps.m_bCodeGenerated)
    return ezStatus(EZ_SUCCESS);

  ps.m_bCodeGenerated = true;

  EZ_SUCCEED_OR_RETURN(GenerateNode(pOwnerNode));

  const ezVisualShaderNodeDescriptor* pDesc = m_pTypeRegistry->GetDescriptorForType(pOwnerNode->GetType());
  const ezUInt16 uiPinID = DeterminePinId(pOwnerNode, pPin);

  ezStringBuilder sInlineCode = pDesc->m_OutputPins[uiPinID].m_sShaderCodeInline;

  ReplaceInputPinsByCode(pOwnerNode, pDesc, sInlineCode);

  EZ_SUCCEED_OR_RETURN(InsertPropertyValues(pOwnerNode, pDesc, sInlineCode));

  // store the result
  ps.m_sCodeAtPin = sInlineCode;

  return ezStatus(EZ_SUCCESS);
}



void ezVisualShaderCodeGenerator::ReplaceInputPinsByCode(const ezDocumentObject* pOwnerNode, const ezVisualShaderNodeDescriptor* pNodeDesc, ezStringBuilder &sInlineCode)
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


void ezVisualShaderCodeGenerator::AppendStringIfUnique(ezStringBuilder& inout_String, const char* szAppend)
{
  if (inout_String.FindSubString(szAppend) != nullptr)
    return;

  inout_String.Append(szAppend);
}

ezStatus ezVisualShaderCodeGenerator::InsertPropertyValues(const ezDocumentObject* pNode, const ezVisualShaderNodeDescriptor* pDesc, ezStringBuilder& sString) const
{
  const auto& TypeAccess = pNode->GetTypeAccessor();

  ezStringBuilder sPropName, sPropValue;

  const auto& props = pDesc->m_Properties;
  for (ezUInt32 p = 0; p < props.GetCount(); ++p)
  {
    sPropName.Format("$prop%u", p);

    const ezVariant value = TypeAccess.GetValue(props[p].m_sName);
    sPropValue = ToShaderString(value);

    sString.ReplaceAll(sPropName, sPropValue);
  }

  return ezStatus(EZ_SUCCESS);
}
