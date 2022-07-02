#pragma once

#include "StandardMacros.h"

#define PLATFORM_SHADER EZ_OFF
#define PLATFORM_DX11 EZ_OFF
#define PLATFORM_VULKAN EZ_OFF

#if defined(DX11_SM40_93) || defined(DX11_SM40) || defined(DX11_SM41) || defined(DX11_SM50)

  #undef PLATFORM_SHADER
  #define PLATFORM_SHADER EZ_ON

  #undef PLATFORM_DX11
  #define PLATFORM_DX11 EZ_ON

float ezEvaluateAttributeAtSample(float Attribute, uint SampleIndex)
{
  return EvaluateAttributeAtSample(Attribute, SampleIndex);
}
float2 ezEvaluateAttributeAtSample(float2 Attribute, uint SampleIndex)
{
  return EvaluateAttributeAtSample(Attribute, SampleIndex);
}
float3 ezEvaluateAttributeAtSample(float3 Attribute, uint SampleIndex)
{
  return EvaluateAttributeAtSample(Attribute, SampleIndex);
}
float4 ezEvaluateAttributeAtSample(float4 Attribute, uint SampleIndex)
{
  return EvaluateAttributeAtSample(Attribute, SampleIndex);
}
#endif

#if defined(VULKAN)

  #undef PLATFORM_SHADER
  #define PLATFORM_SHADER EZ_ON

  #undef PLATFORM_VULKAN
  #define PLATFORM_VULKAN EZ_ON

// Workaround for error: EvaluateAttributeAtSample intrinsic function unimplemented
// See https://github.com/microsoft/DirectXShaderCompiler/issues/3649
float ezEvaluateAttributeAtSample(float Attribute, uint SampleIndex)
{
  return Attribute;
}
float2 ezEvaluateAttributeAtSample(float2 Attribute, uint SampleIndex)
{
  return Attribute;
}
float3 ezEvaluateAttributeAtSample(float3 Attribute, uint SampleIndex)
{
  return Attribute;
}
float4 ezEvaluateAttributeAtSample(float4 Attribute, uint SampleIndex)
{
  return Attribute;
}

#endif
