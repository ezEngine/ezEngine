#pragma once

/// \file

#include <Foundation/Reflection/Implementation/StaticRTTI.h>

/// \brief This needs to be put into the class declaration of EVERY dynamically reflectable class.
///
/// This macro extends a class, such that it is now able to return its own type information via GetDynamicRTTI(),
/// which is a virtual function, that is reimplemented on each type. A class needs to be derived from ezReflectedClass
/// (at least indirectly) for this.
#define EZ_ADD_DYNAMIC_REFLECTION_NO_GETTER(SELF, BASE_TYPE)                                                                               \
  EZ_ALLOW_PRIVATE_PROPERTIES(SELF);                                                                                                       \
                                                                                                                                           \
public:                                                                                                                                    \
  typedef BASE_TYPE SUPER;                                                                                                                 \
  EZ_ALWAYS_INLINE static const ezRTTI* GetStaticRTTI() { return &SELF::s_RTTI; }                                                          \
                                                                                                                                           \
private:                                                                                                                                   \
  static ezRTTI s_RTTI;                                                                                                                    \
  EZ_REFLECTION_DEBUG_CODE


#define EZ_ADD_DYNAMIC_REFLECTION(SELF, BASE_TYPE)                                                                                         \
  EZ_ADD_DYNAMIC_REFLECTION_NO_GETTER(SELF, BASE_TYPE)                                                                                     \
public:                                                                                                                                    \
  virtual const ezRTTI* GetDynamicRTTI() const override { return &SELF::s_RTTI; }


#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG) && EZ_ENABLED(EZ_COMPILER_MSVC)

#define EZ_REFLECTION_DEBUG_CODE                                                                                                           \
  static const ezRTTI* ReflectionDebug_GetParentType() { return __super::GetStaticRTTI(); }

#define EZ_REFLECTION_DEBUG_GETPARENTFUNC &OwnType::ReflectionDebug_GetParentType

#else
#define EZ_REFLECTION_DEBUG_CODE /*empty*/
#define EZ_REFLECTION_DEBUG_GETPARENTFUNC nullptr
#endif


/// \brief Implements the necessary functionality for a type to be dynamically reflectable.
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
#define EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(Type, Version, AllocatorType)                                                                      \
  EZ_RTTIINFO_DECL(Type, Type::SUPER, Version)                                                                                             \
  ezRTTI Type::s_RTTI = ezRTTInfo_##Type::GetRTTI();                                                                                       \
  EZ_RTTIINFO_GETRTTI_IMPL_BEGIN(Type, AllocatorType)

/// \brief Ends the reflection code block that was opened with EZ_BEGIN_DYNAMIC_REFLECTED_TYPE.
#define EZ_END_DYNAMIC_REFLECTED_TYPE                                                                                                      \
  return ezRTTI(GetTypeName(), ezGetStaticRTTI<OwnBaseType>(), sizeof(OwnType), GetTypeVersion(),                                          \
                ezVariant::TypeDeduction<OwnType>::value, flags, &Allocator, Properties, Functions, Attributes, MessageHandlers,           \
                MessageSenders, EZ_REFLECTION_DEBUG_GETPARENTFUNC);                                                                        \
  }

/// \brief All classes that should be dynamically reflectable, need to be derived from this base class.
class EZ_FOUNDATION_DLL ezReflectedClass : public ezNoBase
{
  EZ_ADD_DYNAMIC_REFLECTION_NO_GETTER(ezReflectedClass, ezNoBase);

public:
  virtual const ezRTTI* GetDynamicRTTI() const { return &ezReflectedClass::s_RTTI; }

public:
  EZ_ALWAYS_INLINE ezReflectedClass() = default;
  EZ_ALWAYS_INLINE virtual ~ezReflectedClass() = default;

  /// \brief Returns whether the type of this instance is of the given type or derived from it.
  EZ_ALWAYS_INLINE bool IsInstanceOf(const ezRTTI* pType) const { return GetDynamicRTTI()->IsDerivedFrom(pType); }

  /// \brief Returns whether the type of this instance is of the given type or derived from it.
  template <typename T>
  EZ_ALWAYS_INLINE bool IsInstanceOf() const
  {
    return GetDynamicRTTI()->IsDerivedFrom<T>();
  }
};
