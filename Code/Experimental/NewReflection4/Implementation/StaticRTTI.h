#pragma once

/// \file

#include "RTTI.h"

/// \brief Dummy type to pass to EZ_BEGIN_REFLECTED_TYPE for all types that have no base class.
class ezNoBase { };

/// \brief Simple helper class to execute code at startup.
class ezExecuteAtStartup
{
public:
  typedef void (*Function)();

  ezExecuteAtStartup(Function f)
  {
    f();
  }
};

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

/// \brief Insert this into a class/struct to enable properties that are private members.
/// All types that have dynamic reflection (\see EZ_ADD_DYNAMIC_REFLECTION) already have this ability.
#define EZ_ALLOW_PRIVATE_PROPERTIES(SELF)                             \
  friend class ezRTTInfo_##SELF                                       \

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
                                                                              \
    static ezRTTI* GetReflectableTypeRTTI()                                   \
    {                                                                         \
      static AllocatorType Allocator;                                         \
      static ezArrayPtr<ezAbstractProperty*> Properties;                      \
      static ezArrayPtr<ezAbstractMessageHandler*> MessageHandlers            \

#define EZ_END_REFLECTED_TYPE(Type)                                           \
      static ezRTTI rtti(GetTypeName(),                                       \
        ezGetStaticRTTI<OwnBaseType>(),                                       \
        sizeof(OwnType),                                                      \
        &Allocator, Properties, MessageHandlers);                             \
                                                                              \
      return &rtti;                                                           \
    }                                                                         \
  };                                                                          \
  static void Register_##Type()                                               \
  {                                                                           \
    ezRTTInfo_##Type::GetReflectableTypeRTTI();                               \
  }                                                                           \
                                                                              \
  static ezExecuteAtStartup s_AutoRegister_##Type (Register_##Type);          \
                                                                              \
  ezRTTI* ezReflectableTypeRTTI_##Type()                                      \
  {                                                                           \
    return ezRTTInfo_##Type::GetReflectableTypeRTTI();                        \
  }                                                                           \


#define EZ_BEGIN_PROPERTIES                                                   \
    static ezAbstractProperty* PropertyList[] =                               \
    {                                                                         \


#define EZ_END_PROPERTIES                                                     \
    };                                                                        \
  Properties = PropertyList;                                                  \

#define EZ_FUNCTION_PROPERTY(PropertyName, FunctionName)                      \
  new ezFunctionProperty<OwnType>(PropertyName, OwnType::FunctionName)        \

#define EZ_GETTER_TYPE(Class, GetterFunc)                                     \
  decltype(((Class*) NULL)->GetterFunc())

#define EZ_ACCESSOR_PROPERTY(PropertyName, Getter, Setter)                    \
  new ezAccessorProperty<OwnType, EZ_GETTER_TYPE(OwnType, OwnType::Getter)>   \
    (PropertyName, OwnType::Getter, OwnType::Setter)                          \

#define EZ_MEMBER_TYPE(Class, Member)                                         \
  decltype(((Class*) NULL)->Member)

#define EZ_MEMBER_PROPERTY(PropertyName, MemberName)                          \
  new ezMemberProperty<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName)>          \
    (PropertyName,                                                            \
    &ezPropertyAccessor<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetValue,            \
    &ezPropertyAccessor<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::SetValue,            \
    &ezPropertyAccessor<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetPropertyPointer)  \



#define EZ_BEGIN_MESSAGEHANDLERS                                              \
    static ezAbstractMessageHandler* HandlerList[] =                          \
    {                                                                         \

#define EZ_END_MESSAGEHANDLERS                                                \
    };                                                                        \
  MessageHandlers = HandlerList;                                              \


#define EZ_MESSAGE_HANDLER(MessageType, FunctionName)                         \
  new ezMessageHandler<OwnType, MessageType>(OwnType::FunctionName)           \

