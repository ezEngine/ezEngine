#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Reflection/PhantomProperty.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

ezPhantomConstantProperty::ezPhantomConstantProperty(const ezReflectedPropertyDescriptor* pDesc)
  : ezAbstractConstantProperty(nullptr)
{
  m_sPropertyNameStorage = pDesc->m_sName;
  m_szPropertyName = m_sPropertyNameStorage.GetData();
  m_Value = pDesc->m_ConstantValue;
  m_pPropertyType = ezRTTI::FindTypeByName(pDesc->m_sType);

  m_Flags = pDesc->m_Flags;
  m_Flags.Add(ezPropertyFlags::Phantom);
  m_Attributes = pDesc->m_Attributes;
  pDesc->m_Attributes.Clear();
}

ezPhantomConstantProperty::~ezPhantomConstantProperty()
{
  for (auto pAttr : m_Attributes)
    pAttr->GetDynamicRTTI()->GetAllocator()->Deallocate(pAttr);
}

const ezRTTI* ezPhantomConstantProperty::GetSpecificType() const
{
  return m_pPropertyType;
}

void* ezPhantomConstantProperty::GetPropertyPointer() const
{
  return nullptr;
}



ezPhantomMemberProperty::ezPhantomMemberProperty(const ezReflectedPropertyDescriptor* pDesc)
  : ezAbstractMemberProperty(nullptr)
{
  m_sPropertyNameStorage = pDesc->m_sName;
  m_szPropertyName = m_sPropertyNameStorage.GetData();
  m_pPropertyType = ezRTTI::FindTypeByName(pDesc->m_sType);

  m_Flags = pDesc->m_Flags;
  m_Flags.Add(ezPropertyFlags::Phantom);
  m_Attributes = pDesc->m_Attributes;
  pDesc->m_Attributes.Clear();
}

ezPhantomMemberProperty::~ezPhantomMemberProperty()
{
  for (auto pAttr : m_Attributes)
    pAttr->GetDynamicRTTI()->GetAllocator()->Deallocate(pAttr);
}

const ezRTTI* ezPhantomMemberProperty::GetSpecificType() const
{
  return m_pPropertyType;
}




ezPhantomFunctionProperty::ezPhantomFunctionProperty(const ezReflectedPropertyDescriptor* pDesc)
  : ezAbstractFunctionProperty(nullptr)
{
  m_sPropertyNameStorage = pDesc->m_sName;
  m_szPropertyName = m_sPropertyNameStorage.GetData();

  m_Flags = pDesc->m_Flags;
  m_Flags.Add(ezPropertyFlags::Phantom);
  m_Attributes = pDesc->m_Attributes;
  pDesc->m_Attributes.Clear();
}



ezPhantomFunctionProperty::~ezPhantomFunctionProperty()
{
  for (auto pAttr : m_Attributes)
    pAttr->GetDynamicRTTI()->GetAllocator()->Deallocate(pAttr);
}

ezPhantomArrayProperty::ezPhantomArrayProperty(const ezReflectedPropertyDescriptor* pDesc)
  : ezAbstractArrayProperty(nullptr)
{
  m_sPropertyNameStorage = pDesc->m_sName;
  m_szPropertyName = m_sPropertyNameStorage.GetData();
  m_pPropertyType = ezRTTI::FindTypeByName(pDesc->m_sType);

  m_Flags = pDesc->m_Flags;
  m_Flags.Add(ezPropertyFlags::Phantom);
  m_Attributes = pDesc->m_Attributes;
  pDesc->m_Attributes.Clear();
}

ezPhantomArrayProperty::~ezPhantomArrayProperty()
{
  for (auto pAttr : m_Attributes)
    pAttr->GetDynamicRTTI()->GetAllocator()->Deallocate(pAttr);
}

const ezRTTI* ezPhantomArrayProperty::GetSpecificType() const
{
  return m_pPropertyType;
}

ezPhantomSetProperty::ezPhantomSetProperty(const ezReflectedPropertyDescriptor* pDesc)
  : ezAbstractSetProperty(nullptr)
{
  m_sPropertyNameStorage = pDesc->m_sName;
  m_szPropertyName = m_sPropertyNameStorage.GetData();
  m_pPropertyType = ezRTTI::FindTypeByName(pDesc->m_sType);

  m_Flags = pDesc->m_Flags;
  m_Flags.Add(ezPropertyFlags::Phantom);
  m_Attributes = pDesc->m_Attributes;
  pDesc->m_Attributes.Clear();
}

ezPhantomSetProperty::~ezPhantomSetProperty()
{
  for (auto pAttr : m_Attributes)
    pAttr->GetDynamicRTTI()->GetAllocator()->Deallocate(pAttr);
}

const ezRTTI* ezPhantomSetProperty::GetSpecificType() const
{
  return m_pPropertyType;
}


