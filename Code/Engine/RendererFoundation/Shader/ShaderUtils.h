#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>

#include <Foundation/Math/Float16.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>

class ezShaderUtils
{
public:
  EZ_ALWAYS_INLINE static ezUInt32 Float3ToRGB10(ezVec3 value)
  {
    const ezVec3 unsignedValue = value * 0.5f + ezVec3(0.5f);

    const ezUInt32 r = ezMath::Clamp(static_cast<ezUInt32>(unsignedValue.x * 1023.0f + 0.5f), 0u, 1023u);
    const ezUInt32 g = ezMath::Clamp(static_cast<ezUInt32>(unsignedValue.y * 1023.0f + 0.5f), 0u, 1023u);
    const ezUInt32 b = ezMath::Clamp(static_cast<ezUInt32>(unsignedValue.z * 1023.0f + 0.5f), 0u, 1023u);

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

  EZ_ALWAYS_INLINE static void Float4ToRGBA16F(ezVec4 value, ezUInt32& out_RG, ezUInt32& out_BA)
  {
    out_RG = Float2ToRG16F(ezVec2(value.x, value.y));
    out_BA = Float2ToRG16F(ezVec2(value.z, value.w));
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
