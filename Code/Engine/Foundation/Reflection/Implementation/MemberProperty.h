#pragma once

/// \file

#include <Foundation/Reflection/Implementation/AbstractProperty.h>
#include <Foundation/Reflection/Implementation/StaticRTTI.h>
#include <Foundation/Types/Variant.h>

// ***********************************************
// ***** Base class for accessing properties *****


/// \brief Type-safe base class for accessing member properties with known data types.
///
/// Once you determine a property's type through the reflection system, you can safely cast
/// the abstract property pointer to this typed version. This provides type-safe access to
/// property values without the overhead of variant conversions.
///
/// Example usage:
/// ```cpp
/// auto* abstractProp = rtti->FindPropertyByName("someProperty");
/// if (abstractProp->GetSpecificType() == ezGetStaticRTTI<int>())
/// {
///   auto* intProp = static_cast<ezTypedMemberProperty<int>*>(abstractProp);
///   int value = intProp->GetValue(instance);
/// }
/// ```
template <typename Type>
class ezTypedMemberProperty : public ezAbstractMemberProperty
{
public:
  /// \brief Passes the property name through to ezAbstractMemberProperty.
  ezTypedMemberProperty(const char* szPropertyName)
    : ezAbstractMemberProperty(szPropertyName)
  {
    m_Flags = ezPropertyFlags::GetParameterFlags<Type>();
    static_assert(
      !std::is_pointer<Type>::value ||
        ezVariant::TypeDeduction<typename ezTypeTraits<Type>::NonConstReferencePointerType>::value == ezVariantType::Invalid,
      "Pointer to standard types are not supported.");
  }

  /// \brief Returns the actual type of the property. You can then compare that with known types, eg. compare it to ezGetStaticRTTI<int>()
  /// to see whether this is an int property.
  virtual const ezRTTI* GetSpecificType() const override // [tested]
  {
    return ezGetStaticRTTI<typename ezTypeTraits<Type>::NonConstReferencePointerType>();
  }

  /// \brief Returns the value of the property. Pass the instance pointer to the surrounding class along.
  virtual Type GetValue(const void* pInstance) const = 0; // [tested]

  /// \brief Modifies the value of the property. Pass the instance pointer to the surrounding class along.
  ///
  /// \note Make sure the property is not read-only before calling this, otherwise an assert will fire.
  virtual void SetValue(void* pInstance, Type value) const = 0; // [tested]

  virtual void GetValuePtr(const void* pInstance, void* pObject) const override { *static_cast<Type*>(pObject) = GetValue(pInstance); };
  virtual void SetValuePtr(void* pInstance, const void* pObject) const override { SetValue(pInstance, *static_cast<const Type*>(pObject)); };
};

/// \brief Specialization of ezTypedMemberProperty for const char*.
///
/// This works because ezTypedMemberProperty< typename ezTypeTraits<Type>::NonConstReferenceType > in ezAccessorProperty
/// does not actually remove the constness of the type but of the pointer, so const char* is not affected.
template <>
class ezTypedMemberProperty<const char*> : public ezAbstractMemberProperty
{
public:
  ezTypedMemberProperty(const char* szPropertyName)
    : ezAbstractMemberProperty(szPropertyName)
  {
    // We treat const char* as a basic type and not a pointer.
    m_Flags = ezPropertyFlags::GetParameterFlags<const char*>();
  }

  virtual const ezRTTI* GetSpecificType() const override // [tested]
  {
    return ezGetStaticRTTI<const char*>();
  }

  virtual const char* GetValue(const void* pInstance) const = 0;
  virtual void SetValue(void* pInstance, const char* value) const = 0;
  virtual void GetValuePtr(const void* pInstance, void* pObject) const override { *static_cast<const char**>(pObject) = GetValue(pInstance); };
  virtual void SetValuePtr(void* pInstance, const void* pObject) const override { SetValue(pInstance, *static_cast<const char* const*>(pObject)); };
};


// *******************************************************************
// ***** Class for properties that use custom accessor functions *****

/// \brief Implementation of ezTypedMemberProperty that uses custom getter/setter functions to access a property.
///
/// This property type is used when you want to expose computed or transformed values as properties,
/// or when you need to perform validation, logging, or side effects during property access.
/// The actual data may be stored differently than how it's exposed through the property interface.
///
/// Use this when:
/// - Property value needs computation or transformation
/// - You need to validate or clamp values on set
/// - Property access should trigger side effects
/// - The internal storage format differs from the exposed type
template <typename Class, typename Type>
class ezAccessorProperty : public ezTypedMemberProperty<typename ezTypeTraits<Type>::NonConstReferenceType>
{
public:
  using RealType = typename ezTypeTraits<Type>::NonConstReferenceType;
  using GetterFunc = Type (Class::*)() const;
  using SetterFunc = void (Class::*)(Type value);

  /// \brief Constructor.
  ezAccessorProperty(const char* szPropertyName, GetterFunc getter, SetterFunc setter)
    : ezTypedMemberProperty<RealType>(szPropertyName)
  {
    EZ_ASSERT_DEBUG(getter != nullptr, "The getter of a property cannot be nullptr.");

    m_Getter = getter;
    m_Setter = setter;

    if (m_Setter == nullptr)
      ezAbstractMemberProperty::m_Flags.Add(ezPropertyFlags::ReadOnly);
  }

  /// \brief Always returns nullptr; once a property is modified through accessors, there is no point in giving more direct access to
  /// others.
  virtual void* GetPropertyPointer(const void* pInstance) const override
  {
    EZ_IGNORE_UNUSED(pInstance);

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
  virtual void SetValue(void* pInstance, RealType value) const override // [tested]
  {
    EZ_ASSERT_DEV(m_Setter != nullptr, "The property '{0}' has no setter function, thus it is read-only.", ezAbstractProperty::GetPropertyName());

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
template <typename Class, typename Type, Type Class::*Member>
struct ezPropertyAccessor
{
  static Type GetValue(const Class* pInstance) { return (*pInstance).*Member; }

  static void SetValue(Class* pInstance, Type value) { (*pInstance).*Member = value; }

  static void* GetPropertyPointer(const Class* pInstance) { return (void*)&((*pInstance).*Member); }
};


/// \brief Implementation of ezTypedMemberProperty that provides direct access to member variables.
///
/// This property type offers the most efficient access to object members by directly
/// reading from and writing to the memory location of a class member. It's the preferred
/// choice for simple data members that don't require special handling.
template <typename Class, typename Type>
class ezMemberProperty : public ezTypedMemberProperty<Type>
{
public:
  using GetterFunc = Type (*)(const Class* pInstance);
  using SetterFunc = void (*)(Class* pInstance, Type value);
  using PointerFunc = void* (*)(const Class* pInstance);

  /// \brief Constructor.
  ezMemberProperty(const char* szPropertyName, GetterFunc getter, SetterFunc setter, PointerFunc pointer)
    : ezTypedMemberProperty<Type>(szPropertyName)
  {
    EZ_ASSERT_DEBUG(getter != nullptr, "The getter of a property cannot be nullptr.");

    m_Getter = getter;
    m_Setter = setter;
    m_Pointer = pointer;

    if (m_Setter == nullptr)
      ezAbstractMemberProperty::m_Flags.Add(ezPropertyFlags::ReadOnly);
  }

  /// \brief Returns a pointer to the member property.
  virtual void* GetPropertyPointer(const void* pInstance) const override { return m_Pointer(static_cast<const Class*>(pInstance)); }

  /// \brief Returns the value of the property. Pass the instance pointer to the surrounding class along.
  virtual Type GetValue(const void* pInstance) const override { return m_Getter(static_cast<const Class*>(pInstance)); }

  /// \brief Modifies the value of the property. Pass the instance pointer to the surrounding class along.
  ///
  /// \note Make sure the property is not read-only before calling this, otherwise an assert will fire.
  virtual void SetValue(void* pInstance, Type value) const override
  {
    EZ_ASSERT_DEV(m_Setter != nullptr, "The property '{0}' has no setter function, thus it is read-only.", ezAbstractProperty::GetPropertyName());

    if (m_Setter)
      m_Setter(static_cast<Class*>(pInstance), value);
  }

private:
  GetterFunc m_Getter;
  SetterFunc m_Setter;
  PointerFunc m_Pointer;
};
