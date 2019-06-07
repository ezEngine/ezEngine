#pragma once

#include <Foundation/Types/Variant.h>

class ezDocumentObject;

struct ezPropertySelection
{
  const ezDocumentObject* m_pObject;
  ezVariant m_Index;

  bool operator==(const ezPropertySelection& rhs) const
  {
    return m_pObject == rhs.m_pObject && m_Index == rhs.m_Index;
  }
};