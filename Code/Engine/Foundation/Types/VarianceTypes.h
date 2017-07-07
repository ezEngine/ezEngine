#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Reflection/Reflection.h>

struct EZ_FOUNDATION_DLL ezVarianceTypeBase
{
  EZ_DECLARE_POD_TYPE();

  float m_fVariance = 0;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezVarianceTypeBase);

struct EZ_FOUNDATION_DLL ezVarianceTypeFloat : public ezVarianceTypeBase
{
  EZ_DECLARE_POD_TYPE();

  float m_Value = 0;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezVarianceTypeFloat);

struct EZ_FOUNDATION_DLL ezVarianceTypeTime : public ezVarianceTypeBase
{
  EZ_DECLARE_POD_TYPE();

  ezTime m_Value;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezVarianceTypeTime);

struct EZ_FOUNDATION_DLL ezVarianceTypeAngle : public ezVarianceTypeBase
{
  EZ_DECLARE_POD_TYPE();

  ezAngle m_Value;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezVarianceTypeAngle);
