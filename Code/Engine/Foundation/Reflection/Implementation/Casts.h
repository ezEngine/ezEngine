#pragma once

/// \file

#include <Foundation/Reflection/Implementation/DynamicRTTI.h>

EZ_WARNING_PUSH()
EZ_WARNING_DISABLE_CLANG("-Wunused-local-typedef")
EZ_WARNING_DISABLE_GCC("-Wunused-local-typedefs")

/// \brief Casts the given object to the given type with no runtime cost (like C++ static_cast).
/// This function will assert when the object is not an instance of the given type.
/// E.g. DerivedType* d = ezStaticCast<DerivedType*>(pObj);
template <typename T>
EZ_ALWAYS_INLINE T ezStaticCast(ezReflectedClass* pObject)
{
  using NonPointerT = typename ezTypeTraits<T>::NonPointerType;
  EZ_ASSERT_DEV(pObject == nullptr || pObject->IsInstanceOf<NonPointerT>(), "Invalid static cast: Object of type '{0}' is not an instance of '{1}'",
    pObject->GetDynamicRTTI()->GetTypeName(), ezGetStaticRTTI<NonPointerT>()->GetTypeName());
  return static_cast<T>(pObject);
}

/// \brief Casts the given object to the given type with no runtime cost (like C++ static_cast).
/// This function will assert when the object is not an instance of the given type.
/// E.g. const DerivedType* d = ezStaticCast<const DerivedType*>(pConstObj);
template <typename T>
EZ_ALWAYS_INLINE T ezStaticCast(const ezReflectedClass* pObject)
{
  using NonPointerT = typename ezTypeTraits<T>::NonConstReferencePointerType;
  EZ_ASSERT_DEV(pObject == nullptr || pObject->IsInstanceOf<NonPointerT>(), "Invalid static cast: Object of type '{0}' is not an instance of '{1}'",
    pObject->GetDynamicRTTI()->GetTypeName(), ezGetStaticRTTI<NonPointerT>()->GetTypeName());
  return static_cast<T>(pObject);
}

/// \brief Casts the given object to the given type with no runtime cost (like C++ static_cast).
/// This function will assert when the object is not an instance of the given type.
/// E.g. DerivedType& d = ezStaticCast<DerivedType&>(obj);
template <typename T>
EZ_ALWAYS_INLINE T ezStaticCast(ezReflectedClass& in_object)
{
  using NonReferenceT = typename ezTypeTraits<T>::NonReferenceType;
  EZ_ASSERT_DEV(in_object.IsInstanceOf<NonReferenceT>(), "Invalid static cast: Object of type '{0}' is not an instance of '{1}'",
    in_object.GetDynamicRTTI()->GetTypeName(), ezGetStaticRTTI<NonReferenceT>()->GetTypeName());
  return static_cast<T>(in_object);
}

/// \brief Casts the given object to the given type with no runtime cost (like C++ static_cast).
/// This function will assert when the object is not an instance of the given type.
/// E.g. const DerivedType& d = ezStaticCast<const DerivedType&>(constObj);
template <typename T>
EZ_ALWAYS_INLINE T ezStaticCast(const ezReflectedClass& object)
{
  using NonReferenceT = typename ezTypeTraits<T>::NonConstReferenceType;
  EZ_ASSERT_DEV(object.IsInstanceOf<NonReferenceT>(), "Invalid static cast: Object of type '{0}' is not an instance of '{1}'",
    object.GetDynamicRTTI()->GetTypeName(), ezGetStaticRTTI<NonReferenceT>()->GetTypeName());
  return static_cast<T>(object);
}

/// \brief Casts the given object to the given type with by checking if the object is actually an instance of the given type (like C++
/// dynamic_cast). This function will return a nullptr if the object is not an instance of the given type.
/// E.g. DerivedType* d = ezDynamicCast<DerivedType*>(pObj);
template <typename T>
EZ_ALWAYS_INLINE T ezDynamicCast(ezReflectedClass* pObject)
{
  if (pObject)
  {
    using NonPointerT = typename ezTypeTraits<T>::NonPointerType;
    if (pObject->IsInstanceOf<NonPointerT>())
    {
      return static_cast<T>(pObject);
    }
  }
  return nullptr;
}

/// \brief Casts the given object to the given type with by checking if the object is actually an instance of the given type (like C++
/// dynamic_cast). This function will return a nullptr if the object is not an instance of the given type.
/// E.g. const DerivedType* d = ezDynamicCast<const DerivedType*>(pConstObj);
template <typename T>
EZ_ALWAYS_INLINE T ezDynamicCast(const ezReflectedClass* pObject)
{
  if (pObject)
  {
    using NonPointerT = typename ezTypeTraits<T>::NonConstReferencePointerType;
    if (pObject->IsInstanceOf<NonPointerT>())
    {
      return static_cast<T>(pObject);
    }
  }
  return nullptr;
}

EZ_WARNING_POP()
