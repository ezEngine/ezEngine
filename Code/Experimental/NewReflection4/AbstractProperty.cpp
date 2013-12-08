#include "AbstractProperty.h"

ezAbstractProperty::ezAbstractProperty(const char* szPropertyName)
{
  m_szPropertyName = szPropertyName;
}

ezAbstractMemberProperty::ezAbstractMemberProperty(const char* szPropertyName) : ezAbstractProperty(szPropertyName)
{
}

ezAbstractFunctionProperty::ezAbstractFunctionProperty(const char* szPropertyName) : ezAbstractProperty(szPropertyName)
{
}

ezAbstractArrayProperty::ezAbstractArrayProperty(const char* szPropertyName) : ezAbstractProperty(szPropertyName)
{
}

