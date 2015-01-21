#pragma once

/// \file

#include <Foundation/Reflection/Implementation/MemberProperty.h>
#include <Foundation/Reflection/Implementation/StaticRTTI.h>

/// \brief The base class for enum and bitflags member properties.
///
/// Cast any property whose type derives from ezEnumBase or ezBitflagsBase class to access its value.
class ezAbstractEnumerationProperty : public ezAbstractMemberProperty
{
public:

  /// \brief Passes the property name through to ezAbstractMemberProperty.
  ezAbstractEnumerationProperty(const char* szPropertyName) : ezAbstractMemberProperty(szPropertyName)
  {
  }

  /// \brief Returns the value of the property. Pass the instance pointer to the surrounding class along.
  virtual ezInt64 GetValue(const void* pInstance) const = 0;

  /// \brief Modifies the value of the property. Pass the instance pointer to the surrounding class along.
  ///
  /// \note Make sure the property is not read-only before calling this, otherwise an assert will fire.
  virtual void SetValue(void* pInstance, ezInt64 value) = 0;
};


/// \brief [internal] Base class for enum / bitflags properties that already defines the type.
template<typename EnumType>
class ezTypedEnumProperty : public ezAbstractEnumerationProperty
{
public:

  /// \brief Passes the property name through to ezAbstractEnumerationProperty.
  ezTypedEnumProperty(const char* szPropertyName) : ezAbstractEnumerationProperty(szPropertyName)
  {
  }

  /// \brief Returns the actual type of the property. You can then test whether it derives from ezEnumBase or 
  ///  ezBitflagsBase to determine whether we are dealing with an enum or bitflags property.
  virtual const ezRTTI* GetPropertyType() const override // [tested]
  {
    return ezGetStaticRTTI<typename ezTypeTraits<EnumType>::NonConstReferenceType>();
  }
};


/// \brief [internal] An implementation of ezTypedEnumProperty that uses custom getter / setter functions to access an enum property.
template<typename Class, typename EnumType, typename Type>
class ezEnumAccessorProperty : public ezTypedEnumProperty< EnumType >
{
public:
  typedef typename ezTypeTraits<Type>::NonConstReferenceType RealType;
  typedef Type (Class::*GetterFunc)() const;
  typedef void (Class::*SetterFunc)(Type value);

  /// \brief Constructor.
  ezEnumAccessorProperty(const char* szPropertyName, GetterFunc getter, SetterFunc setter) : ezTypedEnumProperty<EnumType>(szPropertyName)
  {
    EZ_ASSERT_DEBUG(getter != nullptr, "The getter of a property cannot be nullptr.");

    m_Getter = getter;
    m_Setter = setter;
  }

  virtual void* GetPropertyPointer(const void* pInstance) const override
  {
    // No access to sub-properties, if we have accessors for this property
    return nullptr;
  }

  virtual bool IsReadOnly() const override
  {
    return m_Setter == nullptr;
  }

  virtual ezInt64 GetValue(const void* pInstance) const override // [tested]
  {
    typename EnumType::Enum enumTemp = (static_cast<const Class*>(pInstance)->*m_Getter)();
    return (ezInt64)enumTemp;
  }

  virtual void SetValue(void* pInstance, ezInt64 value) override // [tested]
  {
    EZ_ASSERT_DEV(m_Setter != nullptr, "The property '%s' has no setter function, thus it is read-only.", ezAbstractProperty::GetPropertyName());
    if (m_Setter)
      (static_cast<Class*>(pInstance)->*m_Setter)((typename EnumType::Enum) value);
  }

private:
  GetterFunc m_Getter;
  SetterFunc m_Setter;
};


/// \brief [internal] An implementation of ezTypedEnumProperty that accesses the enum property data directly.
template <typename Class, typename EnumType, typename Type>
class ezEnumMemberProperty : public ezTypedEnumProperty< EnumType >
{
public:
  typedef Type  (*GetterFunc)(const Class* pInstance);
  typedef void  (*SetterFunc)(      Class* pInstance, Type value);
  typedef void* (*PointerFunc)(const Class* pInstance);

  /// \brief Constructor.
  ezEnumMemberProperty(const char* szPropertyName, GetterFunc getter, SetterFunc setter, PointerFunc pointer) : ezTypedEnumProperty<EnumType>(szPropertyName)
  {
    EZ_ASSERT_DEBUG(getter != nullptr, "The getter of a property cannot be nullptr.");

    m_Getter = getter;
    m_Setter = setter;
    m_Pointer = pointer;
  }

  virtual void* GetPropertyPointer(const void* pInstance) const override
  {
    return m_Pointer(static_cast<const Class*>(pInstance));
  }

  virtual bool IsReadOnly() const override
  {
    return m_Setter == nullptr;
  }

  virtual ezInt64 GetValue(const void* pInstance) const override // [tested]
  {
    typename EnumType::Enum enumTemp = m_Getter(static_cast<const Class*>(pInstance));
    return (ezInt64)enumTemp;
  }

  virtual void SetValue(void* pInstance, ezInt64 value) override // [tested]
  {
    EZ_ASSERT_DEV(m_Setter != nullptr, "The property '%s' has no setter function, thus it is read-only.", ezAbstractProperty::GetPropertyName());

    if (m_Setter)
      m_Setter(static_cast<Class*>(pInstance), (typename EnumType::Enum) value);
  }

private:
  GetterFunc m_Getter;
  SetterFunc m_Setter;
  PointerFunc m_Pointer;
};
