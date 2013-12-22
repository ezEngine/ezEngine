#pragma once

/// \file

#include <Foundation/Basics.h>

class ezRTTI;

/// \brief Dummy type to pass to EZ_BEGIN_REFLECTED_TYPE for all types that have no base class.
class EZ_FOUNDATION_DLL ezNoBase { };

/// \brief [internal] Simple helper class to execute code at startup.
class EZ_FOUNDATION_DLL ezRTTIExecuteAtStartup
{
public:
  typedef void (*Function)();

  ezRTTIExecuteAtStartup(Function f)
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
/// \param BaseType
///   The base class type of \a Type. If it has no base class, pass ezNoBase
/// \param AllocatorType
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

/// \brief Ends the reflection code block that was opened with EZ_BEGIN_REFLECTED_TYPE.
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
  static ezRTTIExecuteAtStartup s_AutoRegister_##Type (Register_##Type);      \
                                                                              \
  ezRTTI* ezReflectableTypeRTTI_##Type()                                      \
  {                                                                           \
    return ezRTTInfo_##Type::GetReflectableTypeRTTI();                        \
  }                                                                           \


/// \brief Within a EZ_BEGIN_REFLECTED_TYPE / EZ_END_REFLECTED_TYPE block, use this to start the block that declares all the properties.
#define EZ_BEGIN_PROPERTIES                                                   \
    static ezAbstractProperty* PropertyList[] =                               \
    {                                                                         \


/// \brief Ends the block to declare properties that was started with EZ_BEGIN_PROPERTIES.
#define EZ_END_PROPERTIES                                                     \
    };                                                                        \
  Properties = PropertyList;                                                  \

/// \brief Within a EZ_BEGIN_PROPERTIES / EZ_END_PROPERTIES block, this adds a function property.
/// 
/// \param PropertyName
///   The unique (in this class) name under which the function property should be registered.
/// \param FunctionName
///   The actual C++ name of the function that should be exposed as a property.
///
/// \note Currently only functions that take no parameter and return void can be added as function properties.
#define EZ_FUNCTION_PROPERTY(PropertyName, FunctionName)                      \
  new ezFunctionProperty<OwnType>(PropertyName, OwnType::FunctionName)        \

// [internal] Helper macro to get the return type of a getter function.
#define EZ_GETTER_TYPE(Class, GetterFunc)                                     \
  decltype(((Class*) NULL)->GetterFunc())

/// \brief Within a EZ_BEGIN_PROPERTIES / EZ_END_PROPERTIES block, this adds a property that uses custom getter / setter functions.
///
/// \param PropertyName
///   The unique (in this class) name under which the property should be registered.
/// \param Getter
///   The getter function for this property.
/// \param Setter
///   The setter function for this property.
///
/// \note There does not actually need to be a variable for this type of properties, as all accesses go through functions.
/// Thus you can for example expose a 'vector' property that is actually stored as a column of a matrix.
#define EZ_ACCESSOR_PROPERTY(PropertyName, Getter, Setter)                    \
  new ezAccessorProperty<OwnType, EZ_GETTER_TYPE(OwnType, OwnType::Getter)>   \
    (PropertyName, OwnType::Getter, OwnType::Setter)                          \

/// \brief Same as EZ_ACCESSOR_PROPERTY, but no setter is provided, thus making the property read-only.
#define EZ_ACCESSOR_PROPERTY_READ_ONLY(PropertyName, Getter)                  \
  new ezAccessorProperty<OwnType, EZ_GETTER_TYPE(OwnType, OwnType::Getter)>   \
    (PropertyName, OwnType::Getter, NULL)                                     \

// [internal] Helper macro to get the type of a class member.
#define EZ_MEMBER_TYPE(Class, Member)                                         \
  decltype(((Class*) NULL)->Member)

/// \brief Within a EZ_BEGIN_PROPERTIES / EZ_END_PROPERTIES block, this adds a property that actually exists as a member.
///
/// \param PropertyName
///   The unique (in this class) name under which the property should be registered.
/// \param MemberName
///   The name of the member variable that should get exposed as a property.
///
/// \note Since the member is exposed directly, there is no way to know when the variable was modified. That also means
/// no custom limits to the values can be applied. If that becomes necessary, just add getter / setter functions and
/// expose the property as a EZ_ACCESSOR_PROPERTY instead.
#define EZ_MEMBER_PROPERTY(PropertyName, MemberName)                          \
  new ezMemberProperty<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName)>          \
    (PropertyName,                                                            \
    &ezPropertyAccessor<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetValue,            \
    &ezPropertyAccessor<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::SetValue,            \
    &ezPropertyAccessor<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetPropertyPointer)  \

/// \brief Same as EZ_MEMBER_PROPERTY, but the property is read-only.
#define EZ_MEMBER_PROPERTY_READ_ONLY(PropertyName, MemberName)                \
  new ezMemberProperty<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName)>          \
    (PropertyName,                                                            \
    &ezPropertyAccessor<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetValue,            \
    NULL,                                                                     \
    &ezPropertyAccessor<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetPropertyPointer)  \

/// \brief Within an EZ_BEGIN_REFLECTED_TYPE / EZ_END_REFLECTED_TYPE block, use this to start the block that declares all the message handlers.
#define EZ_BEGIN_MESSAGEHANDLERS                                              \
    static ezAbstractMessageHandler* HandlerList[] =                          \
    {                                                                         \

/// \brief Ends the block to declare message handlers that was started with EZ_BEGIN_MESSAGEHANDLERS.
#define EZ_END_MESSAGEHANDLERS                                                \
    };                                                                        \
  MessageHandlers = HandlerList;                                              \


/// \brief Within an EZ_BEGIN_MESSAGEHANDLERS / EZ_END_MESSAGEHANDLERS block, this adds another message handler.
/// 
/// \param MessageType
///   The type of message that this handler function accepts. You may add 'const' in front of it.
/// \param FunctionName
///   The actual C++ name of the message handler function.
///
/// \note A message handler is a function that takes one parameter of type ezMessage (or a derived type) and returns void.
#define EZ_MESSAGE_HANDLER(MessageType, FunctionName)                         \
  new ezMessageHandler<OwnType, MessageType>(OwnType::FunctionName)           \

