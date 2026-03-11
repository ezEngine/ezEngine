#pragma once

/// \file

#include <Foundation/Reflection/Implementation/StaticRTTI.h>

/// \brief Adds dynamic reflection capabilities to a class declaration.
///
/// This macro must be placed in the class declaration of every type that needs dynamic reflection.
/// It adds the necessary infrastructure for runtime type identification, including static RTTI
/// storage and helper typedefs. The class must derive from ezReflectedClass (directly or indirectly)
/// to provide the virtual GetDynamicRTTI() method.
///
/// Unlike EZ_ADD_DYNAMIC_REFLECTION, this variant does not automatically implement GetDynamicRTTI(),
/// allowing for custom implementations or abstract base classes.
#define EZ_ADD_DYNAMIC_REFLECTION_NO_GETTER(SELF, BASE_TYPE) \
  EZ_ALLOW_PRIVATE_PROPERTIES(SELF);                         \
                                                             \
public:                                                      \
  using OWNTYPE = SELF;                                      \
  using SUPER = BASE_TYPE;                                   \
  EZ_ALWAYS_INLINE static const ezRTTI* GetStaticRTTI()      \
  {                                                          \
    return &SELF::s_RTTI;                                    \
  }                                                          \
                                                             \
private:                                                     \
  static ezRTTI s_RTTI;                                      \
  EZ_REFLECTION_DEBUG_CODE


#define EZ_ADD_DYNAMIC_REFLECTION(SELF, BASE_TYPE)      \
  EZ_ADD_DYNAMIC_REFLECTION_NO_GETTER(SELF, BASE_TYPE)  \
public:                                                 \
  virtual const ezRTTI* GetDynamicRTTI() const override \
  {                                                     \
    return &SELF::s_RTTI;                               \
  }


#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT) && EZ_ENABLED(EZ_COMPILER_MSVC)

#  define EZ_REFLECTION_DEBUG_CODE                       \
    static const ezRTTI* ReflectionDebug_GetParentType() \
    {                                                    \
      return __super::GetStaticRTTI();                   \
    }

#  define EZ_REFLECTION_DEBUG_GETPARENTFUNC &OwnType::ReflectionDebug_GetParentType

#else
#  define EZ_REFLECTION_DEBUG_CODE /*empty*/
#  define EZ_REFLECTION_DEBUG_GETPARENTFUNC nullptr
#endif


/// \brief Begins the implementation block for dynamic reflection of a type.
///
/// This macro starts the definition of the static RTTI object for a type, enabling
/// runtime type information, property access, and dynamic instantiation. Must be
/// paired with EZ_END_DYNAMIC_REFLECTED_TYPE in the implementation file.
///
/// \param Type
///   The type being reflected. Must have used EZ_ADD_DYNAMIC_REFLECTION in its declaration.
/// \param Version
///   Version number for serialization compatibility. Increment when changing reflection data.
/// \param AllocatorType
///   Controls dynamic instantiation capability:
///   - ezRTTINoAllocator: Type cannot be instantiated dynamically (abstract/interface types)
///   - ezRTTIDefaultAllocator<Type>: Standard heap allocation for concrete types
///   - Custom allocator: Specialized allocation strategy for pool/stack allocated types
#define EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(Type, Version, AllocatorType) \
  EZ_RTTIINFO_DECL(Type, Type::SUPER, Version)                        \
  ezRTTI Type::s_RTTI = GetRTTI((Type*)0);                            \
  EZ_RTTIINFO_GETRTTI_IMPL_BEGIN(Type, Type::SUPER, AllocatorType)

/// \brief Ends the reflection code block that was opened with EZ_BEGIN_DYNAMIC_REFLECTED_TYPE.
#define EZ_END_DYNAMIC_REFLECTED_TYPE                                                                                                \
  return ezRTTI(GetTypeName((OwnType*)0), ezGetStaticRTTI<OwnBaseType>(), sizeof(OwnType), GetTypeVersion((OwnType*)0),              \
    ezVariant::TypeDeduction<OwnType>::value, flags, &Allocator, Properties, Functions, Attributes, MessageHandlers, MessageSenders, \
    EZ_REFLECTION_DEBUG_GETPARENTFUNC);                                                                                              \
  }

/// \brief Same as EZ_BEGIN_DYNAMIC_REFLECTED_TYPE but forces the type to be treated as abstract by reflection even though
/// it might not be abstract from a C++ perspective.
#define EZ_BEGIN_ABSTRACT_DYNAMIC_REFLECTED_TYPE(Type, Version)     \
  EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(Type, Version, ezRTTINoAllocator) \
    flags.Add(ezTypeFlags::Abstract);

#define EZ_END_ABSTRACT_DYNAMIC_REFLECTED_TYPE EZ_END_DYNAMIC_REFLECTED_TYPE

/// \brief Base class for all types that support dynamic reflection and runtime type identification.
///
/// This class provides the fundamental virtual interface for runtime type queries and
/// the foundational reflection infrastructure. All types that need dynamic reflection
/// must derive from this class, either directly or through an inheritance chain.
///
/// Key capabilities provided:
/// - Virtual GetDynamicRTTI() for runtime type identification
/// - IsInstanceOf() for type checking and inheritance queries
/// - Foundation for property access, serialization, and other reflection features
class EZ_FOUNDATION_DLL ezReflectedClass : public ezNoBase
{
  EZ_ADD_DYNAMIC_REFLECTION_NO_GETTER(ezReflectedClass, ezNoBase);

public:
  virtual const ezRTTI* GetDynamicRTTI() const { return &ezReflectedClass::s_RTTI; }

public:
  EZ_ALWAYS_INLINE ezReflectedClass() = default;
  EZ_ALWAYS_INLINE virtual ~ezReflectedClass() = default;

  /// \brief Returns whether the type of this instance is of the given type or derived from it.
  bool IsInstanceOf(const ezRTTI* pType) const;

  /// \brief Returns whether the type of this instance is of the given type or derived from it.
  template <typename T>
  EZ_ALWAYS_INLINE bool IsInstanceOf() const
  {
    const ezRTTI* pType = ezGetStaticRTTI<T>();
    return IsInstanceOf(pType);
  }
};
