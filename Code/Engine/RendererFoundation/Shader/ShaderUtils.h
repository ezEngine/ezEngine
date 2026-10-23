#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>

#include <Foundation/Math/Float16.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Time/Time.h>

class ezShaderUtils
{
public:
  EZ_ALWAYS_INLINE static ezUInt32 Float3ToRGB10(ezVec3 value)
  {
    const ezVec3 unsignedValue = value * 0.5f + ezVec3(0.5f);

    const ezUInt32 r = ezMath::ColorFloatToUnsignedInt<10>(unsignedValue.x);
    const ezUInt32 g = ezMath::ColorFloatToUnsignedInt<10>(unsignedValue.y);
    const ezUInt32 b = ezMath::ColorFloatToUnsignedInt<10>(unsignedValue.z);

    return r | (g << 10) | (b << 20);
  }

  EZ_ALWAYS_INLINE static ezUInt32 PackFloat16intoUint(ezFloat16 x, ezFloat16 y)
  {
    const ezUInt32 r = x.GetRawData();
    const ezUInt32 g = y.GetRawData();

    return r | (g << 16);
  }

  EZ_ALWAYS_INLINE static ezUInt32 Float2ToRG16F(ezVec2 value)
  {
    const ezUInt32 r = ezFloat16(value.x).GetRawData();
    const ezUInt32 g = ezFloat16(value.y).GetRawData();

    return r | (g << 16);
  }

  EZ_ALWAYS_INLINE static void Float4ToRGBA16F(ezVec4 value, ezUInt32& out_uiRG, ezUInt32& out_uiBA)
  {
    out_uiRG = Float2ToRG16F(ezVec2(value.x, value.y));
    out_uiBA = Float2ToRG16F(ezVec2(value.z, value.w));
  }


  /// The shader constants GlobalTime and WorldTime wrap around at this value to prevent floating point precision issues.
  ///
  /// Must match TIME_WRAP_AROUND in Shaders/Common/GlobalConstants.h.
  static constexpr double s_fTimeWrapAround = 1000.0;
  static constexpr float s_fInvTimeWrapAround = static_cast<float>(1.0 / s_fTimeWrapAround);

  /// Wraps the given time around at TimeWrapAround, the same way GlobalTime and WorldTime are wrapped for shaders.
  EZ_ALWAYS_INLINE static float WrappedTimeInSeconds(ezTime time)
  {
    return static_cast<float>(ezMath::Mod(time.GetSeconds(), s_fTimeWrapAround));
  }

  /// Rounds a rate (units per second) so that rate * TimeWrapAround is a whole multiple of fPeriod.
  ///
  /// Any periodic function f(wrappedTime * rate) with the given period then does not jump when the time wraps around.
  /// E.g. use a period of 1 for texture scrolling in UV space with a repeating sampler, or 2 * pi for sin / cos (see WrapSafeAngularFrequency).
  /// CPU equivalent of WrapSafeRate() in Shaders/Common/GlobalConstants.h.
  EZ_ALWAYS_INLINE static float WrapSafeRate(float fRate, float fPeriod)
  {
    return ezMath::RoundToMultiple(fRate, fPeriod * s_fInvTimeWrapAround);
  }

  /// Rounds an angular frequency (radians per second, as in sin(wrappedTime * frequency)) so that the sine / cosine does not jump when the time wraps around.
  ///
  /// CPU equivalent of WrapSafeAngularFrequency() in Shaders/Common/GlobalConstants.h.
  EZ_ALWAYS_INLINE static float WrapSafeAngularFrequency(float fFrequency)
  {
    return WrapSafeRate(fFrequency, 2.0f * ezMath::Pi<float>());
  }


  enum class ezBuiltinShaderType
  {
    CopyImage,
    CopyImageArray,
    DownscaleImage,
    DownscaleImageArray,
  };

  struct ezBuiltinShader
  {
    ezGALShaderHandle m_hActiveGALShader;
    ezGALBlendStateHandle m_hBlendState;
    ezGALDepthStencilStateHandle m_hDepthStencilState;
    ezGALRasterizerStateHandle m_hRasterizerState;
  };

  EZ_RENDERERFOUNDATION_DLL static ezDelegate<void(ezBuiltinShaderType type, ezBuiltinShader& out_shader)> g_RequestBuiltinShaderCallback;

  EZ_ALWAYS_INLINE static void RequestBuiltinShader(ezBuiltinShaderType type, ezBuiltinShader& out_shader)
  {
    g_RequestBuiltinShaderCallback(type, out_shader);
  }
};
EZ_DEFINE_AS_POD_TYPE(ezShaderUtils::ezBuiltinShaderType);
