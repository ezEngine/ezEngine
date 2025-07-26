#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

class EZ_CORE_DLL ezScriptExtensionClass_StableRandom
{
public:
  static int IntMinMax(int& inout_iPosition, int iMinValue, int iMaxValue, ezUInt32 uiSeed);

  static float FloatZeroToOne(int& inout_iPosition, ezUInt32 uiSeed);
  static float FloatMinMax(int& inout_iPosition, float fMinValue, float fMaxValue, ezUInt32 uiSeed);

  static ezVec3 Vec3MinMax(int& inout_iPosition, const ezVec3& vMinValue, const ezVec3& vMaxValue, ezUInt32 uiSeed);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezScriptExtensionClass_StableRandom);
