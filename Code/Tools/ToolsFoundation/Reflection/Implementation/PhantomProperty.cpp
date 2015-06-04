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
}

const ezRTTI* ezPhantomConstantProperty::GetPropertyType() const
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

}

const ezRTTI* ezPhantomMemberProperty::GetPropertyType() const
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
}



ezPhantomArrayProperty::ezPhantomArrayProperty(const ezReflectedPropertyDescriptor* pDesc)
  : ezAbstractArrayProperty(nullptr)
{
  m_sPropertyNameStorage = pDesc->m_sName;
  m_szPropertyName = m_sPropertyNameStorage.GetData();
  m_pPropertyType = ezRTTI::FindTypeByName(pDesc->m_sType);

  m_Flags = pDesc->m_Flags;
  m_Flags.Add(ezPropertyFlags::Phantom);
}

const ezRTTI* ezPhantomArrayProperty::GetElementType() const
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
}

const ezRTTI* ezPhantomSetProperty::GetElementType() const
{
  return m_pPropertyType;
}


