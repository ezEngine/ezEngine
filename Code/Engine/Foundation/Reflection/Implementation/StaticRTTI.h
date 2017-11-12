#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Types/Variant.h>
#include <type_traits>
#include <Foundation/Types/Bitflags.h>

class ezRTTI;
class ezReflectedClass;

/// \brief Flags that describe a reflected type.
struct ezTypeFlags
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    StandardType = EZ_BIT(0), ///< Anything that can be stored inside an ezVariant except for pointers and containers.
    IsEnum = EZ_BIT(1),       ///< enum struct used for ezEnum.
    Bitflags = EZ_BIT(2),     ///< bitflags struct used for ezBitflags.
    Class = EZ_BIT(3),        ///< A class or struct. The above flags are mutually exclusive.

    Abstract = EZ_BIT(4),     ///< Type is abstract.
    Phantom = EZ_BIT(5),      ///< De-serialized type information that cannot be created on this process.
    Minimal = EZ_BIT(6),      ///< Does not contain any property, function or attribute information. Used only for versioning.
    Default = 0
  };

  struct Bits
  {
    StorageType StandardType   : 1;
    StorageType IsEnum : 1;
    StorageType Bitflags : 1;
    StorageType Class : 1;
    StorageType Abstract : 1;
    StorageType Phantom : 1;
  };
};

EZ_DECLARE_FLAGS_OPERATORS(ezTypeFlags)


// ****************************************************
// ***** Templates for accessing static RTTI data *****

namespace ezInternal
{
  /// \brief [internal] Helper struct for accessing static RTTI data.
  template<typename T>
  struct ezStaticRTTI
  {
  };

  // Special implementation for types that have no base
  template<>
  struct ezStaticRTTI<ezNoBase>
  {
    static const ezRTTI* GetRTTI()
    {
      return nullptr;
    }
  };

  // Special implementation for void to make function reflection compile void return values without further specialization.
  template<>
  struct ezStaticRTTI<void>
  {
    static const ezRTTI* GetRTTI()
    {
      return nullptr;
    }
  };

  template <typename T>
  EZ_ALWAYS_INLINE const ezRTTI* GetStaticRTTI(ezTraitInt<1>) // class derived from ezReflectedClass
  {
    return T::GetStaticRTTI();
  }

  template <typename T>
  EZ_ALWAYS_INLINE const ezRTTI* GetStaticRTTI(ezTraitInt<0>) // static rtti
  {
    // Since this is pure C++ and no preprocessor macro, calling it with types such as 'int' and 'ezInt32' will
    // actually return the same RTTI object, which would not be possible with a purely macro based solution

    return ezStaticRTTI<T>::GetRTTI();
  }

  template <typename Type>
  ezBitflags<ezTypeFlags> DetermineTypeFlags()
  {
    ezBitflags<ezTypeFlags> flags;
    ezVariant::Type::Enum type = static_cast<ezVariant::Type::Enum>(ezVariant::TypeDeduction<typename ezTypeTraits<Type>::NonConstReferenceType>::value);
    if ((type >= ezVariant::Type::FirstStandardType && type <= ezVariant::Type::LastStandardType) || EZ_IS_SAME_TYPE(ezVariant, Type))
      flags.Add(ezTypeFlags::StandardType);
    else
      flags.Add(ezTypeFlags::Class);

    if (std::is_abstract<Type>::value)
      flags.Add(ezTypeFlags::Abstract);

    return flags;
  }

  template <>
  EZ_ALWAYS_INLINE ezBitflags<ezTypeFlags> DetermineTypeFlags<ezVariant>()
  {
    return ezTypeFlags::StandardType;
  }
}

/// \brief Use this function, specialized with the type that you are interested in, to get the static RTTI data for some type.
template<typename T>
EZ_ALWAYS_INLINE const ezRTTI* ezGetStaticRTTI()
{
  return ezInternal::GetStaticRTTI<T>(ezTraitInt<EZ_IS_DERIVED_FROM_STATIC(ezReflectedClass, T)>());
}

// **************************************************
// ***** Macros for declaring types reflectable *****

#define EZ_NO_LINKAGE

/// \brief Declares a type to be statically reflectable. Insert this into the header of a type to enable reflection on it.
/// This is not needed if the type is already dynamically reflectable.
#define EZ_DECLARE_REFLECTABLE_TYPE(Linkage, TYPE)                      \
  namespace ezInternal                                                  \
  {                                                                     \
    struct Linkage ezStaticRTTIWrapper_##TYPE                           \
    {                                                                   \
      static ezRTTI s_RTTI;                                             \
    };                                                                  \
                                                                        \
    /* This specialization calls the function to get the RTTI data */   \
    /* This code might get duplicated in different DLLs, but all   */   \
    /* will call the same function, so the RTTI object is unique   */   \
    template<>                                                          \
    struct ezStaticRTTI<TYPE>                                           \
    {                                                                   \
      EZ_ALWAYS_INLINE static const ezRTTI* GetRTTI()                    \
      {                                                                 \
        return &ezStaticRTTIWrapper_##TYPE::s_RTTI;                     \
      }                                                                 \
    };                                                                  \
  }

/// \brief Insert this into a class/struct to enable properties that are private members.
/// All types that have dynamic reflection (\see EZ_ADD_DYNAMIC_REFLECTION) already have this ability.
#define EZ_ALLOW_PRIVATE_PROPERTIES(SELF)                             \
  friend struct ezRTTInfo_##SELF

/// \cond
// internal helper macro
#define EZ_RTTIINFO_DECL(Type, BaseType, Version)                     \
  struct ezRTTInfo_##Type                                             \
  {                                                                   \
    static const char* GetTypeName() { return #Type; }                \
    static ezUInt32 GetTypeVersion() { return Version; }              \
                                                                      \
    typedef Type OwnType;                                             \
    typedef BaseType OwnBaseType;                                     \
                                                                      \
    static ezRTTI GetRTTI();                                          \
  };

// internal helper macro
#define EZ_RTTIINFO_GETRTTI_IMPL_BEGIN(Type, AllocatorType)           \
  ezRTTI ezRTTInfo_##Type::GetRTTI()                                  \
  {                                                                   \
    static AllocatorType Allocator;                                   \
    static ezBitflags<ezTypeFlags> flags = ezInternal::DetermineTypeFlags<Type>();\
    static ezArrayPtr<ezAbstractProperty*> Properties;                \
    static ezArrayPtr<ezAbstractFunctionProperty*> Functions;         \
    static ezArrayPtr<ezPropertyAttribute*> Attributes;               \
    static ezArrayPtr<ezAbstractMessageHandler*> MessageHandlers;     \
    static ezArrayPtr<ezMessageSenderInfo> MessageSenders;

/// \endcond

/// \brief Implements the necessary functionality for a type to be statically reflectable.
///
/// \param Type
///   The type for which the reflection functionality should be implemented.
/// \param BaseType
///   The base class type of \a Type. If it has no base class, pass ezNoBase
/// \param Version
///   The version of \a Type. Must be increased when the class serialization changes.
/// \param AllocatorType
///   The type of an ezRTTIAllocator that can be used to create and destroy instances
///   of \a Type. Pass ezRTTINoAllocator for types that should not be created dynamically.
///   Pass ezRTTIDefaultAllocator<Type> for types that should be created on the default heap.
///   Pass a custom ezRTTIAllocator type to handle allocation differently.
#define EZ_BEGIN_STATIC_REFLECTED_TYPE(Type, BaseType, Version, AllocatorType)                \
  EZ_RTTIINFO_DECL(Type, BaseType, Version)                                                   \
  ezRTTI ezInternal::ezStaticRTTIWrapper_##Type::s_RTTI = ezRTTInfo_##Type::GetRTTI();        \
  EZ_RTTIINFO_GETRTTI_IMPL_BEGIN(Type, AllocatorType)


/// \brief Ends the reflection code block that was opened with EZ_BEGIN_STATIC_REFLECTED_TYPE.
#define EZ_END_STATIC_REFLECTED_TYPE                                \
    return ezRTTI(GetTypeName(),                                    \
      ezGetStaticRTTI<OwnBaseType>(),                               \
      sizeof(OwnType),                                              \
      GetTypeVersion(),                                             \
      ezVariant::TypeDeduction<OwnType>::value,                     \
      flags,                                                        \
      &Allocator, Properties, Functions, Attributes, MessageHandlers, MessageSenders, nullptr);\
  }


/// \brief Within a EZ_BEGIN_REFLECTED_TYPE / EZ_END_REFLECTED_TYPE block, use this to start the block that declares all the properties.
#define EZ_BEGIN_PROPERTIES                                                   \
    static ezAbstractProperty* PropertyList[] =                               \
                                                                              \


/// \brief Ends the block to declare properties that was started with EZ_BEGIN_PROPERTIES.
#define EZ_END_PROPERTIES                                                     \
     ;                                                                        \
  Properties = PropertyList;                                                  \

/// \brief Within a EZ_BEGIN_REFLECTED_TYPE / EZ_END_REFLECTED_TYPE block, use this to start the block that declares all the functions.
#define EZ_BEGIN_FUNCTIONS                                                    \
    static ezAbstractFunctionProperty* FunctionList[] =                       \
                                                                              \


/// \brief Ends the block to declare functions that was started with EZ_BEGIN_FUNCTIONS.
#define EZ_END_FUNCTIONS                                                     \
     ;                                                                       \
  Functions = FunctionList;                                                  \

/// \brief Within a EZ_BEGIN_REFLECTED_TYPE / EZ_END_REFLECTED_TYPE block, use this to start the block that declares all the attributes.
#define EZ_BEGIN_ATTRIBUTES                                                   \
    static ezPropertyAttribute* AttributeList[] =                             \
                                                                              \


/// \brief Ends the block to declare attributes that was started with EZ_BEGIN_ATTRIBUTES.
#define EZ_END_ATTRIBUTES                                                     \
     ;                                                                        \
  Attributes = AttributeList;                                                 \

/// \brief Within a EZ_BEGIN_FUNCTIONS / EZ_END_FUNCTIONS block, this adds a member or static function property stored inside the RTTI data.
///
/// \param Function
///   The function to be executed, must match the C++ function name.
#define EZ_FUNCTION_PROPERTY(Function)                                         \
  (new ezFunctionProperty<decltype(&OwnType::Function)>                        \
    (EZ_STRINGIZE(Function), &OwnType::Function))                              \

/// \brief Within a EZ_BEGIN_FUNCTIONS / EZ_END_FUNCTIONS block, this adds a member or static function property stored inside the RTTI data.
/// Use this version if you need to change the name of the function or need to cast the function to one of its overload versions.
///
/// \param PropertyName
///   The name under which the property should be registered.
///
/// \param Function
///   The function to be executed, must match the C++ function name including the class name e.g. 'CLASS::NAME'.
#define EZ_FUNCTION_PROPERTY_EX(PropertyName, Function)                            \
  (new ezFunctionProperty<decltype(&Function)>                                  \
    (PropertyName, &Function))                                                  \


/// \brief Within a EZ_BEGIN_FUNCTIONS / EZ_END_FUNCTIONS block, this adds a constructor function property stored inside the RTTI data.
///
/// \param Function
///   The function to be executed in the form of CLASS::FUNCTION_NAME.
#define EZ_CONSTRUCTOR_PROPERTY(...)                                            \
  (new ezConstructorFunctionProperty<OwnType, ##__VA_ARGS__>())                   \


// [internal] Helper macro to get the return type of a getter function.
#define EZ_GETTER_TYPE(Class, GetterFunc)                                     \
  decltype(((Class*) nullptr)->GetterFunc())

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
  (new ezAccessorProperty<OwnType, EZ_GETTER_TYPE(OwnType, OwnType::Getter)>   \
    (PropertyName, &OwnType::Getter, &OwnType::Setter))                        \

/// \brief Same as EZ_ACCESSOR_PROPERTY, but no setter is provided, thus making the property read-only.
#define EZ_ACCESSOR_PROPERTY_READ_ONLY(PropertyName, Getter)                  \
  (new ezAccessorProperty<OwnType, EZ_GETTER_TYPE(OwnType, OwnType::Getter)>   \
    (PropertyName, &OwnType::Getter, nullptr))                                    \

// [internal] Helper macro to get the return type of a array getter function.
#define EZ_ARRAY_GETTER_TYPE(Class, GetterFunc)                                     \
  decltype(((Class*) nullptr)->GetterFunc(0))

/// \brief Within a EZ_BEGIN_PROPERTIES / EZ_END_PROPERTIES block, this adds a property that uses custom functions to access an array.
///
/// \param PropertyName
///   The unique (in this class) name under which the property should be registered.
/// \param GetCount
///   Function signature: ezUInt32 GetCount() const;
/// \param Getter
///   Function signature: Type GetValue(ezUInt32 uiIndex) const;
/// \param Setter
///   Function signature: void SetValue(ezUInt32 uiIndex, Type value);
/// \param Insert
///   Function signature: void Insert(ezUInt32 uiIndex, Type value);
/// \param Remove
///   Function signature: void Remove(ezUInt32 uiIndex);
#define EZ_ARRAY_ACCESSOR_PROPERTY(PropertyName, GetCount, Getter, Setter, Insert, Remove)   \
  (new ezAccessorArrayProperty<OwnType, EZ_ARRAY_GETTER_TYPE(OwnType, OwnType::Getter)>   \
    (PropertyName, &OwnType::GetCount, &OwnType::Getter, &OwnType::Setter, &OwnType::Insert, &OwnType::Remove)) \

/// \brief Same as EZ_ARRAY_ACCESSOR_PROPERTY, but no setter is provided, thus making the property read-only.
#define EZ_ARRAY_ACCESSOR_PROPERTY_READ_ONLY(PropertyName, GetCount, Getter)   \
  (new ezAccessorArrayProperty<OwnType, EZ_ARRAY_GETTER_TYPE(OwnType, OwnType::Getter)>   \
    (PropertyName, &OwnType::GetCount, &OwnType::Getter, nullptr, nullptr, nullptr)) \

#define EZ_SET_CONTAINER_TYPE(Class, GetterFunc)                                     \
  decltype(((Class*) nullptr)->GetterFunc())

#define EZ_SET_CONTAINER_SUB_TYPE(Class, GetterFunc)                 \
  ezContainerSubTypeResolver<ezTypeTraits<decltype(((Class*) nullptr)->GetterFunc())>::NonConstReferenceType>::Type

/// \brief Within a EZ_BEGIN_PROPERTIES / EZ_END_PROPERTIES block, this adds a property that uses custom functions to access a set.
///
/// \param PropertyName
///   The unique (in this class) name under which the property should be registered.
/// \param GetValues
///   Function signature: Container<Type> GetValues() const;
/// \param Insert
///   Function signature: void Insert(Type value);
/// \param Remove
///   Function signature: void Remove(Type value);
///
/// \note Container<Type> can be any container that can be iterated via range based for loops.
#define EZ_SET_ACCESSOR_PROPERTY(PropertyName, GetValues, Insert, Remove)   \
  (new ezAccessorSetProperty<OwnType, ezFunctionParameterTypeResolver<0, decltype(&OwnType::Insert)>::ParameterType, EZ_SET_CONTAINER_TYPE(OwnType, GetValues)>   \
    (PropertyName, &OwnType::GetValues, &OwnType::Insert, &OwnType::Remove)) \

/// \brief Same as EZ_SET_ACCESSOR_PROPERTY, but no setter is provided, thus making the property read-only.
#define EZ_SET_ACCESSOR_PROPERTY_READ_ONLY(PropertyName, GetValues)   \
  (new ezAccessorSetProperty<OwnType, EZ_SET_CONTAINER_SUB_TYPE(OwnType, GetValues), EZ_SET_CONTAINER_TYPE(OwnType, GetValues)>   \
    (PropertyName, &OwnType::GetValues, nullptr, nullptr)) \

/// \brief Within a EZ_BEGIN_PROPERTIES / EZ_END_PROPERTIES block, this adds a property that uses custom functions to access a map.
///
/// \param PropertyName
///   The unique (in this class) name under which the property should be registered.
/// \param GetContainer
///   Function signature: const Container<Key, Type>& GetValues() const;
/// \param Insert
///   Function signature: void Insert(const char* szKey, Type value);
/// \param Remove
///   Function signature: void Remove(const char* szKey);
///
/// \note Container can be ezMap or ezHashTable
#define EZ_MAP_ACCESSOR_PROPERTY(PropertyName, GetContainer, Insert, Remove)   \
  (new ezAccessorMapProperty<OwnType, ezFunctionParameterTypeResolver<1, decltype(&OwnType::Insert)>::ParameterType, EZ_SET_CONTAINER_TYPE(OwnType, GetContainer)>   \
    (PropertyName, &OwnType::GetContainer, &OwnType::Insert, &OwnType::Remove)) \

/// \brief Same as EZ_MAP_ACCESSOR_PROPERTY, but no setter is provided, thus making the property read-only.
#define EZ_MAP_ACCESSOR_PROPERTY_READ_ONLY(PropertyName, GetContainer)   \
  (new ezAccessorMapProperty<OwnType, EZ_SET_CONTAINER_SUB_TYPE(OwnType, GetContainer), EZ_SET_CONTAINER_TYPE(OwnType, GetContainer)>   \
    (PropertyName, &OwnType::GetContainer, nullptr, nullptr)) \

/// \brief Within a EZ_BEGIN_PROPERTIES / EZ_END_PROPERTIES block, this adds a property that uses custom getter / setter functions.
///
/// \param PropertyName
///   The unique (in this class) name under which the property should be registered.
/// \param EnumType
///   The name of the enum struct used by ezEnum.
/// \param Getter
///   The getter function for this property.
/// \param Setter
///   The setter function for this property.
#define EZ_ENUM_ACCESSOR_PROPERTY(PropertyName, EnumType, Getter, Setter)                    \
  (new ezEnumAccessorProperty<OwnType, EnumType, EZ_GETTER_TYPE(OwnType, OwnType::Getter)>    \
    (PropertyName, &OwnType::Getter, &OwnType::Setter))                                       \

/// \brief Same as EZ_ENUM_ACCESSOR_PROPERTY, but no setter is provided, thus making the property read-only.
#define EZ_ENUM_ACCESSOR_PROPERTY_READ_ONLY(PropertyName, EnumType, Getter)                  \
  (new ezEnumAccessorProperty<OwnType, EnumType, EZ_GETTER_TYPE(OwnType, OwnType::Getter)>    \
    (PropertyName, &OwnType::Getter, nullptr))                                                \

/// \brief Same as EZ_ENUM_ACCESSOR_PROPERTY, but for bitfields.
#define EZ_BITFLAGS_ACCESSOR_PROPERTY(PropertyName, BitflagsType, Getter, Setter)                    \
  (new ezBitflagsAccessorProperty<OwnType, BitflagsType, EZ_GETTER_TYPE(OwnType, OwnType::Getter)>    \
    (PropertyName, &OwnType::Getter, &OwnType::Setter))                                               \

/// \brief Same as EZ_BITFLAGS_ACCESSOR_PROPERTY, but no setter is provided, thus making the property read-only.
#define EZ_BITFLAGS_ACCESSOR_PROPERTY_READ_ONLY(PropertyName, BitflagsType, Getter)                  \
  (new ezBitflagsAccessorProperty<OwnType, BitflagsType, EZ_GETTER_TYPE(OwnType, OwnType::Getter)>    \
    (PropertyName, &OwnType::Getter, nullptr))                                                        \


// [internal] Helper macro to get the type of a class member.
#define EZ_MEMBER_TYPE(Class, Member)                                         \
  decltype(((Class*) nullptr)->Member)

#define EZ_MEMBER_CONTAINER_SUB_TYPE(Class, Member)                 \
  ezContainerSubTypeResolver<ezTypeTraits<decltype(((Class*) nullptr)->Member)>::NonConstReferenceType>::Type

/// \brief Within a EZ_BEGIN_PROPERTIES / EZ_END_PROPERTIES block, this adds a property that actually exists as a member.
///
/// \param PropertyName
///   The unique (in this class) name under which the property should be registered.
/// \param MemberName
///   The name of the member variable that should get exposed as a property.
///
/// \note Since the member is exposed directly, there is no way to know when the variable was modified. That also means
/// no custom limits to the values can be applied. If that becomes necessary, just add getter / setter functions and
/// expose the property as a EZ_ENUM_ACCESSOR_PROPERTY instead.
#define EZ_MEMBER_PROPERTY(PropertyName, MemberName)                          \
  (new ezMemberProperty<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName)>          \
    (PropertyName,                                                            \
    &ezPropertyAccessor<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetValue,            \
    &ezPropertyAccessor<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::SetValue,            \
    &ezPropertyAccessor<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetPropertyPointer))  \

/// \brief Same as EZ_MEMBER_PROPERTY, but the property is read-only.
#define EZ_MEMBER_PROPERTY_READ_ONLY(PropertyName, MemberName)                \
  (new ezMemberProperty<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName)>          \
    (PropertyName,                                                            \
    &ezPropertyAccessor<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetValue,            \
    nullptr,                                                                     \
    &ezPropertyAccessor<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetPropertyPointer))  \

/// \brief Same as EZ_MEMBER_PROPERTY, but the property is an array (ezHybridArray, ezDynamicArray or ezDeque).
#define EZ_ARRAY_MEMBER_PROPERTY(PropertyName, MemberName)                          \
  (new ezMemberArrayProperty<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName), EZ_MEMBER_CONTAINER_SUB_TYPE(OwnType, MemberName)>          \
    (PropertyName,                                                            \
    &ezArrayPropertyAccessor<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetConstContainer, \
    &ezArrayPropertyAccessor<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetContainer))  \

/// \brief Same as EZ_MEMBER_PROPERTY, but the property is a read-only array (ezHybridArray, ezDynamicArray or ezDeque).
#define EZ_ARRAY_MEMBER_PROPERTY_READ_ONLY(PropertyName, MemberName)                          \
  (new ezMemberArrayProperty<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName), EZ_MEMBER_CONTAINER_SUB_TYPE(OwnType, MemberName)>          \
    (PropertyName,                                                            \
    &ezArrayPropertyAccessor<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetConstContainer, \
    nullptr))  \

/// \brief Same as EZ_MEMBER_PROPERTY, but the property is a set (ezSet, ezHashSet).
#define EZ_SET_MEMBER_PROPERTY(PropertyName, MemberName)                          \
  (new ezMemberSetProperty<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName), EZ_MEMBER_CONTAINER_SUB_TYPE(OwnType, MemberName)>          \
    (PropertyName,                                                            \
    &ezSetPropertyAccessor<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetConstContainer, \
    &ezSetPropertyAccessor<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetContainer))  \

/// \brief Same as EZ_MEMBER_PROPERTY, but the property is a read-only set (ezSet, ezHashSet).
#define EZ_SET_MEMBER_PROPERTY_READ_ONLY(PropertyName, MemberName)                          \
  (new ezMemberSetProperty<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName), EZ_MEMBER_CONTAINER_SUB_TYPE(OwnType, MemberName)>          \
    (PropertyName,                                                            \
    &ezSetPropertyAccessor<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetConstContainer, \
    nullptr))  \

/// \brief Same as EZ_MEMBER_PROPERTY, but the property is a map (ezMap, ezHashTable).
#define EZ_MAP_MEMBER_PROPERTY(PropertyName, MemberName)                          \
  (new ezMemberMapProperty<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName), EZ_MEMBER_CONTAINER_SUB_TYPE(OwnType, MemberName)>          \
    (PropertyName,                                                            \
    &ezMapPropertyAccessor<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetConstContainer, \
    &ezMapPropertyAccessor<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetContainer))  \

/// \brief Same as EZ_MEMBER_PROPERTY, but the property is a read-only map (ezMap, ezHashTable).
#define EZ_MAP_MEMBER_PROPERTY_READ_ONLY(PropertyName, MemberName)                          \
  (new ezMemberMapProperty<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName), EZ_MEMBER_CONTAINER_SUB_TYPE(OwnType, MemberName)>          \
    (PropertyName,                                                            \
    &ezMapPropertyAccessor<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetConstContainer, \
    nullptr))  \

/// \brief Within a EZ_BEGIN_PROPERTIES / EZ_END_PROPERTIES block, this adds a property that actually exists as a member.
///
/// \param PropertyName
///   The unique (in this class) name under which the property should be registered.
/// \param EnumType
///   Name of the struct used by ezEnum.
/// \param MemberName
///   The name of the member variable that should get exposed as a property.
///
/// \note Since the member is exposed directly, there is no way to know when the variable was modified. That also means
/// no custom limits to the values can be applied. If that becomes necessary, just add getter / setter functions and
/// expose the property as a EZ_ACCESSOR_PROPERTY instead.
#define EZ_ENUM_MEMBER_PROPERTY(PropertyName, EnumType, MemberName)                                               \
  (new ezEnumMemberProperty<OwnType, EnumType, EZ_MEMBER_TYPE(OwnType, MemberName)>                                \
    (PropertyName,                                                                                                \
    &ezPropertyAccessor<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetValue,            \
    &ezPropertyAccessor<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::SetValue,            \
    &ezPropertyAccessor<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetPropertyPointer))  \

/// \brief Same as EZ_ENUM_MEMBER_PROPERTY, but the property is read-only.
#define EZ_ENUM_MEMBER_PROPERTY_READ_ONLY(PropertyName, EnumType, MemberName)                                     \
  (new ezEnumMemberProperty<OwnType, EnumType, EZ_MEMBER_TYPE(OwnType, MemberName)>                                \
    (PropertyName,                                                                                                \
    &ezPropertyAccessor<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetValue,            \
    nullptr,                                                                                                      \
    &ezPropertyAccessor<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetPropertyPointer))  \

/// \brief Same as EZ_ENUM_MEMBER_PROPERTY, but for bitfields.
#define EZ_BITFLAGS_MEMBER_PROPERTY(PropertyName, BitflagsType, MemberName)                                       \
  (new ezBitflagsMemberProperty<OwnType, BitflagsType, EZ_MEMBER_TYPE(OwnType, MemberName)>                        \
    (PropertyName,                                                                                                \
    &ezPropertyAccessor<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetValue,            \
    &ezPropertyAccessor<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::SetValue,            \
    &ezPropertyAccessor<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetPropertyPointer))  \

/// \brief Same as EZ_ENUM_MEMBER_PROPERTY_READ_ONLY, but for bitfields.
#define EZ_BITFLAGS_MEMBER_PROPERTY_READ_ONLY(PropertyName, BitflagsType, MemberName)                             \
  (new ezBitflagsMemberProperty<OwnType, BitflagsType, EZ_MEMBER_TYPE(OwnType, MemberName)>                        \
    (PropertyName,                                                                                                \
    &ezPropertyAccessor<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetValue,            \
    nullptr,                                                                                                      \
    &ezPropertyAccessor<OwnType, EZ_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetPropertyPointer))  \



/// \brief Within a EZ_BEGIN_PROPERTIES / EZ_END_PROPERTIES block, this adds a constant property stored inside the RTTI data.
///
/// \param PropertyName
///   The unique (in this class) name under which the property should be registered.
/// \param Value
///   The constant value to be stored.
#define EZ_CONSTANT_PROPERTY(PropertyName, Value)                             \
  (new ezConstantProperty<decltype(Value)>                                     \
    (PropertyName, Value))                                                     \



// [internal] Helper macro
#define EZ_ENUM_VALUE_TO_CONSTANT_PROPERTY(name)                              \
  EZ_CONSTANT_PROPERTY(EZ_STRINGIZE(name), (Storage)name),                    \

/// \brief Within a EZ_BEGIN_STATIC_REFLECTED_ENUM / EZ_END_STATIC_REFLECTED_ENUM block, this converts a
/// list of enum values into constant RTTI properties.
#define EZ_ENUM_CONSTANTS(...)                                                \
  EZ_EXPAND_ARGS(EZ_ENUM_VALUE_TO_CONSTANT_PROPERTY, ##__VA_ARGS__)           \

/// \brief Within a EZ_BEGIN_STATIC_REFLECTED_ENUM / EZ_END_STATIC_REFLECTED_ENUM block, this converts a
/// an enum value into a constant RTTI property.
#define EZ_ENUM_CONSTANT(Value)                                               \
EZ_CONSTANT_PROPERTY(EZ_STRINGIZE(Value), (Storage)Value)                     \

/// \brief Within a EZ_BEGIN_STATIC_REFLECTED_ENUM / EZ_END_STATIC_REFLECTED_ENUM block, this converts a
/// list of bitflags into constant RTTI properties.
#define EZ_BITFLAGS_CONSTANTS(...)                                            \
  EZ_EXPAND_ARGS(EZ_ENUM_VALUE_TO_CONSTANT_PROPERTY, ##__VA_ARGS__)           \

/// \brief Within a EZ_BEGIN_STATIC_REFLECTED_ENUM / EZ_END_STATIC_REFLECTED_ENUM block, this converts a
/// an bitflags into a constant RTTI property.
#define EZ_BITFLAGS_CONSTANT(Value)                                           \
EZ_CONSTANT_PROPERTY(EZ_STRINGIZE(Value), (Storage)Value)                     \



/// \brief Implements the necessary functionality for an enum to be statically reflectable.
///
/// \param Type
///   The enum struct used by ezEnum for which reflection should be defined.
/// \param Version
///   The version of \a Type. Must be increased when the class changes.
#define EZ_BEGIN_STATIC_REFLECTED_ENUM(Type, Version)                           \
  EZ_BEGIN_STATIC_REFLECTED_TYPE(Type, ezEnumBase, Version, ezRTTINoAllocator); \
  typedef Type::StorageType Storage;                                            \
  EZ_BEGIN_PROPERTIES                                                           \
  {                                                                             \
    EZ_CONSTANT_PROPERTY(EZ_STRINGIZE(Type::Default), (Storage)Type::Default),  \

#define EZ_END_STATIC_REFLECTED_ENUM()                                        \
  }                                                                           \
  EZ_END_PROPERTIES                                                           \
  flags |= ezTypeFlags::IsEnum;                                               \
  flags.Remove(ezTypeFlags::Class);                                       \
  EZ_END_STATIC_REFLECTED_TYPE                                                \


/// \brief Implements the necessary functionality for bitflags to be statically reflectable.
///
/// \param Type
///   The bitflags struct used by ezBitflags for which reflection should be defined.
/// \param Version
///   The version of \a Type. Must be increased when the class changes.
#define EZ_BEGIN_STATIC_REFLECTED_BITFLAGS(Type, Version)                           \
  EZ_BEGIN_STATIC_REFLECTED_TYPE(Type, ezBitflagsBase, Version, ezRTTINoAllocator); \
  typedef Type::StorageType Storage;                                                \
  EZ_BEGIN_PROPERTIES                                                               \
  {                                                                                 \
    EZ_CONSTANT_PROPERTY(EZ_STRINGIZE(Type::Default), (Storage)Type::Default),      \

#define EZ_END_STATIC_REFLECTED_BITFLAGS()                                    \
  }                                                                           \
  EZ_END_PROPERTIES                                                           \
  flags |= ezTypeFlags::Bitflags;                                             \
  flags.Remove(ezTypeFlags::Class);                                       \
  EZ_END_STATIC_REFLECTED_TYPE                                                \



/// \brief Within an EZ_BEGIN_REFLECTED_TYPE / EZ_END_REFLECTED_TYPE block, use this to start the block that declares all the message handlers.
#define EZ_BEGIN_MESSAGEHANDLERS                                              \
    static ezAbstractMessageHandler* HandlerList[] =                          \


/// \brief Ends the block to declare message handlers that was started with EZ_BEGIN_MESSAGEHANDLERS.
#define EZ_END_MESSAGEHANDLERS                                                \
    ;                                                                         \
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
  new ezInternal::MessageHandler<EZ_IS_CONST_MESSAGE_HANDLER(OwnType, MessageType, &OwnType::FunctionName)>::Impl<OwnType, MessageType, &OwnType::FunctionName>()  \


/// \brief Within an EZ_BEGIN_REFLECTED_TYPE / EZ_END_REFLECTED_TYPE block, use this to start the block that declares all the message senders.
#define EZ_BEGIN_MESSAGESENDERS                                               \
    static ezMessageSenderInfo SenderList[] =                                 \


/// \brief Ends the block to declare message senders that was started with EZ_BEGIN_MESSAGESENDERS.
#define EZ_END_MESSAGESENDERS                                                 \
    ;                                                                         \
  MessageSenders = SenderList;                                                \

/// \brief Within an EZ_BEGIN_MESSAGESENDERS / EZ_END_MESSAGESENDERS block, this adds another message sender.
///
/// \param MemberName
///   The name of the member variable that should get exposed as a message sender.
///
/// \note A message sender must be derived from ezMessageSenderBase.
#define EZ_MESSAGE_SENDER(MemberName)                         \
  { #MemberName, ezGetStaticRTTI<EZ_MEMBER_TYPE(OwnType, MemberName)::MessageType>() }  \

