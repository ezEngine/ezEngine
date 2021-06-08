#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>

struct ezMaterialResourceSlot
{
  ezString m_sLabel;
  ezString m_sResource;
  bool m_bHighlight = false;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezMaterialResourceSlot);
