#include <ToolsFoundationPCH.h>

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



ezPhantomFunctionProperty::ezPhantomFunctionProperty(ezReflectedFunctionDescriptor* pDesc)
    : ezAbstractFunctionProperty(nullptr)
{
  m_sPropertyNameStorage = pDesc->m_sName;
  m_szPropertyName = m_sPropertyNameStorage.GetData();
  m_FunctionType = pDesc->m_Type;
  m_Flags = pDesc->m_Flags;
  m_Flags.Add(ezPropertyFlags::Phantom);
  m_Attributes = pDesc->m_Attributes;
  pDesc->m_Attributes.Clear();

  m_ReturnValue = pDesc->m_ReturnValue;
  m_Arguments.Swap(pDesc->m_Arguments);
}



ezPhantomFunctionProperty::~ezPhantomFunctionProperty()
{
  for (auto pAttr : m_Attributes)
    pAttr->GetDynamicRTTI()->GetAllocator()->Deallocate(pAttr);
}

ezFunctionType::Enum ezPhantomFunctionProperty::GetFunctionType() const
{
  return m_FunctionType;
}

const ezRTTI* ezPhantomFunctionProperty::GetReturnType() const
{
  return ezRTTI::FindTypeByName(m_ReturnValue.m_sType);
}

ezBitflags<ezPropertyFlags> ezPhantomFunctionProperty::GetReturnFlags() const
{
  return m_ReturnValue.m_Flags;
}

ezUInt32 ezPhantomFunctionProperty::GetArgumentCount() const
{
  return m_Arguments.GetCount();
}

const ezRTTI* ezPhantomFunctionProperty::GetArgumentType(ezUInt32 uiParamIndex) const
{
  return ezRTTI::FindTypeByName(m_Arguments[uiParamIndex].m_sType);
}

ezBitflags<ezPropertyFlags> ezPhantomFunctionProperty::GetArgumentFlags(ezUInt32 uiParamIndex) const
{
  return m_Arguments[uiParamIndex].m_Flags;
}

void ezPhantomFunctionProperty::Execute(void* pInstance, ezArrayPtr<ezVariant> values, ezVariant& returnValue) const
{
  EZ_ASSERT_NOT_IMPLEMENTED;
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

ezPhantomMapProperty::ezPhantomMapProperty(const ezReflectedPropertyDescriptor* pDesc)
    : ezAbstractMapProperty(nullptr)
{
  m_sPropertyNameStorage = pDesc->m_sName;
  m_szPropertyName = m_sPropertyNameStorage.GetData();
  m_pPropertyType = ezRTTI::FindTypeByName(pDesc->m_sType);

  m_Flags = pDesc->m_Flags;
  m_Flags.Add(ezPropertyFlags::Phantom);
  m_Attributes = pDesc->m_Attributes;
  pDesc->m_Attributes.Clear();
}

ezPhantomMapProperty::~ezPhantomMapProperty()
{
  for (auto pAttr : m_Attributes)
    pAttr->GetDynamicRTTI()->GetAllocator()->Deallocate(pAttr);
}

const ezRTTI* ezPhantomMapProperty::GetSpecificType() const
{
  return m_pPropertyType;
}
