#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Reflection/Implementation/StaticRTTI.h>
#include <Foundation/Types/TypeTraits.h>

#define EZ_DECLARE_VARIANCE_HASH_HELPER(TYPE)                        \
  template <>                                                        \
  struct ezHashHelper<TYPE>                                          \
  {                                                                  \
    EZ_ALWAYS_INLINE static ezUInt32 Hash(const TYPE& value)         \
    {                                                                \
      return ezHashingUtils::xxHash32(&value, sizeof(TYPE));         \
    }                                                                \
    EZ_ALWAYS_INLINE static bool Equal(const TYPE& a, const TYPE& b) \
    {                                                                \
      return a == b;                                                 \
    }                                                                \
  };

struct EZ_FOUNDATION_DLL ezVarianceTypeBase
{
  EZ_DECLARE_POD_TYPE();

  float m_fVariance = 0;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezVarianceTypeBase);

struct EZ_FOUNDATION_DLL ezVarianceTypeFloat : public ezVarianceTypeBase
{
  EZ_DECLARE_POD_TYPE();
  bool operator==(const ezVarianceTypeFloat& rhs) const
  {
    return m_fVariance == rhs.m_fVariance && m_Value == rhs.m_Value;
  }
  bool operator!=(const ezVarianceTypeFloat& rhs) const
  {
    return !(*this == rhs);
  }
  float m_Value = 0;
};

EZ_DECLARE_VARIANCE_HASH_HELPER(ezVarianceTypeFloat);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezVarianceTypeFloat);
EZ_DECLARE_CUSTOM_VARIANT_TYPE(ezVarianceTypeFloat);

struct EZ_FOUNDATION_DLL ezVarianceTypeTime : public ezVarianceTypeBase
{
  EZ_DECLARE_POD_TYPE();
  bool operator==(const ezVarianceTypeTime& rhs) const
  {
    return m_fVariance == rhs.m_fVariance && m_Value == rhs.m_Value;
  }
  bool operator!=(const ezVarianceTypeTime& rhs) const
  {
    return !(*this == rhs);
  }
  ezTime m_Value;
};

EZ_DECLARE_VARIANCE_HASH_HELPER(ezVarianceTypeTime);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezVarianceTypeTime);
EZ_DECLARE_CUSTOM_VARIANT_TYPE(ezVarianceTypeTime);

struct EZ_FOUNDATION_DLL ezVarianceTypeAngle : public ezVarianceTypeBase
{
  EZ_DECLARE_POD_TYPE();
  bool operator==(const ezVarianceTypeAngle& rhs) const
  {
    return m_fVariance == rhs.m_fVariance && m_Value == rhs.m_Value;
  }
  bool operator!=(const ezVarianceTypeAngle& rhs) const
  {
    return !(*this == rhs);
  }
  ezAngle m_Value;
};

EZ_DECLARE_VARIANCE_HASH_HELPER(ezVarianceTypeAngle);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezVarianceTypeAngle);
EZ_DECLARE_CUSTOM_VARIANT_TYPE(ezVarianceTypeAngle);
