#pragma once

#include <Foundation/Reflection/Implementation/StaticRTTI.h>
#include <Foundation/Types/Variant.h>

class ezDocumentObject;

struct EZ_GUIFOUNDATION_DLL ezPropertySelection
{
  const ezDocumentObject* m_pObject;
  ezVariant m_Index;

  bool operator==(const ezPropertySelection& rhs) const { return m_pObject == rhs.m_pObject && m_Index == rhs.m_Index; }
};

struct EZ_GUIFOUNDATION_DLL ezPropertyClipboard
{
  ezString m_Type;
  ezVariant m_Value;
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_GUIFOUNDATION_DLL, ezPropertyClipboard)
