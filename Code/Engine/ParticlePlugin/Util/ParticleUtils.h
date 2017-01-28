#pragma once

#include <ParticlePlugin/Basics.h>
#include <Foundation/Reflection/Reflection.h>

struct EZ_PARTICLEPLUGIN_DLL ezVarianceTypeBase
{
  EZ_DECLARE_POD_TYPE();

  float m_fVariance = 0;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PARTICLEPLUGIN_DLL, ezVarianceTypeBase);

struct EZ_PARTICLEPLUGIN_DLL ezVarianceTypeFloat : public ezVarianceTypeBase
{
  EZ_DECLARE_POD_TYPE();

  float m_Value = 0;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PARTICLEPLUGIN_DLL, ezVarianceTypeFloat);

struct EZ_PARTICLEPLUGIN_DLL ezVarianceTypeTime : public ezVarianceTypeBase
{
  EZ_DECLARE_POD_TYPE();

  ezTime m_Value;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PARTICLEPLUGIN_DLL, ezVarianceTypeTime);

struct EZ_PARTICLEPLUGIN_DLL ezVarianceTypeAngle : public ezVarianceTypeBase
{
  EZ_DECLARE_POD_TYPE();

  ezAngle m_Value;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PARTICLEPLUGIN_DLL, ezVarianceTypeAngle);
