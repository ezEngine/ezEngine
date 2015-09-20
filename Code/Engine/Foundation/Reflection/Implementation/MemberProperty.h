#pragma once

/// \file

#include <Foundation/Reflection/Implementation/AbstractProperty.h>
#include <Foundation/Reflection/Implementation/StaticRTTI.h>
#include <Foundation/Types/Variant.h>

// ***********************************************
// ***** Base class for accessing properties *****


/// \brief The base class for all typed member properties. I.e. once the type of a property is determined, it can be cast to the proper version of this.
///
/// For example, when you have a pointer to an ezAbstractMemberProperty and it returns that the property is of type 'int', you can cast the pointer
/// to an pointer to ezTypedMemberProperty<int> which then allows you to access its values.
template<typename Type>
class ezTypedMemberProperty : public ezAbstractMemberProperty
{
public:

  /// \brief Passes the property name through to ezAbstractMemberProperty.
  ezTypedMemberProperty(const char* szPropertyName) : ezAbstractMemberProperty(szPropertyName)
  {
    ezVariant::Type::Enum type = static_cast<ezVariant::Type::Enum>(ezVariant::TypeDeduction<typename ezTypeTraits<Type>::NonConstReferenceType>::value);
    if ((type >= ezVariant::Type::Bool && type <= ezVariant::Type::Uuid) || EZ_IS_SAME_TYPE(ezVariant, Type))
      m_Flags.Add(ezPropertyFlags::StandardType);

    if (type == ezVariant::Type::VoidPointer || type == ezVariant::Type::ReflectedPointer)
      m_Flags.Add(ezPropertyFlags::Pointer);
  }

  /// \brief Returns the actual type of the property. You can then compare that with known types, eg. compare it to ezGetStaticRTTI<int>() to see whether this is an int property.
  virtual const ezRTTI* GetSpecificType() const override // [tested]
  {
    return ezGetStaticRTTI<typename ezTypeTraits<Type>::NonConstReferencePointerType>();
  }

  /// \brief Returns the value of the property. Pass the instance pointer to the surrounding class along.
  virtual Type GetValue(const void* pInstance) const = 0; // [tested]

  /// \brief Modifies the value of the property. Pass the instance pointer to the surrounding class along.
  ///
  /// \note Make sure the property is not read-only before calling this, otherwise an assert will fire.
  virtual void SetValue(void* pInstance, Type value) = 0; // [tested]

  virtual void GetValuePtr(const void* pInstance, void* pObject) const override { *static_cast<Type*>(pObject) = GetValue(pInstance); };
  virtual void SetValuePtr(void* pInstance, void* pObject) override { SetValue(pInstance, *static_cast<Type*>(pObject)); };
};

/// \brief Specialization of ezTypedMemberProperty for const char*.
///
/// This works because ezTypedMemberProperty< typename ezTypeTraits<Type>::NonConstReferenceType > in ezAccessorProperty
/// does not actually remove the constness of the type but of the pointer, so const char* is not affected.
template <>
class ezTypedMemberProperty<const char*> : public ezAbstractMemberProperty
{
public:
  ezTypedMemberProperty(const char* szPropertyName) : ezAbstractMemberProperty(szPropertyName)
  {
    // We treat const char* as a basic type and not a pointer.
    m_Flags.Add(ezPropertyFlags::StandardType);
  }

  virtual const ezRTTI* GetSpecificType() const override // [tested]
  {
    return ezGetStaticRTTI<const char*>();
  }

  virtual const char* GetValue(const void* pInstance) const = 0;
  virtual void SetValue(void* pInstance, const char* value) = 0;
  virtual void GetValuePtr(const void* pInstance, void* pObject) const override { *static_cast<const char**>(pObject) = GetValue(pInstance); };
  virtual void SetValuePtr(void* pInstance, void* pObject) override { SetValue(pInstance, *static_cast<const char**>(pObject)); };
};


// *******************************************************************
// ***** Class for properties that use custom accessor functions *****

/// \brief [internal] An implementation of ezTypedMemberProperty that uses custom getter / setter functions to access a property.
template<typename Class, typename Type>
class ezAccessorProperty : public ezTypedMemberProperty< typename ezTypeTraits<Type>::NonConstReferenceType >
{
public:
  typedef typename ezTypeTraits<Type>::NonConstReferenceType RealType;
  typedef Type (Class::*GetterFunc)() const;
  typedef void (Class::*SetterFunc)(Type value);

  /// \brief Constructor.
  ezAccessorProperty(const char* szPropertyName, GetterFunc getter, SetterFunc setter) : ezTypedMemberProperty<RealType>(szPropertyName)
  {
    EZ_ASSERT_DEBUG(getter != nullptr, "The getter of a property cannot be nullptr.");

    m_Getter = getter;
    m_Setter = setter;

    if (m_Setter == nullptr)
      ezAbstractMemberProperty::m_Flags.Add(ezPropertyFlags::ReadOnly);
  }

  /// \brief Always returns nullptr; once a property is modified through accessors, there is no point in giving more direct access to others.
  virtual void* GetPropertyPointer(const void* pInstance) const override
  {
    // No access to sub-properties, if we have accessors for this property
    return nullptr;
  }

  /// \brief Returns the value of the property. Pass the instance pointer to the surrounding class along.
  virtual RealType GetValue(const void* pInstance) const override // [tested]
  {
    return (static_cast<const Class*>(pInstance)->*m_Getter)();
  }

  /// \brief Modifies the value of the property. Pass the instance pointer to the surrounding class along.
  ///
  /// \note Make sure the property is not read-only before calling this, otherwise an assert will fire.
  virtual void SetValue(void* pInstance, RealType value) override // [tested]
  {
    EZ_ASSERT_DEV(m_Setter != nullptr, "The property '%s' has no setter function, thus it is read-only.", ezAbstractProperty::GetPropertyName());

    if (m_Setter)
      (static_cast<Class*>(pInstance)->*m_Setter)(value);
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

  static void* GetPropertyPointer(const Class* pInstance)
  {
    return (void*) &((*pInstance).*Member);
  }
};


/// \brief [internal] An implementation of ezTypedMemberProperty that accesses the property data directly.
template <typename Class, typename Type>
class ezMemberProperty : public ezTypedMemberProperty<Type>
{
public:
  typedef Type  (*GetterFunc)(const Class* pInstance);
  typedef void  (*SetterFunc)(      Class* pInstance, Type value);
  typedef void* (*PointerFunc)(const Class* pInstance);

  /// \brief Constructor.
  ezMemberProperty(const char* szPropertyName, GetterFunc getter, SetterFunc setter, PointerFunc pointer) : ezTypedMemberProperty<Type>(szPropertyName)
  {
    EZ_ASSERT_DEBUG(getter != nullptr, "The getter of a property cannot be nullptr.");

    m_Getter = getter;
    m_Setter = setter;
    m_Pointer = pointer;

    if (m_Setter == nullptr)
      ezAbstractMemberProperty::m_Flags.Add(ezPropertyFlags::ReadOnly);
  }

  /// \brief Returns a pointer to the member property.
  virtual void* GetPropertyPointer(const void* pInstance) const override
  {
    return m_Pointer(static_cast<const Class*>(pInstance));
  }

  /// \brief Returns the value of the property. Pass the instance pointer to the surrounding class along.
  virtual Type GetValue(const void* pInstance) const override
  {
    return m_Getter(static_cast<const Class*>(pInstance));
  }

  /// \brief Modifies the value of the property. Pass the instance pointer to the surrounding class along.
  ///
  /// \note Make sure the property is not read-only before calling this, otherwise an assert will fire.
  virtual void SetValue(void* pInstance, Type value) override
  {
    EZ_ASSERT_DEV(m_Setter != nullptr, "The property '%s' has no setter function, thus it is read-only.", ezAbstractProperty::GetPropertyName());

    if (m_Setter)
      m_Setter(static_cast<Class*>(pInstance), value);
  }

private:
  GetterFunc m_Getter;
  SetterFunc m_Setter;
  PointerFunc m_Pointer;
};

