#pragma once

#define PLATFORM_VULKAN EZ_OFF

#if defined(VULKAN)

#  undef PLATFORM_SHADER
#  define PLATFORM_SHADER EZ_ON

#  undef PLATFORM_VULKAN
#  define PLATFORM_VULKAN EZ_ON

#  define BEGIN_PUSH_CONSTANTS(Name) struct EZ_SHADER_STRUCT EZ_PP_CONCAT(Name, _PushConstants)
#  define END_PUSH_CONSTANTS(Name) \
    ;                              \
    [[vk::push_constant]] EZ_PP_CONCAT(Name, _PushConstants) Name;
#  define GET_PUSH_CONSTANT(Name, Constant) Name.Constant

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
  return DepthTexture.SampleLevel(DepthSampler, float3(SamplePos, ArrayIndex), MipLevel);
}
#endif
