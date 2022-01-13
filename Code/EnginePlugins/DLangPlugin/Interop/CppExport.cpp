#include <DLangPlugin/DLangPluginPCH.h>

#include <Foundation/Basics.h>

#include <Foundation/Math/Math.h>
#include <Foundation/Math/Quat.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Math/Vec4.h>

template class __declspec(dllexport) ezVec2Template<float>;
template class __declspec(dllexport) ezVec3Template<float>;
template class __declspec(dllexport) ezVec4Template<float>;
template class __declspec(dllexport) ezQuatTemplate<float>;

template class __declspec(dllexport) ezVec2Template<double>;
template class __declspec(dllexport) ezVec3Template<double>;
template class __declspec(dllexport) ezVec4Template<double>;
template class __declspec(dllexport) ezQuatTemplate<double>;

__declspec(dllexport) ezRandom* Make_ezRandom()
{
  return new ezRandom();
}

#include "DLangLog_inl.h"


class Test
{
public:
  __declspec(dllexport) static const int const_vars[2];
  __declspec(dllexport) static int nonconst_vars[2];
};

const int Test::const_vars[2] = {11, 23};
int Test::nonconst_vars[2] = {12, 24};


struct __declspec(dllexport) SmallStruct
{
  float value = 0;
  //float value2 = 0;
  //float value3 = 0;

  SmallStruct(float val)
    : value(val)
  {
  }

  static SmallStruct GetValue(float input)
  {
    return SmallStruct(input * 3.141f);
  }
};
