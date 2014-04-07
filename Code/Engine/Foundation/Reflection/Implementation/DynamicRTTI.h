#pragma once

/// \file

#include <Foundation/Reflection/Implementation/StaticRTTI.h>

/// \brief This needs to be put into the class declaration of EVERY dynamically reflectable class.
///
/// This macro extends a class, such that it is now able to return its own type information via GetDynamicRTTI(),
/// which is a virtual function, that is reimplemented on each type. A class needs to be derived from ezReflectedClass
/// (at least indirectly) for this.
#define EZ_ADD_DYNAMIC_REFLECTION(SELF)                               \
  EZ_ALLOW_PRIVATE_PROPERTIES(SELF);                                  \
  public:                                                             \
    EZ_FORCE_INLINE static const ezRTTI* GetStaticRTTI()              \
    {                                                                 \
      return &SELF::s_RTTI;                                           \
    }                                                                 \
  private:                                                            \
    virtual const ezRTTI* GetDynamicRTTIImpl()                        \
    {                                                                 \
      return &SELF::s_RTTI;                                           \
    }                                                                 \
    static ezRTTI s_RTTI;

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
#define EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(Type, BaseType, AllocatorType)  \
  EZ_RTTIINFO_DECL(Type, BaseType)                                      \
  ezRTTI Type::s_RTTI = ezRTTInfo_##Type::GetRTTI();                    \
  EZ_RTTIINFO_GETRTTI_IMPL_BEGIN(Type, AllocatorType)


/// \brief All classes that should be dynamically reflectable, need to be derived from this base class.
///
/// The only functionality that this class provides is the GetDynamicRTTI() function.
class EZ_FOUNDATION_DLL ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezReflectedClass);
public:
  EZ_FORCE_INLINE ezReflectedClass() : m_pRTTI(nullptr)
  {
  }

  EZ_FORCE_INLINE const ezRTTI* GetDynamicRTTI()
  {
    if (m_pRTTI == nullptr)
    {
      m_pRTTI = GetDynamicRTTIImpl();
    }
    return m_pRTTI;
  }

private:
  const ezRTTI* m_pRTTI;
};

