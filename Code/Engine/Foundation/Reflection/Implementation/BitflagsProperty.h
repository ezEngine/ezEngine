#pragma once

/// \file

#include <Foundation/Reflection/Implementation/EnumProperty.h>
#include <Foundation/Reflection/Implementation/StaticRTTI.h>

/// \brief [internal] An implementation of ezTypedEnumProperty that uses custom getter / setter functions to access a bitflags property.
template<typename Class, typename EnumType, typename Type>
class ezBitflagsAccessorProperty : public ezTypedEnumProperty< typename EnumType >
{
public:
  typedef typename ezTypeTraits<Type>::NonConstReferenceType RealType;
  typedef Type (Class::*GetterFunc)() const;
  typedef void (Class::*SetterFunc)(Type value);

  /// \brief Constructor.
  ezBitflagsAccessorProperty(const char* szPropertyName, GetterFunc getter, SetterFunc setter) : ezTypedEnumProperty<EnumType>(szPropertyName)
  {
    EZ_ASSERT(getter != nullptr, "The getter of a property cannot be nullptr.");

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
    EnumType::StorageType enumTemp = (static_cast<const Class*>(pInstance)->*m_Getter)().GetValue();
    return (ezInt64)enumTemp;
  }

  virtual void SetValue(void* pInstance, ezInt64 value) override // [tested]
  {
    EZ_ASSERT(m_Setter != nullptr, "The property '%s' has no setter function, thus it is read-only.", ezAbstractProperty::GetPropertyName());
    if (m_Setter)
      (static_cast<Class*>(pInstance)->*m_Setter)((EnumType::Enum)value);
  }

private:
  GetterFunc m_Getter;
  SetterFunc m_Setter;
};


/// \brief [internal] An implementation of ezTypedEnumProperty that accesses the bitflags property data directly.
template <typename Class, typename EnumType, typename Type>
class ezBitflagsMemberProperty : public ezTypedEnumProperty< typename EnumType >
{
public:
  typedef Type  (*GetterFunc)(const Class* pInstance);
  typedef void  (*SetterFunc)(      Class* pInstance, Type value);
  typedef void* (*PointerFunc)(const Class* pInstance);

  /// \brief Constructor.
  ezBitflagsMemberProperty(const char* szPropertyName, GetterFunc getter, SetterFunc setter, PointerFunc pointer) : ezTypedEnumProperty<EnumType>(szPropertyName)
  {
    EZ_ASSERT(getter != nullptr, "The getter of a property cannot be nullptr.");

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
    EnumType::StorageType enumTemp = m_Getter(static_cast<const Class*>(pInstance)).GetValue();
    return (ezInt64)enumTemp;
  }

  virtual void SetValue(void* pInstance, ezInt64 value) override // [tested]
  {
    EZ_ASSERT(m_Setter != nullptr, "The property '%s' has no setter function, thus it is read-only.", ezAbstractProperty::GetPropertyName());

    if (m_Setter)
      m_Setter(static_cast<Class*>(pInstance), (EnumType::Enum)value);
  }

private:
  GetterFunc m_Getter;
  SetterFunc m_Setter;
  PointerFunc m_Pointer;
};