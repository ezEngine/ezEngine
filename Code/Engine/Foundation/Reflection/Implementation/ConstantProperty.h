#pragma once

/// \file

#include <Foundation/Reflection/Implementation/StaticRTTI.h>
#include <Foundation/Reflection/Implementation/AbstractProperty.h>

/// \brief The base class for all typed member properties. Ie. once the type of a property is determined, it can be cast to the proper version of this.
///
/// For example, when you have a pointer to an ezAbstractMemberProperty and it returns that the property is of type 'int', you can cast the pointer
/// to an pointer to ezTypedMemberProperty<int> which then allows you to access its values.
template<typename Type>
class ezTypedConstantProperty : public ezAbstractConstantProperty
{
public:

  /// \brief Passes the property name through to ezAbstractMemberProperty.
  ezTypedConstantProperty(const char* szPropertyName) : ezAbstractConstantProperty(szPropertyName)
  {
  }

  /// \brief Returns the actual type of the property. You can then compare that with known types, eg. compare it to ezGetStaticRTTI<int>() to see whether this is an int property.
  virtual const ezRTTI* GetSpecificType() const override // [tested]
  {
    return ezGetStaticRTTI<typename ezTypeTraits<Type>::NonConstReferenceType>();
  }

  /// \brief Returns the value of the property. Pass the instance pointer to the surrounding class along.
  virtual Type GetValue() const = 0;
};

/// \brief [internal] An implementation of ezTypedConstantProperty that accesses the property data directly.
template <typename Type>
class ezConstantProperty : public ezTypedConstantProperty<Type>
{
public:
  /// \brief Constructor.
  ezConstantProperty(const char* szPropertyName, Type value)
    : ezTypedConstantProperty<Type>(szPropertyName)
    , m_Value(value)
  {
  }

  /// \brief Returns a pointer to the member property.
  virtual void* GetPropertyPointer() const override
  {
    return (void*)&m_Value;
  }

  /// \brief Returns the value of the property. Pass the instance pointer to the surrounding class along.
  virtual Type GetValue() const override // [tested]
  {
    return m_Value;
  }

  virtual ezVariant GetConstant() const override
  {
    return m_Value;
  }

private:
  Type m_Value;
};

