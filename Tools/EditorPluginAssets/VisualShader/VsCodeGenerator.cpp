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
    case ezVariantType::ColorGamma:
    {
      ezColor v = value.ConvertTo<ezColor>();
      temp.Format("float4({0}, {1}, {2}, {3})", v.r, v.g, v.b, v.a);
    }
    break;

    case ezVariantType::Vector4:
    {
      ezVec4 v = value.Get<ezVec4>();
      temp.Format("float4({0}, {1}, {2}, {3})", v.x, v.y, v.z, v.w);
    }
    break;

    case ezVariantType::Vector3:
    {
      ezVec3 v = value.Get<ezVec3>();
      temp.Format("float3({0}, {1}, {2})", v.x, v.y, v.z);
    }
    break;

    case ezVariantType::Vector2:
    {
      ezVec2 v = value.Get<ezVec2>();
      temp.Format("float2({0}, {1})", v.x, v.y);
    }
    break;

    case ezVariantType::Float:
    case ezVariantType::Int32:
    case ezVariantType::Bool:
    {
      temp.Format("{0}", value);
    }
    break;

    case ezVariantType::Time:
    {
      float v = value.Get<ezTime>().GetSeconds();
      temp.Format("{0}", v);
    }
    break;

    case ezVariantType::Angle:
    {
      float v = value.Get<ezAngle>().GetRadian();
      temp.Format("{0}", v);
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

ezStatus ezVisualShaderCodeGenerator::GenerateVisualShader(const ezDocumentNodeManager* pNodeManager, const char* szPlatform,
                                                           ezStringBuilder& out_sCheckPerms)
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

  EZ_SUCCEED_OR_RETURN(GenerateNode(m_pMainNode));

  const ezStringBuilder sMaterialCBDefine("#define VSE_CONSTANTS ", m_sShaderMaterialCB);

  m_sFinalShaderCode.Set("[PLATFORMS]\nALL\n\n");
  m_sFinalShaderCode.Append("[PERMUTATIONS]\n\n", m_sShaderPermutations, "\n");
  m_sFinalShaderCode.Append("[MATERIALPARAMETER]\n\n", m_sShaderMaterialParam, "\n");
  m_sFinalShaderCode.Append("[RENDERSTATE]\n\n", m_sShaderRenderState, "\n");
  m_sFinalShaderCode.Append("[VERTEXSHADER]\n\n", sMaterialCBDefine, "\n\n", m_sShaderVertex, "\n");
  m_sFinalShaderCode.Append("[GEOMETRYSHADER]\n\n", sMaterialCBDefine, "\n\n", m_sShaderGeometry, "\n");
  m_sFinalShaderCode.Append("[PIXELSHADER]\n\n", sMaterialCBDefine, "\n\n");
  m_sFinalShaderCode.Append(m_sShaderPixelDefines, "\n", m_sShaderPixelIncludes, "\n");
  m_sFinalShaderCode.Append(m_sShaderPixelConstants, "\n", m_sShaderPixelSamplers, "\n", m_sShaderPixelBody, "\n");

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

  ezStringBuilder sConstantsCode, sPsBodyCode, sMaterialParamCode, sPixelSamplersCode, sVsBodyCode, sGsBodyCode, sMaterialCB, sPermutations,
      sRenderStates, sPixelDefines, sPixelIncludes;

  sConstantsCode = pDesc->m_sShaderCodePixelConstants;
  sPsBodyCode = pDesc->m_sShaderCodePixelBody;
  sMaterialParamCode = pDesc->m_sShaderCodeMaterialParams;
  sPixelSamplersCode = pDesc->m_sShaderCodePixelSamplers;
  sVsBodyCode = pDesc->m_sShaderCodeVertexShader;
  sGsBodyCode = pDesc->m_sShaderCodeGeometryShader;
  sMaterialCB = pDesc->m_sShaderCodeMaterialCB;
  sPermutations = pDesc->m_sShaderCodePermutations;
  sRenderStates = pDesc->m_sShaderCodeRenderState;
  sPixelDefines = pDesc->m_sShaderCodePixelDefines;
  sPixelIncludes = pDesc->m_sShaderCodePixelIncludes;

  EZ_SUCCEED_OR_RETURN(ReplaceInputPinsByCode(pNode, pDesc, sPsBodyCode));
  EZ_SUCCEED_OR_RETURN(ReplaceInputPinsByCode(pNode, pDesc, sVsBodyCode));
  EZ_SUCCEED_OR_RETURN(ReplaceInputPinsByCode(pNode, pDesc, sGsBodyCode));

  EZ_SUCCEED_OR_RETURN(CheckPropertyValues(pNode, pDesc));
  EZ_SUCCEED_OR_RETURN(InsertPropertyValues(pNode, pDesc, sConstantsCode));
  EZ_SUCCEED_OR_RETURN(InsertPropertyValues(pNode, pDesc, sVsBodyCode));
  EZ_SUCCEED_OR_RETURN(InsertPropertyValues(pNode, pDesc, sGsBodyCode));
  EZ_SUCCEED_OR_RETURN(InsertPropertyValues(pNode, pDesc, sPsBodyCode));
  EZ_SUCCEED_OR_RETURN(InsertPropertyValues(pNode, pDesc, sMaterialParamCode));
  EZ_SUCCEED_OR_RETURN(InsertPropertyValues(pNode, pDesc, sPixelDefines));
  EZ_SUCCEED_OR_RETURN(InsertPropertyValues(pNode, pDesc, sMaterialCB));
  EZ_SUCCEED_OR_RETURN(InsertPropertyValues(pNode, pDesc, sPixelSamplersCode));

  SetPinDefines(pNode, sPermutations);
  SetPinDefines(pNode, sRenderStates);
  SetPinDefines(pNode, sVsBodyCode);
  SetPinDefines(pNode, sGsBodyCode);
  SetPinDefines(pNode, sMaterialParamCode);
  SetPinDefines(pNode, sPixelDefines);
  SetPinDefines(pNode, sPixelIncludes);
  SetPinDefines(pNode, sPsBodyCode);
  SetPinDefines(pNode, sConstantsCode);
  SetPinDefines(pNode, sPixelSamplersCode);
  SetPinDefines(pNode, sMaterialCB);

  {
    AppendStringIfUnique(m_sShaderPermutations, sPermutations);
    AppendStringIfUnique(m_sShaderRenderState, sRenderStates);
    AppendStringIfUnique(m_sShaderVertex, sVsBodyCode);
    AppendStringIfUnique(m_sShaderGeometry, sGsBodyCode);
    AppendStringIfUnique(m_sShaderMaterialParam, sMaterialParamCode);
    AppendStringIfUnique(m_sShaderPixelDefines, sPixelDefines);
    AppendStringIfUnique(m_sShaderPixelIncludes, sPixelIncludes);
    AppendStringIfUnique(m_sShaderPixelBody, sPsBodyCode);
    AppendStringIfUnique(m_sShaderPixelConstants, sConstantsCode);
    AppendStringIfUnique(m_sShaderPixelSamplers, sPixelSamplersCode);
    AppendStringIfUnique(m_sShaderMaterialCB, sMaterialCB);
  }

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezVisualShaderCodeGenerator::GenerateInputPinCode(ezArrayPtr<ezPin* const> pins)
{
  for (ezUInt32 i = 0; i < pins.GetCount(); ++i)
  {
    const ezVisualShaderPin* pPin = ezDynamicCast<const ezVisualShaderPin*>(pins[i]);
    EZ_ASSERT_DEBUG(pPin != nullptr, "Invalid pin pointer: {0}", ezArgP(pins[i]));

    auto connections = pPin->GetConnections();
    EZ_ASSERT_DEBUG(connections.GetCount() <= 1, "Input pin has {0} connections", connections.GetCount());

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



ezStatus ezVisualShaderCodeGenerator::ReplaceInputPinsByCode(const ezDocumentObject* pOwnerNode,
                                                             const ezVisualShaderNodeDescriptor* pNodeDesc, ezStringBuilder& sInlineCode)
{
  const ezArrayPtr<ezPin* const> inputPins = m_pNodeManager->GetInputPins(pOwnerNode);

  ezStringBuilder sPinName, sValue;

  for (ezUInt32 i0 = inputPins.GetCount(); i0 > 0; --i0)
  {
    const ezUInt32 i = i0 - 1;

    sPinName.Format("$in{0}", i);

    if (inputPins[i]->GetConnections().IsEmpty())
    {
      if (pNodeDesc->m_InputPins[i].m_bExposeAsProperty)
      {
        ezVariant val = pOwnerNode->GetTypeAccessor().GetValue(pNodeDesc->m_InputPins[i].m_sName);
        sValue = ToShaderString(val);
      }
      else
      {
        sValue = pNodeDesc->m_InputPins[i].m_sDefaultValue;
      }

      if (sValue.IsEmpty())
      {
        return ezStatus(ezFmt("Not all required input pins on a '{0}' node are connected.", pNodeDesc->m_sName));
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

  return ezStatus(EZ_SUCCESS);
}


void ezVisualShaderCodeGenerator::SetPinDefines(const ezDocumentObject* pOwnerNode, ezStringBuilder& sInlineCode)
{
  ezStringBuilder sDefineName;

  {
    const ezArrayPtr<ezPin* const> pins = m_pNodeManager->GetInputPins(pOwnerNode);

    for (ezUInt32 i = 0; i < pins.GetCount(); ++i)
    {
      sDefineName.Format("INPUT_PIN_{0}_CONNECTED", i);

      if (pins[i]->GetConnections().IsEmpty())
      {
        sInlineCode.ReplaceAll(sDefineName, "false");
      }
      else
      {
        sInlineCode.ReplaceAll(sDefineName, "true");
      }
    }
  }

  {
    const ezArrayPtr<ezPin* const> pins = m_pNodeManager->GetOutputPins(pOwnerNode);

    for (ezUInt32 i = 0; i < pins.GetCount(); ++i)
    {
      sDefineName.Format("OUTPUT_PIN_{0}_CONNECTED", i);

      if (pins[i]->GetConnections().IsEmpty())
      {
        sInlineCode.ReplaceAll(sDefineName, "false");
      }
      else
      {
        sInlineCode.ReplaceAll(sDefineName, "true");
      }
    }
  }
}

void ezVisualShaderCodeGenerator::AppendStringIfUnique(ezStringBuilder& inout_String, const char* szAppend)
{
  if (inout_String.FindSubString(szAppend) != nullptr)
    return;

  inout_String.Append(szAppend);
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
        return ezStatus(
            ezFmt("A '{0}' node has a '{1}' property that is not a valid identifier: '{2}'. Only letters, digits and _ are allowed.",
                  pDesc->m_sName, props[p].m_sName, sPropValue));
      }

      auto& set = m_UsedUniqueValues[iUniqueValueGroup];

      if (set.Contains(sPropValue))
      {
        return ezStatus(ezFmt("A '{0}' node has a '{1}' property that has the same value ('{2}') as another parameter.", pDesc->m_sName,
                              props[p].m_sName, sPropValue));
      }

      set.Insert(sPropValue);
    }
  }

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezVisualShaderCodeGenerator::InsertPropertyValues(const ezDocumentObject* pNode, const ezVisualShaderNodeDescriptor* pDesc,
                                                           ezStringBuilder& sString)
{
  const auto& TypeAccess = pNode->GetTypeAccessor();

  ezStringBuilder sPropName, sPropValue;

  const auto& props = pDesc->m_Properties;
  for (ezUInt32 p0 = props.GetCount(); p0 > 0; --p0)
  {
    const ezUInt32 p = p0 - 1;

    sPropName.Format("$prop{0}", p);

    const ezVariant value = TypeAccess.GetValue(props[p].m_sName);
    sPropValue = ToShaderString(value);

    sString.ReplaceAll(sPropName, sPropValue);
  }

  return ezStatus(EZ_SUCCESS);
}
