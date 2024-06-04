#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/RenderPipelineNode.h>

// static_assert(sizeof(ezRenderPipelineNodePin) == 4);

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderPipelineNode, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezRenderPipelineNodePin, ezNoBase, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_ATTRIBUTES
  {
   new ezHiddenAttribute(),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezRenderPipelineNodeInputPin, ezRenderPipelineNodePin, 1, ezRTTINoAllocator)
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezRenderPipelineNodeOutputPin, ezRenderPipelineNodePin, 1, ezRTTINoAllocator)
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezRenderPipelineNodePassThrougPin, ezRenderPipelineNodePin, 1, ezRTTINoAllocator)
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

void ezRenderPipelineNode::InitializePins()
{
  m_InputPins.Clear();
  m_OutputPins.Clear();
  m_NameToPin.Clear();

  const ezRTTI* pType = GetDynamicRTTI();

  ezHybridArray<const ezAbstractProperty*, 32> properties;
  pType->GetAllProperties(properties);

  for (auto pProp : properties)
  {
    if (pProp->GetCategory() != ezPropertyCategory::Member || !pProp->GetSpecificType()->IsDerivedFrom(ezGetStaticRTTI<ezRenderPipelineNodePin>()))
      continue;

    auto pPinProp = static_cast<const ezAbstractMemberProperty*>(pProp);
    ezRenderPipelineNodePin* pPin = static_cast<ezRenderPipelineNodePin*>(pPinProp->GetPropertyPointer(this));

    pPin->m_pParent = this;
    if (pPin->m_Type == ezRenderPipelineNodePin::Type::Unknown)
    {
      EZ_REPORT_FAILURE("Pin '{0}' has an invalid type. Do not use ezRenderPipelineNodePin directly as member but one of its derived types", pProp->GetPropertyName());
      continue;
    }

    if (pPin->m_Type == ezRenderPipelineNodePin::Type::Input || pPin->m_Type == ezRenderPipelineNodePin::Type::PassThrough)
    {
      pPin->m_uiInputIndex = static_cast<ezUInt8>(m_InputPins.GetCount());
      m_InputPins.PushBack(pPin);
    }
    if (pPin->m_Type == ezRenderPipelineNodePin::Type::Output || pPin->m_Type == ezRenderPipelineNodePin::Type::PassThrough)
    {
      pPin->m_uiOutputIndex = static_cast<ezUInt8>(m_OutputPins.GetCount());
      m_OutputPins.PushBack(pPin);
    }

    ezHashedString sHashedName;
    sHashedName.Assign(pProp->GetPropertyName());
    m_NameToPin.Insert(sHashedName, pPin);
  }
}

ezHashedString ezRenderPipelineNode::GetPinName(const ezRenderPipelineNodePin* pPin) const
{
  for (auto it = m_NameToPin.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value() == pPin)
    {
      return it.Key();
    }
  }
  return ezHashedString();
}

const ezRenderPipelineNodePin* ezRenderPipelineNode::GetPinByName(const char* szName) const
{
  ezHashedString sHashedName;
  sHashedName.Assign(szName);
  return GetPinByName(sHashedName);
}

const ezRenderPipelineNodePin* ezRenderPipelineNode::GetPinByName(ezHashedString sName) const
{
  const ezRenderPipelineNodePin* pin;
  if (m_NameToPin.TryGetValue(sName, pin))
  {
    return pin;
  }

  return nullptr;
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderPipelineNode);
