#pragma once

#include <Foundation/Types/TypeTraits.h>

class ezRTTI;

/// \brief A typed raw pointer.
///
/// Common use case is the storage of object pointers inside an ezVariant.
/// Has the same lifetime concerns that any other raw pointer.
/// \sa ezVariant
struct ezTypedPointer
{
  EZ_DECLARE_POD_TYPE();
  void* m_pObject = nullptr;
  const ezRTTI* m_pType = nullptr;

  ezTypedPointer() = default;
  ezTypedPointer(void* pObject, const ezRTTI* pType)
    : m_pObject(pObject)
    , m_pType(pType)
  {
  }

  bool operator==(const ezTypedPointer& rhs) const
  {
    return m_pObject == rhs.m_pObject;
  }
  bool operator!=(const ezTypedPointer& rhs) const
  {
    return m_pObject != rhs.m_pObject;
  }
};
