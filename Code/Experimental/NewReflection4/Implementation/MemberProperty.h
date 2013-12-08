#pragma once

/// \file

#include "AbstractProperty.h"
#include "StaticRTTI.h"

// ***********************************************
// ***** Base class for accessing properties *****


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

  virtual Type GetValue(const void* pInstance) const = 0;
  virtual void SetValue(void* pInstance, Type value) = 0;
};


// *******************************************************************
// ***** Class for properties that use custom accessor functions *****

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

  virtual Type GetValue(const void* pInstance) const EZ_OVERRIDE
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


// *************************************************************
// ***** Classes for properties that are accessed directly *****

/// \brief [internal] Helper class to generate accessor functions for (private) members of another class
template <typename Class, typename Type, Type Class::* Member>
struct ezPropertyAccessor
{
  static Type GetValue(const Class* pInstance)
  {
    return (*pInstance).*Member;
  }

  static void SetValue(Class* pInstance, Type value)
  {
    (*pInstance).*Member = value;
  }
};


template <typename Class, typename Type>
class ezMemberProperty : public ezTypedMemberProperty<Type>
{
public:
  typedef Type (*GetterFunc)(const Class* pInstance);
  typedef void (*SetterFunc)(      Class* pInstance, Type value);

  ezMemberProperty(const char* szPropertyName, GetterFunc getter, SetterFunc setter) : ezTypedMemberProperty<Type>(szPropertyName)
  {
    EZ_ASSERT(getter != NULL, "The getter of a property cannot be NULL.");

    m_Getter = getter;
    m_Setter = setter;
  }

  virtual void* GetPropertyPointer() const EZ_OVERRIDE
  {
    /// \todo bla
    return NULL;
  }

  virtual bool IsReadOnly() const EZ_OVERRIDE
  {
    return m_Setter == NULL;
  }

  virtual Type GetValue(const void* pInstance) const EZ_OVERRIDE
  {
    return m_Getter(static_cast<const Class*>(pInstance));
  }

  virtual void SetValue(void* pInstance, Type value) EZ_OVERRIDE
  {
    EZ_ASSERT(m_Setter != NULL, "The property '%s' has no setter function, thus it is read-only.", GetPropertyName());

    if (m_Setter)
      m_Setter(static_cast<Class*>(pInstance), value);
  }

private:
  GetterFunc m_Getter;
  SetterFunc m_Setter;
};