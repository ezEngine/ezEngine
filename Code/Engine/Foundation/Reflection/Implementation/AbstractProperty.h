#pragma once

/// \file

#include <Foundation/Basics.h>

class ezRTTI;

/// \brief This is the base interface for all properties in the reflection system. It provides enough information to cast to the next better base class.
class EZ_FOUNDATION_DLL ezAbstractProperty
{
public:

  /// \brief Describes what category a property belongs to.
  enum PropertyCategory
  {
    Constant, ///< The property is a constant value that is stored inside the RTTI data.
    Member,   ///< The property is a 'member property', i.e. it represents some accessible value. Cast to ezAbstractMemberProperty.
    Function, ///< The property is a function which can be called. Cast to ezAbstractFunctionProperty.
    Array     ///< The property is actually an array of values. The array dimensions might be changeable. Cast to ezAbstractArrayProperty.
  };

  /// \brief The constructor must get the name of the property. The string must be a compile-time constant.
  ezAbstractProperty(const char* szPropertyName)
  {
    m_szPropertyName = szPropertyName;
  }

  /// \brief Returns the name of the property.
  const char* GetPropertyName() const { return m_szPropertyName; }

  /// \brief Returns the category of this property. Cast this property to the next higher type for more information.
  virtual PropertyCategory GetCategory() const = 0; // [tested]

private:
  const char* m_szPropertyName;
};

/// \brief This is the base class for all constant properties that are stored inside the RTTI data.
class EZ_FOUNDATION_DLL ezAbstractConstantProperty : public ezAbstractProperty
{
public:
  
  /// \brief Passes the property name through to ezAbstractProperty.
  ezAbstractConstantProperty(const char* szPropertyName) : ezAbstractProperty(szPropertyName)
  {
  }

  /// \brief Returns ezAbstractProperty::Constant.
  virtual ezAbstractProperty::PropertyCategory GetCategory() const override { return ezAbstractProperty::Constant; } // [tested]
  
  /// \brief Returns the type information of the constant property. Use this to cast this property to a specific version of ezTypedConstantProperty.
  virtual const ezRTTI* GetPropertyType() const = 0;
  
  /// \brief Returns a pointer to the constant data or nullptr. See ezAbstractMemberProperty::GetPropertyPointer for more information.
  virtual void* GetPropertyPointer() const = 0;
};

/// \brief This is the base class for all properties that are members of a class. It provides more information about the actual type.
class EZ_FOUNDATION_DLL ezAbstractMemberProperty : public ezAbstractProperty
{
public:
  
  /// \brief Passes the property name through to ezAbstractProperty.
  ezAbstractMemberProperty(const char* szPropertyName) : ezAbstractProperty(szPropertyName)
  {
  }

  /// \brief Returns ezAbstractProperty::Member.
  virtual ezAbstractProperty::PropertyCategory GetCategory() const override { return ezAbstractProperty::Member; }

  /// \brief Returns the type information of the member property. Use this to cast this property to a specific version of ezTypedMemberProperty.
  virtual const ezRTTI* GetPropertyType() const = 0; // [tested]

  /// \brief Returns a pointer to the property data or nullptr. If a valid pointer is returned, that pointer and the information from GetPropertyType() can
  /// be used to step deeper into the type (if required).
  ///
  /// You need to pass the pointer to an object on which you are operating. This function is mostly of interest when the property itself is a compound
  /// type (a struct or class). If it is a simple type (int, float, etc.) it doesn't make much sense to retrieve the pointer.
  ///
  /// For example GetPropertyType() might return that a property is of type ezVec3. In that case one might either stop and just use the code to handle
  /// ezVec3 types, or one might continue and enumerate all sub-properties (x, y and z) as well.
  ///
  /// \note There is no guarantee that this function returns a non-nullptr pointer, independent of the type. When a property uses custom 'accessors' 
  /// (functions to get / set the property value), it is not possible (or useful) to get the property pointer.
  virtual void* GetPropertyPointer(const void* pInstance) const = 0;

  /// \brief When this returns true, the property can be read, but not modified.
  virtual bool IsReadOnly() const = 0; // [tested]
};

/// \brief The base class for all function properties.
class EZ_FOUNDATION_DLL ezAbstractFunctionProperty : public ezAbstractProperty
{
public:

  /// \brief Passes the property name through to ezAbstractProperty.
  ezAbstractFunctionProperty(const char* szPropertyName) : ezAbstractProperty(szPropertyName)
  {
  }

  /// \brief Returns ezAbstractProperty::Function.
  virtual ezAbstractProperty::PropertyCategory GetCategory() const override { return ezAbstractProperty::Function; }

  /// \brief Calls the function. Provide the instance on which the function is supposed to be called.
  virtual void Execute(void* pInstance) const = 0;
};

/// \brief The base class for a property that represents an array of values.
///
/// \note Not implemented.
class EZ_FOUNDATION_DLL ezAbstractArrayProperty : public ezAbstractProperty
{
public:
  /// \brief Passes the property name through to ezAbstractProperty.
  ezAbstractArrayProperty(const char* szPropertyName) : ezAbstractProperty(szPropertyName)
  {
  }

  /// \brief Returns ezAbstractProperty::Array.
  virtual ezAbstractProperty::PropertyCategory GetCategory() const override { return ezAbstractProperty::Array; }

  /// \brief Returns the type of the elements in the array.
  virtual const ezRTTI* GetElementType() const = 0;
};



