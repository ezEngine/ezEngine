#pragma once

#include "AbstractProperty.h"
#include "StaticRTTI.h"

template<typename Type>
class ezTypedMemberProperty : public ezAbstractMemberProperty
{
public:
  ezTypedMemberProperty(const char* szPropertyName) : ezAbstractMemberProperty(szPropertyName)
  {
  }

  virtual const ezRTTI* GetPropertyType() const EZ_OVERRIDE
  {
    return ezGetStaticRTTI<Type>();
  }

  virtual Type GetValue(void* pInstance) const = 0;
  virtual void SetValue(void* pInstance, Type value) = 0;
};


template<typename Class, typename Type>
class ezAccessorProperty : public ezTypedMemberProperty<Type>
{
public:
  typedef Type (Class::*GetterFunc)() const;
  typedef void (Class::*SetterFunc)(Type value);

  ezAccessorProperty(const char* szPropertyName, GetterFunc getter, SetterFunc setter) : ezTypedMemberProperty<Type>(szPropertyName)
  {
    EZ_ASSERT(getter != NULL, "The getter of a property cannot be NULL.");

    m_Getter = getter;
    m_Setter = setter;
  }

  virtual void* GetPropertyPointer() const EZ_OVERRIDE
  {
    return NULL;
  }

  virtual bool IsReadOnly() const EZ_OVERRIDE
  {
    return m_Setter == NULL;
  }

  virtual Type GetValue(void* pInstance) const EZ_OVERRIDE
  {
    return (((Class*) pInstance)->*m_Getter)();
  }

  virtual void SetValue(void* pInstance, Type value) EZ_OVERRIDE
  {
    EZ_ASSERT(m_Setter != NULL, "The property '%s' has no setter function, thus it is read-only.", GetPropertyName());

    if (m_Setter)
      (((Class*) pInstance)->*m_Setter)(value);
  }

private:
  GetterFunc m_Getter;
  SetterFunc m_Setter;
};