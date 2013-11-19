#pragma once

#include <Foundation/Basics.h>
#include "Property.h"

class ezReflectedBase;

template<class TYPE>
class ezTypedProperty : public ezAbstractProperty
{
public:
  ezTypedProperty(const char* szName) : ezAbstractProperty(szName)
  {
  }

  virtual ezPropertyType GetPropertyType() const
  {
    return ezPropertyTypeDeduction<TYPE>::s_TypeID;
  }

  virtual TYPE GetValue(const void* pReflected) const = 0;

  virtual void SetValue(void* pReflected, TYPE Value) = 0;

  virtual bool IsReadOnly() = 0;
};


template<class OWNER, class TYPE>
class ezCustomAccessorProperty : public ezTypedProperty<TYPE>
{
public:
  typedef TYPE (OWNER::*GetterFunc)() const;
  typedef void (OWNER::*SetterFunc)(TYPE value);

  ezCustomAccessorProperty(const char* szName, GetterFunc getter, SetterFunc setter) : ezTypedProperty(szName)
  {
    m_Getter = getter;
    m_Setter = setter;
  }

  virtual TYPE GetValue(const void* pReflected) const
  {
    return (((OWNER*) pReflected)->*m_Getter)();
  }

  virtual void SetValue(void* pReflected, TYPE Value)
  {
    if (m_Setter)
      (((OWNER*) pReflected)->*m_Setter)(Value);
  }

  virtual bool IsReadOnly()
  {
    return m_Setter == NULL;
  }

private:
  GetterFunc m_Getter;
  SetterFunc m_Setter;
};


template<class OWNER, class TYPE>
class ezGeneratedAccessorProperty : public ezTypedProperty<TYPE>
{
public:
  typedef TYPE (*GetterFunc)(const OWNER* pReflected);
  typedef void (*SetterFunc)(      OWNER* pReflected, TYPE value);

  ezGeneratedAccessorProperty(const char* szName, GetterFunc getter, SetterFunc setter) : ezTypedProperty(szName)
  {
    m_Getter = getter;
    m_Setter = setter;
  }

  virtual TYPE GetValue(const void* pReflected) const
  {
    return m_Getter((OWNER*) pReflected);
  }

  virtual void SetValue(void* pReflected, TYPE Value)
  {
    if (m_Setter)
      m_Setter((OWNER*) pReflected, Value);
  }

  virtual bool IsReadOnly()
  {
    return m_Setter == NULL;
  }

private:
  GetterFunc m_Getter;
  SetterFunc m_Setter;
};