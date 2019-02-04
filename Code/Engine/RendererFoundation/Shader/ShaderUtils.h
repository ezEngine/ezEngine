#pragma once

#include <RendererFoundation/Basics.h>

#include <Foundation/Math/Float16.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>

namespace ezShaderUtils
{
  EZ_ALWAYS_INLINE ezUInt32 Float3ToRGB10(ezVec3 value)
  {
    const ezVec3 unsignedValue = value * 0.5f + ezVec3(0.5f);

    const ezUInt32 r = ezMath::Clamp(static_cast<ezUInt32>(unsignedValue.x * 1023.0f + 0.5f), 0u, 1023u);
    const ezUInt32 g = ezMath::Clamp(static_cast<ezUInt32>(unsignedValue.y * 1023.0f + 0.5f), 0u, 1023u);
    const ezUInt32 b = ezMath::Clamp(static_cast<ezUInt32>(unsignedValue.z * 1023.0f + 0.5f), 0u, 1023u);

    return r | (g << 10) | (b << 20);
  }

  EZ_ALWAYS_INLINE ezUInt32 PackFloat16intoUint(ezFloat16 x, ezFloat16 y)
  {
    const ezUInt32 r = x.GetRawData();
    const ezUInt32 g = y.GetRawData();

    return r | (g << 16);
  }

  EZ_ALWAYS_INLINE ezUInt32 Float2ToRG16F(ezVec2 value)
  {
    const ezUInt32 r = ezFloat16(value.x).GetRawData();
    const ezUInt32 g = ezFloat16(value.y).GetRawData();

    return r | (g << 16);
  }
}

