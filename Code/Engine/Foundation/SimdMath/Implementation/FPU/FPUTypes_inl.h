#pragma once

#include <Foundation/Math/Vec4.h>

namespace ezInternal
{
  typedef ezVec4 QuadFloat;
  typedef ezVec4I32 QuadInt;
  typedef ezVec4U32 QuadUInt;

  struct QuadBool
  {
    bool x, y, z, w;
  };
}

