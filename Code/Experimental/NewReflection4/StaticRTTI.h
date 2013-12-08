#pragma once

#include "RTTI.h"
#include "Helpers.h"

/// \brief Dummy type to pass to EZ_BEGIN_REFLECTED_TYPE for all types that have no base class.
class ezNoBase { };

// ****************************************************
// ***** Templates for accessing static RTTI data *****

/// \brief [internal] Helper struct for accessing static RTTI data.
template<typename T>
struct ezStaticRTTI
{
  static ezRTTI* GetRTTI();
};

// Special implementation for types that have no base
template<>
struct ezStaticRTTI<ezNoBase>
{
  static ezRTTI* GetRTTI()
  {
    return NULL;
  }
};


/// \brief Use this function, specialized with the type that you are interested in, to get the static RTTI data for some type.
template<typename T>
const ezRTTI* ezGetStaticRTTI()
{
  // Since this is pure C++ and no preprocessor macro, calling it with types such as 'int' and 'ezInt32' will 
  // actually return the same RTTI object, which would not be possible with a purely macro based solution

  return ezStaticRTTI<T>::GetRTTI();
}

// **************************************************
// ***** Macros for declaring types reflectable *****

#define EZ_NO_LINKAGE

/// \brief Declares a type to be reflectable. Insert this into the header of a type to enable reflection on it.
#define EZ_DECLARE_REFLECTABLE_TYPE(Linkage, TYPE)                    \
  /* The function that stores the RTTI object.*/                      \
  Linkage ezRTTI* ezReflectableTypeRTTI_##TYPE();                     \
                                                                      \
  /* This specialization calls the function to get the RTTI data */   \
  /* This code might get duplicated in different DLLs, but all   */   \
  /* will call the same function, so the RTTI object is unique   */   \
  template<>                                                          \
  struct ezStaticRTTI<TYPE>                                           \
  {                                                                   \
    static ezRTTI* GetRTTI()                                          \
    {                                                                 \
      static ezRTTI* s_pRTTI = ezReflectableTypeRTTI_##TYPE();        \
      return s_pRTTI;                                                 \
    }                                                                 \
  };                                                                  \


/// \brief Implements the necessary functionality for a type to be generally reflectable.
///
/// \param Type
///   The type for which the reflection functionality should be implemented.
/// \param Base
///   The base class type of \a Type. If it has no base class, pass ezNoBase
/// \param Allocator
///   The type of an ezRTTIAllocator that can be used to create and destroy instances
///   of \a Type. Pass ezRTTINoAllocator for types that should not be created dynamically.
///   Pass ezRTTIDefaultAllocator<Type> for types that should be created on the default heap.
///   Pass a custom ezRTTIAllocator type to handle allocation differently.
#define EZ_BEGIN_REFLECTED_TYPE(Type, BaseType, AllocatorType)                \
  class ezRTTInfo_##Type                                                      \
  {                                                                           \
  public:                                                                     \
    static const char* GetTypeName() { return #Type; }                        \
                                                                              \
    typedef Type OwnType;                                                     \
    typedef BaseType OwnBaseType;                                             \
  };                                                                          \
                                                                              \
  ezRTTI* ezReflectableTypeRTTI_##Type()                                      \
  {                                                                           \
    static AllocatorType Allocator;                                           \
    static ezArrayPtr<ezAbstractProperty*> Properties                         \


#define EZ_END_REFLECTED_TYPE(Type)                                           \
    static ezRTTI rtti(ezRTTInfo_##Type::GetTypeName(),                       \
      ezGetStaticRTTI<ezRTTInfo_##Type::OwnBaseType>(),                       \
      &Allocator, Properties);                                                \
                                                                              \
    return &rtti;                                                             \
  }                                                                           \
  static void Register_##Type() { ezReflectableTypeRTTI_##Type(); }           \
  static ezExecuteAtStartup s_AutoRegister_##Type (Register_##Type)           \


#define EZ_BEGIN_PROPERTIES                                                   \
    static ezAbstractProperty* PropertyList[] =                               \
    {                                                                         \


#define EZ_END_PROPERTIES                                                     \
    };                                                                        \
  Properties = PropertyList;                                                  \


#define EZ_MEMBER_PROPERTY(PropertyName)                                      \
  new ezAbstractMemberProperty(#PropertyName)





