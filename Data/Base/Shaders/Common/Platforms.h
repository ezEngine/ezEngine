#pragma once

#include "StandardMacros.h"

#ifndef PLATFORM_SHADER
#  define PLATFORM_SHADER EZ_OFF
#endif

#define PLATFORM_DX11 EZ_OFF
#define PLATFORM_VULKAN EZ_OFF

#if defined(DX11_SM40_93) || defined(DX11_SM40) || defined(DX11_SM41) || defined(DX11_SM50)

#  undef PLATFORM_SHADER
#  define PLATFORM_SHADER EZ_ON

#  undef PLATFORM_DX11
#  define PLATFORM_DX11 EZ_ON

// DX11 does not support push constants, so we just emulate them via a normal constant buffer.

#  define BEGIN_PUSH_CONSTANTS(Name) cbuffer Name
#  define END_PUSH_CONSTANTS(Name) ;
#  define GET_PUSH_CONSTANT(Name, Constant) Constant

float ezEvaluateAttributeAtSample(float Attribute, uint SampleIndex, uint NumMsaaSamples)
{
  return EvaluateAttributeAtSample(Attribute, SampleIndex);
}
float2 ezEvaluateAttributeAtSample(float2 Attribute, uint SampleIndex, uint NumMsaaSamples)
{
  return EvaluateAttributeAtSample(Attribute, SampleIndex);
}
float3 ezEvaluateAttributeAtSample(float3 Attribute, uint SampleIndex, uint NumMsaaSamples)
{
  return EvaluateAttributeAtSample(Attribute, SampleIndex);
}
float4 ezEvaluateAttributeAtSample(float4 Attribute, uint SampleIndex, uint NumMsaaSamples)
{
  return EvaluateAttributeAtSample(Attribute, SampleIndex);
}

// Custom implementation of HLSL2021 intrinsic function which is not available in SM50 or lower.
// https://github.com/microsoft/DirectXShaderCompiler/wiki/HLSL-2021#logical-operation-short-circuiting-for-scalars
float2 select(bool2 condition, float2 yes, float2 no)
{
  return float2(condition.x ? yes.x : no.x, condition.y ? yes.y : no.y);
}

float3 select(bool3 condition, float3 yes, float3 no)
{
  return float3(condition.x ? yes.x : no.x, condition.y ? yes.y : no.y, condition.z ? yes.z : no.z);
}

float4 select(bool4 condition, float4 yes, float4 no)
{
  return float4(condition.x ? yes.x : no.x, condition.y ? yes.y : no.y, condition.z ? yes.z : no.z, condition.w ? yes.w : no.w);
}

#endif

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

#endif
