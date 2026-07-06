#pragma once

#include <Core/Utils/Blackboard.h>

namespace ezInternal
{
  template <typename T>
  T ApplyBlackboardValueWithStrength(const T& currentValue, const ezBlackboard& blackboard, ezTempHashedString sName, ezTempHashedString sNameStrength)
  {
    if (const auto* pEntry = blackboard.GetEntry(sName))
    {
      if (pEntry->m_Value.CanConvertTo<T>() == false)
      {
        return currentValue;
      }

      T newValue = pEntry->m_Value.ConvertTo<T>();
      if (const auto* pStrengthEntry = blackboard.GetEntry(sNameStrength))
      {
        const float fStrength = pStrengthEntry->m_Value.ConvertTo<float>();
        return ezMath::Lerp(currentValue, newValue, fStrength);
      }

      return newValue;
    }

    return currentValue;
  }
} // namespace ezInternal

#define EZ_STRENGTH_SUFFIX "_Strength"

#define EZ_APPLY_BLACKBOARD_VALUE_WITH_STRENGTH(currentValue, blackboard, name) \
  ezInternal::ApplyBlackboardValueWithStrength(currentValue, blackboard, ezTempHashedString(#name), ezTempHashedString(#name EZ_STRENGTH_SUFFIX))
