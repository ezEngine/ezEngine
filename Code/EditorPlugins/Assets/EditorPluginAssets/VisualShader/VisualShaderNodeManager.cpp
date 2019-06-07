#include <EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/VisualShader/VisualShaderNodeManager.h>
#include <EditorPluginAssets/VisualShader/VisualShaderTypeRegistry.h>
#include <Foundation/Utilities/Node.h>

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
// ezVisualShaderConnection
//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualShaderConnection, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////
// ezVisualShaderNodeManager
//////////////////////////////////////////////////////////////////////////

bool ezVisualShaderNodeManager::InternalIsNode(const ezDocumentObject* pObject) const
{
  return pObject->GetType()->IsDerivedFrom(ezVisualShaderTypeRegistry::GetSingleton()->GetNodeBaseType());
}

void ezVisualShaderNodeManager::InternalCreatePins(const ezDocumentObject* pObject, NodeInternal& node)
{
  const auto* pDesc = ezVisualShaderTypeRegistry::GetSingleton()->GetDescriptorForType(pObject->GetType());

  if (pDesc == nullptr)
    return;

  node.m_Inputs.Reserve(pDesc->m_InputPins.GetCount());
  node.m_Outputs.Reserve(pDesc->m_OutputPins.GetCount());

  for (const auto& pin : pDesc->m_InputPins)
  {
    ezVisualShaderPin* pPin = EZ_DEFAULT_NEW(ezVisualShaderPin, ezPin::Type::Input, &pin, pObject);
    node.m_Inputs.PushBack(pPin);
  }

  for (const auto& pin : pDesc->m_OutputPins)
  {
    ezVisualShaderPin* pPin = EZ_DEFAULT_NEW(ezVisualShaderPin, ezPin::Type::Output, &pin, pObject);
    node.m_Outputs.PushBack(pPin);
  }
}

void ezVisualShaderNodeManager::InternalDestroyPins(const ezDocumentObject* pObject, NodeInternal& node)
{
  for (ezPin* pPin : node.m_Inputs)
  {
    EZ_DEFAULT_DELETE(pPin);
  }
  node.m_Inputs.Clear();

  for (ezPin* pPin : node.m_Outputs)
  {
    EZ_DEFAULT_DELETE(pPin);
  }
  node.m_Outputs.Clear();
}


void ezVisualShaderNodeManager::GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& Types) const
{
  const ezRTTI* pNodeBaseType = ezVisualShaderTypeRegistry::GetSingleton()->GetNodeBaseType();

  for (auto it = ezRTTI::GetFirstInstance(); it != nullptr; it = it->GetNextInstance())
  {
    if (it->IsDerivedFrom(pNodeBaseType) && !it->GetTypeFlags().IsSet(ezTypeFlags::Abstract))
      Types.PushBack(it);
  }
}

ezStatus ezVisualShaderNodeManager::InternalCanConnect(const ezPin* pSource, const ezPin* pTarget, CanConnectResult& out_Result) const
{
  const ezVisualShaderPin* pPinSource = ezDynamicCast<const ezVisualShaderPin*>(pSource);
  const ezVisualShaderPin* pPinTarget = ezDynamicCast<const ezVisualShaderPin*>(pTarget);

  EZ_ASSERT_DEBUG(pPinSource != nullptr && pPinTarget != nullptr, "Das ist eigentlich unmoeglich!");

  const ezRTTI* pSamplerType = ezVisualShaderTypeRegistry::GetSingleton()->GetPinSamplerType();
  const ezRTTI* pStringType = ezGetStaticRTTI<ezString>();

  if ((pPinSource->GetDataType() == pSamplerType && pPinTarget->GetDataType() != pSamplerType) ||
      (pPinSource->GetDataType() != pSamplerType && pPinTarget->GetDataType() == pSamplerType))
  {
    out_Result = CanConnectResult::ConnectNever;
    return ezStatus("Pin of type 'sampler' cannot be connected with a pin of a different type.");
  }

  if ((pPinSource->GetDataType() == pStringType && pPinTarget->GetDataType() != pStringType) ||
      (pPinSource->GetDataType() != pStringType && pPinTarget->GetDataType() == pStringType))
  {
    out_Result = CanConnectResult::ConnectNever;
    return ezStatus("Pin of type 'string' cannot be connected with a pin of a different type.");
  }

  if (WouldConnectionCreateCircle(pSource, pTarget))
  {
    out_Result = CanConnectResult::ConnectNever;
    return ezStatus("Connecting these pins would create a circle in the shader graph.");
  }

  out_Result = CanConnectResult::ConnectNto1;
  return ezStatus(EZ_SUCCESS);
}

const char* ezVisualShaderNodeManager::GetTypeCategory(const ezRTTI* pRtti) const
{
  const ezVisualShaderNodeDescriptor* pDesc = ezVisualShaderTypeRegistry::GetSingleton()->GetDescriptorForType(pRtti);

  if (pDesc == nullptr)
    return nullptr;

  return pDesc->m_sCategory;
}


ezStatus ezVisualShaderNodeManager::InternalCanAdd(const ezRTTI* pRtti, const ezDocumentObject* pParent, const char* szParentProperty,
                                                   const ezVariant& index) const
{
  auto pDesc = ezVisualShaderTypeRegistry::GetSingleton()->GetDescriptorForType(pRtti);

  if (pDesc)
  {
    if (pDesc->m_NodeType == ezVisualShaderNodeType::Main && CountNodesOfType(ezVisualShaderNodeType::Main) > 0)
    {
      return ezStatus("The shader may only contain a single output node");
    }

    /// \todo This is an arbitrary limit and it does not count how many nodes reference the same texture
    static const ezUInt32 uiMaxTextures = 16;
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
