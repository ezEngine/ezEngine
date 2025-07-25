#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptAttributes.h>
#include <Core/Scripting/ScriptClasses/ScriptExtensionClass_StableRandom.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/SimdMath/SimdRandom.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezScriptExtensionClass_StableRandom, ezNoBase, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(IntMinMax, Inout, "Position", In, "MinValue", In, "MaxValue", In, "Seed"),
    EZ_SCRIPT_FUNCTION_PROPERTY(FloatZeroToOne, Inout, "Position", In, "Seed"),
    EZ_SCRIPT_FUNCTION_PROPERTY(FloatMinMax, Inout, "Position", In, "MinValue", In, "MaxValue", In, "Seed"),
    EZ_SCRIPT_FUNCTION_PROPERTY(Vec3MinMax, Inout, "Position", In, "MinValue", In, "MaxValue", In, "Seed"),
  }
  EZ_END_FUNCTIONS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezScriptExtensionAttribute("StableRandom"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

// static
int ezScriptExtensionClass_StableRandom::IntMinMax(int& iPosition, int iMinValue, int iMaxValue, ezUInt32 uiSeed)
{
  const ezSimdVec4i result = ezSimdVec4i::Truncate(ezSimdRandom::FloatMinMax(ezSimdVec4i(iPosition), ezSimdVec4f((float)iMinValue), ezSimdVec4f((float)iMaxValue), ezSimdVec4u(uiSeed)));
  ++iPosition;
  return result.x();
}

// static
float ezScriptExtensionClass_StableRandom::FloatZeroToOne(int& iPosition, ezUInt32 uiSeed)
{
  const ezSimdVec4f result = ezSimdRandom::FloatZeroToOne(ezSimdVec4i(iPosition), ezSimdVec4u(uiSeed));
  ++iPosition;
  return result.x();
}

// static
float ezScriptExtensionClass_StableRandom::FloatMinMax(int& iPosition, float fMinValue, float fMaxValue, ezUInt32 uiSeed)
{
  const ezSimdVec4f result = ezSimdRandom::FloatMinMax(ezSimdVec4i(iPosition), ezSimdVec4f(fMinValue), ezSimdVec4f(fMaxValue), ezSimdVec4u(uiSeed));
  ++iPosition;
  return result.x();
}

// static
ezVec3 ezScriptExtensionClass_StableRandom::Vec3MinMax(int& iPosition, const ezVec3& vMinValue, const ezVec3& vMaxValue, ezUInt32 uiSeed)
{
  const ezSimdVec4i offset(0, 1, 2, 3);
  const ezSimdVec4f result = ezSimdRandom::FloatMinMax(ezSimdVec4i(iPosition) + offset, ezSimdConversion::ToVec3(vMinValue), ezSimdConversion::ToVec3(vMaxValue), ezSimdVec4u(uiSeed));
  iPosition += 4;
  return ezSimdConversion::ToVec3(result);
}
