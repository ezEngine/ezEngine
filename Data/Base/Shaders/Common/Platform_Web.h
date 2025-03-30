#pragma once

#define PLATFORM_WEB EZ_OFF

#if defined(WGSL)

#  undef PLATFORM_SHADER
#  define PLATFORM_SHADER EZ_ON

#  undef PLATFORM_WEB
#  define PLATFORM_WEB EZ_ON

#  define BEGIN_PUSH_CONSTANTS(Name) cbuffer Name
#  define END_PUSH_CONSTANTS(Name) ;
#  define GET_PUSH_CONSTANT(Name, Constant) Constant

#  define BEGIN_MATERIAL_CONSTANTS cbuffer materialData
#  define END_MATERIAL_CONSTANTS ;
#  define GetMaterialData(x) x

#  define SUPPORTS_TEXEL_BUFFER EZ_OFF
#  define SUPPORTS_MSAA_ARRAYS EZ_OFF

// GetRenderTargetSamplePosition does not have an equivalent function in Vulkan so these values are hard-coded.
// https://learn.microsoft.com/windows/win32/api/d3d11/ne-d3d11-d3d11_standard_multisample_quality_levels
static const float2 offsets[] =
  {
    // 1x MSAA
    float2(0, 0),
    // 2x MSAA
    float2(4, 4),
    float2(-4, -4),
    // 4x MSAA
    float2(-2, -6),
    float2(6, -2),
    float2(-6, 2),
    float2(2, 6),
    // 8x MSAA
    float2(1, -3),
    float2(-1, 3),
    float2(-5, 1),
    float2(-3, -5),
    float2(-5, 5),
    float2(-7, -1),
    float2(3, 7),
    float2(7, -7),
    // 16x MSAA
    float2(1, 1),
    float2(-1, -3),
    float2(-3, 2),
    float2(4, -1),
    float2(-5, -2),
    float2(2, 5),
    float2(5, 3),
    float2(3, -5),
    float2(-2, 6),
    float2(0, -7),
    float2(-4, -6),
    float2(-6, 4),
    float2(-8, 0),
    float2(7, -4),
    float2(6, 7),
    float2(-7, -8),
};

// Workaround for error: EvaluateAttributeAtSample intrinsic function unimplemented
// See https://github.com/microsoft/DirectXShaderCompiler/issues/3649
float ezEvaluateAttributeAtSample(float Attribute, uint SampleIndex, uint NumMsaaSamples)
{
  float2 sampleOffset = offsets[NumMsaaSamples + SampleIndex - 1] * 0.125f;
  return Attribute + ddx(Attribute) * sampleOffset.x + ddy(Attribute) * sampleOffset.y;
}

float2 ezEvaluateAttributeAtSample(float2 Attribute, uint SampleIndex, uint NumMsaaSamples)
{
  float2 sampleOffset = offsets[NumMsaaSamples + SampleIndex - 1] * 0.125f;
  return Attribute + ddx(Attribute) * sampleOffset.x + ddy(Attribute) * sampleOffset.y;
}

float3 ezEvaluateAttributeAtSample(float3 Attribute, uint SampleIndex, uint NumMsaaSamples)
{
  float2 sampleOffset = offsets[NumMsaaSamples + SampleIndex - 1] * 0.125f;
  return Attribute + ddx(Attribute) * sampleOffset.x + ddy(Attribute) * sampleOffset.y;
}
float4 ezEvaluateAttributeAtSample(float4 Attribute, uint SampleIndex, uint NumMsaaSamples)
{
  float2 sampleOffset = offsets[NumMsaaSamples + SampleIndex - 1] * 0.125f;
  return Attribute + ddx(Attribute) * sampleOffset.x + ddy(Attribute) * sampleOffset.y;
}

float4 ezSampleLevel_PointClampBorder(Texture2DArray DepthTexture, SamplerState DepthSampler, float2 SamplePos, int ArrayIndex, int MipLevel, float4 BorderColor)
{
  // Get the texture size at the specified mip level
  uint width, height, elements, levels;
  DepthTexture.GetDimensions(0, width, height, elements, levels);
  MipLevel = clamp(MipLevel, 0, levels - 1);
  DepthTexture.GetDimensions(MipLevel, width, height, elements, levels);
  // Convert normalized coordinates to texel space
  float2 texelCoords = SamplePos * int2(width, height);
  // Get the integer parts of the coordinates
  int2 texelBase = int2(floor(texelCoords));
  float4 texel = DepthTexture.Load(int4(texelBase, ArrayIndex, MipLevel));
  // validUV is one if within [0, 1] range on each axis.
  bool validUV = all(saturate(SamplePos) == SamplePos);
  // Apply border color if out of [0, 1] bounds.
  return validUV ? texel : BorderColor;
}

#endif
