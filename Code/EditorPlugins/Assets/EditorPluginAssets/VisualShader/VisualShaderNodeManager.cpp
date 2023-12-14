#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/VisualShader/VisualShaderNodeManager.h>

//////////////////////////////////////////////////////////////////////////
// ezVisualShaderPin
//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualShaderPin, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezVisualShaderPin::ezVisualShaderPin(Type type, const ezVisualShaderPinDescriptor* pDescriptor, const ezDocumentObject* pObject)
  : ezPin(type, pDescriptor->m_sName, pDescriptor->m_Color, pObject)
{
  m_pDescriptor = pDescriptor;
}

const ezRTTI* ezVisualShaderPin::GetDataType() const
{
  return m_pDescriptor->m_pDataType;
}

const ezString& ezVisualShaderPin::GetTooltip() const
{
  return m_pDescriptor->m_sTooltip;
}

//////////////////////////////////////////////////////////////////////////
// ezVisualShaderNodeManager
//////////////////////////////////////////////////////////////////////////

bool ezVisualShaderNodeManager::InternalIsNode(const ezDocumentObject* pObject) const
{
  return pObject->GetType()->IsDerivedFrom(ezVisualShaderTypeRegistry::GetSingleton()->GetNodeBaseType());
}

void ezVisualShaderNodeManager::InternalCreatePins(const ezDocumentObject* pObject, NodeInternal& ref_node)
{
  const auto* pDesc = ezVisualShaderTypeRegistry::GetSingleton()->GetDescriptorForType(pObject->GetType());

  if (pDesc == nullptr)
    return;

  ref_node.m_Inputs.Reserve(pDesc->m_InputPins.GetCount());
  ref_node.m_Outputs.Reserve(pDesc->m_OutputPins.GetCount());

  for (const auto& pin : pDesc->m_InputPins)
  {
    auto pPin = EZ_DEFAULT_NEW(ezVisualShaderPin, ezPin::Type::Input, &pin, pObject);
    ref_node.m_Inputs.PushBack(pPin);
  }

  for (const auto& pin : pDesc->m_OutputPins)
  {
    auto pPin = EZ_DEFAULT_NEW(ezVisualShaderPin, ezPin::Type::Output, &pin, pObject);
    ref_node.m_Outputs.PushBack(pPin);
  }
}

void ezVisualShaderNodeManager::GetNodeCreationTemplates(ezDynamicArray<ezNodeCreationTemplate>& out_templates) const
{
  const ezRTTI* pNodeBaseType = ezVisualShaderTypeRegistry::GetSingleton()->GetNodeBaseType();

  ezRTTI::ForEachDerivedType(
    pNodeBaseType,
    [&](const ezRTTI* pRtti)
    {
      auto& nodeTemplate = out_templates.ExpandAndGetRef();
      nodeTemplate.m_pType = pRtti;

      if (const ezVisualShaderNodeDescriptor* pDesc = ezVisualShaderTypeRegistry::GetSingleton()->GetDescriptorForType(pRtti))
      {
        nodeTemplate.m_sCategory = pDesc->m_sCategory;
      }
    },
    ezRTTI::ForEachOptions::ExcludeAbstract);
}

ezStatus ezVisualShaderNodeManager::InternalCanConnect(const ezPin& source, const ezPin& target, CanConnectResult& out_result) const
{
  const ezVisualShaderPin& pinSource = ezStaticCast<const ezVisualShaderPin&>(source);
  const ezVisualShaderPin& pinTarget = ezStaticCast<const ezVisualShaderPin&>(target);

  const ezRTTI* pSamplerType = ezVisualShaderTypeRegistry::GetSingleton()->GetPinSamplerType();
  const ezRTTI* pStringType = ezGetStaticRTTI<ezString>();

  if ((pinSource.GetDataType() == pSamplerType && pinTarget.GetDataType() != pSamplerType) || (pinSource.GetDataType() != pSamplerType && pinTarget.GetDataType() == pSamplerType))
  {
    out_result = CanConnectResult::ConnectNever;
    return ezStatus("Pin of type 'sampler' cannot be connected with a pin of a different type.");
  }

  if ((pinSource.GetDataType() == pStringType && pinTarget.GetDataType() != pStringType) || (pinSource.GetDataType() != pStringType && pinTarget.GetDataType() == pStringType))
  {
    out_result = CanConnectResult::ConnectNever;
    return ezStatus("Pin of type 'string' cannot be connected with a pin of a different type.");
  }

  if (WouldConnectionCreateCircle(source, target))
  {
    out_result = CanConnectResult::ConnectNever;
    return ezStatus("Connecting these pins would create a circle in the shader graph.");
  }

  out_result = CanConnectResult::ConnectNto1;
  return ezStatus(EZ_SUCCESS);
}


ezStatus ezVisualShaderNodeManager::InternalCanAdd(const ezRTTI* pRtti, const ezDocumentObject* pParent, ezStringView sParentProperty, const ezVariant& index) const
{
  auto pDesc = ezVisualShaderTypeRegistry::GetSingleton()->GetDescriptorForType(pRtti);

  if (pDesc)
  {
    if (pDesc->m_NodeType == ezVisualShaderNodeType::Main && CountNodesOfType(ezVisualShaderNodeType::Main) > 0)
    {
      return ezStatus("The shader may only contain a single output node");
    }

    /// \todo This is an arbitrary limit and it does not count how many nodes reference the same texture
    static constexpr ezUInt32 uiMaxTextures = 16;
    if (pDesc->m_NodeType == ezVisualShaderNodeType::Texture && CountNodesOfType(ezVisualShaderNodeType::Texture) >= uiMaxTextures)
    {
      return ezStatus(ezFmt("The maximum number of texture nodes is {0}", uiMaxTextures));
    }
  }

  return ezStatus(EZ_SUCCESS);
}

ezUInt32 ezVisualShaderNodeManager::CountNodesOfType(ezVisualShaderNodeType::Enum type) const
{
  ezUInt32 count = 0;

  const ezVisualShaderTypeRegistry* pRegistry = ezVisualShaderTypeRegistry::GetSingleton();

  const auto& children = GetRootObject()->GetChildren();
  for (ezUInt32 i = 0; i < children.GetCount(); ++i)
  {
    auto pDesc = pRegistry->GetDescriptorForType(children[i]->GetType());

    if (pDesc && pDesc->m_NodeType == type)
    {
      ++count;
    }
  }

  return count;
}
