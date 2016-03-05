#pragma once

/// \file

#include <Foundation/Reflection/Implementation/DynamicRTTI.h>

template <typename T>
T ezStaticCast(ezReflectedClass* pObject)
{
  typedef ezTypeTraits<T>::NonPointerType NonPointerT;
  EZ_ASSERT_DEV(pObject->IsInstanceOf< NonPointerT >(), "Invalid static cast: Object of type '%s' is not an instance of '%s'", 
    pObject->GetDynamicRTTI()->GetTypeName(), ezGetStaticRTTI<NonPointerT>()->GetTypeName());
  return static_cast<T>(pObject);
}

template <typename T>
T ezStaticCast(const ezReflectedClass* pObject)
{
  typedef ezTypeTraits<T>::NonConstReferencePointerType NonPointerT;
  EZ_ASSERT_DEV(pObject->IsInstanceOf< NonPointerT >(), "Invalid static cast: Object of type '%s' is not an instance of '%s'",
    pObject->GetDynamicRTTI()->GetTypeName(), ezGetStaticRTTI<NonPointerT>()->GetTypeName());
  return static_cast<T>(pObject);
}

template <typename T>
T ezDynamicCast(ezReflectedClass* pObject)
{
  typedef ezTypeTraits<T>::NonPointerType NonPointerT;
  if (pObject->IsInstanceOf< NonPointerT >())
  {
    return static_cast<T>(pObject);
  }

  return nullptr;
}

template <typename T>
T ezDynamicCast(const ezReflectedClass* pObject)
{
  typedef ezTypeTraits<T>::NonConstReferencePointerType NonPointerT;
  if (pObject->IsInstanceOf< NonPointerT >())
  {
    return static_cast<T>(pObject);
  }

  return nullptr;
}

