#include <Foundation/PCH.h>
#include <Foundation/Utilities/Node.h>

//EZ_CHECK_AT_COMPILETIME(sizeof(ezNodePin) == 4);

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezNode, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezNodePin, ezNoBase, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_ATTRIBUTES
  {
   new ezHiddenAttribute(),
  }
  EZ_END_ATTRIBUTES
}
EZ_END_STATIC_REFLECTED_TYPE

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezInputNodePin, ezNodePin, 1, ezRTTINoAllocator)
EZ_END_STATIC_REFLECTED_TYPE

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezOutputNodePin, ezNodePin, 1, ezRTTINoAllocator)
EZ_END_STATIC_REFLECTED_TYPE

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezPassThroughNodePin, ezNodePin, 1, ezRTTINoAllocator)
EZ_END_STATIC_REFLECTED_TYPE

void ezNode::InitializePins()
{
  m_InputPins.Clear();
  m_OutputPins.Clear();
  m_NameToPin.Clear();

  const ezRTTI* pType = GetDynamicRTTI();

  ezHybridArray<ezAbstractProperty*, 32> properties;
  pType->GetAllProperties(properties);

  for (auto pProp : properties)
  {
    if (pProp->GetCategory() != ezPropertyCategory::Member || !pProp->GetSpecificType()->IsDerivedFrom(ezGetStaticRTTI<ezNodePin>()))
      continue;

    auto pPinProp = static_cast<ezAbstractMemberProperty*>(pProp);
    ezNodePin* pPin = static_cast<ezNodePin*>(pPinProp->GetPropertyPointer(this));

    pPin->m_pParent = this;
    if (pPin->m_Type == ezNodePin::Type::Unknown)
    {
      EZ_REPORT_FAILURE("Pin '{0}' has an invalid type. Do not use ezNodePin directly as member but one of its derived types", pProp->GetPropertyName());
      continue;
    }

    if (pPin->m_Type == ezNodePin::Type::Input || pPin->m_Type == ezNodePin::Type::PassThrough)
    {
      pPin->m_uiInputIndex = m_InputPins.GetCount();
      m_InputPins.PushBack(pPin);
    }
    if (pPin->m_Type == ezNodePin::Type::Output || pPin->m_Type == ezNodePin::Type::PassThrough)
    {
      pPin->m_uiOutputIndex = m_OutputPins.GetCount();
      m_OutputPins.PushBack(pPin);
    }

    ezHashedString sHashedName; sHashedName.Assign(pProp->GetPropertyName());
    m_NameToPin.Insert(sHashedName, pPin);
  }
}

ezHashedString ezNode::GetPinName(const ezNodePin* pPin) const
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

const ezNodePin* ezNode::GetPinByName(const char* szName) const
{
  ezHashedString sHashedName; sHashedName.Assign(szName);
  return GetPinByName(sHashedName);
}

const ezNodePin* ezNode::GetPinByName(ezHashedString sName) const
{
  const ezNodePin* pin;
  if (m_NameToPin.TryGetValue(sName, pin))
  {
    return pin;
  }

  return nullptr;
}




EZ_STATICLINK_FILE(Foundation, Foundation_Utilities_Implementation_Node);

