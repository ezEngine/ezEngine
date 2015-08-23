#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Types/Enum.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Containers/Set.h>

class ezRTTI;
class ezPropertyAttribute;


/// \brief Flags used to describe a property and its type.
struct ezPropertyFlags
{
  typedef ezUInt16 StorageType;

  enum Enum
  {
    StandardType = EZ_BIT(0), ///< Anything that can be stored inside an ezVariant except for pointers and containers.
    ReadOnly = EZ_BIT(1),     ///< Can only be read but not modified.
    Pointer = EZ_BIT(2),      ///< Is a pointer to a type.
    PointerOwner = EZ_BIT(3), ///< This pointer property takes ownership of the passed pointer.
    IsEnum = EZ_BIT(4),       ///< enum property, cast to ezAbstractEnumerationProperty.
    Bitflags = EZ_BIT(5),     ///< bitflags property, cast to ezAbstractEnumerationProperty.
    Constant = EZ_BIT(6),     ///< Property is a constant.
    Phantom = EZ_BIT(7),
    Hidden = EZ_BIT(8),
    Default = 0
  };

  struct Bits
  {
    StorageType StandardType : 1;
    StorageType ReadOnly : 1;
    StorageType Pointer : 1;
    StorageType PointerOwner : 1;
    StorageType IsEnum : 1;
    StorageType Bitflags : 1;
    StorageType Constant : 1;
    StorageType Phantom : 1;
    StorageType Hidden : 1;
  };
};

EZ_DECLARE_FLAGS_OPERATORS(ezPropertyFlags)

/// \brief Describes what category a property belongs to.
struct ezPropertyCategory
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    Constant, ///< The property is a constant value that is stored inside the RTTI data.
    Member,   ///< The property is a 'member property', i.e. it represents some accessible value. Cast to ezAbstractMemberProperty.
    Function, ///< The property is a function which can be called. Cast to ezAbstractFunctionProperty.
    Array,    ///< The property is actually an array of values. The array dimensions might be changeable. Cast to ezAbstractArrayProperty.
    Set,      ///< The property is actually a set of values. Cast to ezAbstractArrayProperty.
    Map,
    Default = Member
  };
};

/// \brief This is the base interface for all properties in the reflection system. It provides enough information to cast to the next better base class.
class EZ_FOUNDATION_DLL ezAbstractProperty
{
public:
  /// \brief The constructor must get the name of the property. The string must be a compile-time constant.
  ezAbstractProperty(const char* szPropertyName)
  {
    m_szPropertyName = szPropertyName;
  }

  virtual ~ezAbstractProperty(){}

  /// \brief Returns the name of the property.
  const char* GetPropertyName() const { return m_szPropertyName; }

  /// \brief Returns the type information of the constant property. Use this to cast this property to a specific version of ezTypedConstantProperty.
  virtual const ezRTTI* GetSpecificType() const = 0;

  /// \brief Returns the category of this property. Cast this property to the next higher type for more information.
  virtual ezPropertyCategory::Enum GetCategory() const = 0; // [tested]

  /// \brief Returns the flags of the property.
  const ezBitflags<ezPropertyFlags>& GetFlags() const
  {
    return m_Flags;
  };

  /// \brief Adds flags to the property. Returns itself to allow to be called during initialization.
  ezAbstractProperty* AddFlags(ezBitflags<ezPropertyFlags> flags)
  {
    m_Flags.Add(flags);
    return this;
  };

  /// \brief Adds attributes to the property. Returns itself to allow to be called during initialization. Allocate an attribute using standard 'new'.
  ezAbstractProperty* AddAttributes(ezPropertyAttribute* pAttrib1, ezPropertyAttribute* pAttrib2 = nullptr, ezPropertyAttribute* pAttrib3 = nullptr, ezPropertyAttribute* pAttrib4 = nullptr, ezPropertyAttribute* pAttrib5 = nullptr, ezPropertyAttribute* pAttrib6 = nullptr)
  {
    EZ_ASSERT_DEV(pAttrib1 != nullptr, "invalid attribute");

    m_Attributes.PushBack(pAttrib1);
    if (pAttrib2) m_Attributes.PushBack(pAttrib2);
    if (pAttrib3) m_Attributes.PushBack(pAttrib3);
    if (pAttrib4) m_Attributes.PushBack(pAttrib4);
    if (pAttrib5) m_Attributes.PushBack(pAttrib5);
    if (pAttrib6) m_Attributes.PushBack(pAttrib6);
    return this;
  };

  /// \brief Returns the array of property attributes.
  const ezArrayPtr<ezPropertyAttribute* const> GetAttributes() const
  {
    return m_Attributes.GetArrayPtr();
  }

  /// \brief Returns the first attribute that derives from the given type, or nullptr if nothing is found.
  template<typename Type>
  const Type* GetAttributeByType() const
  {
    for (const auto* pAttr : m_Attributes)
    {
      if (pAttr->GetDynamicRTTI()->IsDerivedFrom<Type>())
        return static_cast<const Type*>(pAttr);
    }
    return nullptr;
  }

protected:
  ezBitflags<ezPropertyFlags> m_Flags;
  const char* m_szPropertyName;
  ezHybridArray<ezPropertyAttribute*, 2> m_Attributes;
};

/// \brief This is the base class for all constant properties that are stored inside the RTTI data.
class EZ_FOUNDATION_DLL ezAbstractConstantProperty : public ezAbstractProperty
{
public:

  /// \brief Passes the property name through to ezAbstractProperty.
  ezAbstractConstantProperty(const char* szPropertyName) : ezAbstractProperty(szPropertyName)
  {
  }

  /// \brief Returns ezPropertyCategory::Constant.
  virtual ezPropertyCategory::Enum GetCategory() const override { return ezPropertyCategory::Constant; } // [tested]

  /// \brief Returns a pointer to the constant data or nullptr. See ezAbstractMemberProperty::GetPropertyPointer for more information.
  virtual void* GetPropertyPointer() const = 0;

  /// \brief Returns the constant value as an ezVariant
  virtual ezVariant GetConstant() const = 0;

};

/// \brief This is the base class for all properties that are members of a class. It provides more information about the actual type.
///
/// If ezPropertyFlags::Pointer is set as a flag, you must not cast this property to ezTypedMemberProperty, instead use GetValuePtr and SetValuePtr.
/// This is because reference and constness of the property are only fixed for the pointer but not the type, so the actual property type cannot be derived.
class EZ_FOUNDATION_DLL ezAbstractMemberProperty : public ezAbstractProperty
{
public:

  /// \brief Passes the property name through to ezAbstractProperty.
  ezAbstractMemberProperty(const char* szPropertyName) : ezAbstractProperty(szPropertyName)
  {
  }

  /// \brief Returns ezPropertyCategory::Member.
  virtual ezPropertyCategory::Enum GetCategory() const override { return ezPropertyCategory::Member; }

  /// \brief Returns a pointer to the property data or nullptr. If a valid pointer is returned, that pointer and the information from GetSpecificType() can
  /// be used to step deeper into the type (if required).
  ///
  /// You need to pass the pointer to an object on which you are operating. This function is mostly of interest when the property itself is a compound
  /// type (a struct or class). If it is a simple type (int, float, etc.) it doesn't make much sense to retrieve the pointer.
  ///
  /// For example GetSpecificType() might return that a property is of type ezVec3. In that case one might either stop and just use the code to handle
  /// ezVec3 types, or one might continue and enumerate all sub-properties (x, y and z) as well.
  ///
  /// \note There is no guarantee that this function returns a non-nullptr pointer, independent of the type. When a property uses custom 'accessors' 
  /// (functions to get / set the property value), it is not possible (or useful) to get the property pointer.
  virtual void* GetPropertyPointer(const void* pInstance) const = 0;

  /// \brief Writes the value of this property in pInstance to pObject.
  /// pObject needs to point to an instance of this property's type.
  virtual void GetValuePtr(const void* pInstance, void* pObject) const = 0;

  /// \brief Sets the value of pObject to the property in pInstance.
  /// pObject needs to point to an instance of this property's type.
  virtual void SetValuePtr(void* pInstance, void* pObject) = 0;

};

/// \brief The base class for all function properties.
class EZ_FOUNDATION_DLL ezAbstractFunctionProperty : public ezAbstractProperty
{
public:

  /// \brief Passes the property name through to ezAbstractProperty.
  ezAbstractFunctionProperty(const char* szPropertyName) : ezAbstractProperty(szPropertyName)
  {
  }

  /// \brief Returns ezPropertyCategory::Function.
  virtual ezPropertyCategory::Enum GetCategory() const override { return ezPropertyCategory::Function; }

  /// \brief Calls the function. Provide the instance on which the function is supposed to be called.
  virtual void Execute(void* pInstance) const = 0;
};


/// \brief The base class for a property that represents an array of values.
class EZ_FOUNDATION_DLL ezAbstractArrayProperty : public ezAbstractProperty
{
public:
  /// \brief Passes the property name through to ezAbstractProperty.
  ezAbstractArrayProperty(const char* szPropertyName) : ezAbstractProperty(szPropertyName)
  {
  }

  /// \brief Returns ezPropertyCategory::Array.
  virtual ezPropertyCategory::Enum GetCategory() const override { return ezPropertyCategory::Array; }

  /// \brief Returns number of elements.
  virtual ezUInt32 GetCount(const void* pInstance) const = 0;

  /// \brief Writes element at index uiIndex to the target of pObject.
  virtual void GetValue(const void* pInstance, ezUInt32 uiIndex, void* pObject) const = 0;

  /// \brief Writes the target of pObject to the element at index uiIndex.
  virtual void SetValue(void* pInstance, ezUInt32 uiIndex, const void* pObject) = 0;

  /// \brief Inserts the target of pObject into the array at index uiIndex.
  virtual void Insert(void* pInstance, ezUInt32 uiIndex, void* pObject) = 0;

  /// \brief Removes the element in the array at index uiIndex.
  virtual void Remove(void* pInstance, ezUInt32 uiIndex) = 0;

  /// \brief Clears the array.
  virtual void Clear(void* pInstance) = 0;

  /// \brief Resizes the array to uiCount.
  virtual void SetCount(void* pInstance, ezUInt32 uiCount) = 0;

};


/// \brief The base class for a property that represents a set of values.
///
/// The element type must either be a standard type or a pointer.
class EZ_FOUNDATION_DLL ezAbstractSetProperty : public ezAbstractProperty
{
public:
  /// \brief Passes the property name through to ezAbstractProperty.
  ezAbstractSetProperty(const char* szPropertyName) : ezAbstractProperty(szPropertyName)
  {

  }

  /// \brief Returns ezPropertyCategory::Array.
  virtual ezPropertyCategory::Enum GetCategory() const override { return ezPropertyCategory::Set; }

  /// \brief Returns number of elements.
  virtual ezUInt32 GetCount(const void* pInstance) const = 0;

  /// \brief Clears the set.
  virtual void Clear(void* pInstance) = 0;

  /// \brief Inserts the target of pObject into the set.
  virtual void Insert(void* pInstance, void* pObject) = 0;

  /// \brief Removes the target of pObject from the set.
  virtual void Remove(void* pInstance, void* pObject) = 0;

  /// \brief Returns whether the target of pObject is in the set. 
  virtual bool Contains(const void* pInstance, void* pObject) const = 0;

  /// \brief Writes the content of the set to out_keys.
  virtual void GetValues(const void* pInstance, ezHybridArray<ezVariant, 16>& out_keys) const = 0;

};


/// \brief Template that allows to probe a single parameter function for parameter and return type.
template <typename FUNC>
struct ezMemberFunctionParameterTypeResolver
{
};

template <class Class, typename R, typename P>
struct ezMemberFunctionParameterTypeResolver<R (Class::*)(P) >
{
  typedef P ParameterType;
  typedef R ReturnType;
};

/// \brief Template that allows to probe a container for its element type.
template <typename CONTAINER>
struct ezContainerSubTypeResolver
{
};


template <typename T>
struct ezContainerSubTypeResolver<ezArrayPtr<T> >
{
  typedef typename ezTypeTraits<T>::NonConstReferenceType Type;
};

template <typename T>
struct ezContainerSubTypeResolver<ezDynamicArray<T> >
{
  typedef typename ezTypeTraits<T>::NonConstReferenceType Type;
};

template <typename T, ezUInt32 Size>
struct ezContainerSubTypeResolver<ezHybridArray<T, Size> >
{
  typedef typename ezTypeTraits<T>::NonConstReferenceType Type;
};

template <typename T>
struct ezContainerSubTypeResolver<ezDeque<T> >
{
  typedef typename ezTypeTraits<T>::NonConstReferenceType Type;
};

template <typename T>
struct ezContainerSubTypeResolver<ezSet<T> >
{
  typedef typename ezTypeTraits<T>::NonConstReferenceType Type;
};

