#pragma once

/// \file

#include <Foundation/Reflection/Implementation/DynamicRTTI.h>

/// \brief Casts the given object to the given type with no runtime cost (like C++ static_cast).
/// This function will assert when the object is not an instance of the given type.
/// E.g. DerivedType* d = ezStaticCast<DerivedType*>(pObj);
template <typename T>
EZ_FORCE_INLINE T ezStaticCast(ezReflectedClass* pObject)
{
  typedef typename ezTypeTraits<T>::NonPointerType NonPointerT;
  EZ_ASSERT_DEV(pObject->IsInstanceOf< NonPointerT >(), "Invalid static cast: Object of type '%s' is not an instance of '%s'", 
    pObject->GetDynamicRTTI()->GetTypeName(), ezGetStaticRTTI<NonPointerT>()->GetTypeName());
  return static_cast<T>(pObject);
}

/// \brief Casts the given object to the given type with no runtime cost (like C++ static_cast). 
/// This function will assert when the object is not an instance of the given type.
/// E.g. const DerivedType* d = ezStaticCast<const DerivedType*>(pConstObj);
template <typename T>
EZ_FORCE_INLINE T ezStaticCast(const ezReflectedClass* pObject)
{
  typedef typename ezTypeTraits<T>::NonConstReferencePointerType NonPointerT;
  EZ_ASSERT_DEV(pObject->IsInstanceOf< NonPointerT >(), "Invalid static cast: Object of type '%s' is not an instance of '%s'",
    pObject->GetDynamicRTTI()->GetTypeName(), ezGetStaticRTTI<NonPointerT>()->GetTypeName());
  return static_cast<T>(pObject);
}

/// \brief Casts the given object to the given type with by checking if the object is actually an instance of the given type (like C++ dynamic_cast).
/// This function will return a nullptr if the object is not an instance of the given type.
/// E.g. DerivedType* d = ezDynamicCast<DerivedType*>(pObj);
template <typename T>
EZ_FORCE_INLINE T ezDynamicCast(ezReflectedClass* pObject)
{
  if (pObject)
  {
    typedef typename ezTypeTraits<T>::NonPointerType NonPointerT;
    if (pObject->IsInstanceOf< NonPointerT >())
    {
      return static_cast<T>(pObject);
    }
  }
  return nullptr;
}

/// \brief Casts the given object to the given type with by checking if the object is actually an instance of the given type (like C++ dynamic_cast).
/// This function will return a nullptr if the object is not an instance of the given type.
/// E.g. const DerivedType* d = ezDynamicCast<const DerivedType*>(pConstObj);
template <typename T>
EZ_FORCE_INLINE T ezDynamicCast(const ezReflectedClass* pObject)
{
  if (pObject)
  {
    typedef typename ezTypeTraits<T>::NonConstReferencePointerType NonPointerT;
    if (pObject->IsInstanceOf< NonPointerT >())
    {
      return static_cast<T>(pObject);
    }
  }
  return nullptr;
}

