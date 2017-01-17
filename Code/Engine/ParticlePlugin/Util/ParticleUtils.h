#pragma once

#include <ParticlePlugin/Basics.h>
#include <Foundation/Reflection/Reflection.h>

struct EZ_PARTICLEPLUGIN_DLL ezVarianceType
{
  EZ_DECLARE_POD_TYPE();

  ezVarianceType();

  float m_fValue;
  float m_fVariance;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PARTICLEPLUGIN_DLL, ezVarianceType);
