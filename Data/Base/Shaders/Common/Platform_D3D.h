#pragma once

#define PLATFORM_DX11 EZ_OFF

#if defined(DX11_SM40_93) || defined(DX11_SM40) || defined(DX11_SM41) || defined(DX11_SM50)

#  undef PLATFORM_SHADER
#  define PLATFORM_SHADER EZ_ON

#  undef PLATFORM_DX11
#  define PLATFORM_DX11 EZ_ON

// DX11 does not support push constants, so we just emulate them via a normal constant buffer.
#  define BEGIN_PUSH_CONSTANTS(Name) cbuffer Name
#  define END_PUSH_CONSTANTS(Name) ;
#  define GET_PUSH_CONSTANT(Name, Constant) Constant

#  define BEGIN_MATERIAL_CONSTANTS cbuffer materialData
#  define END_MATERIAL_CONSTANTS ;
#  define GetMaterialData(x) x

#  define SUPPORTS_TEXEL_BUFFER EZ_ON
#  define SUPPORTS_MSAA_ARRAYS EZ_ON

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

float4 ezSampleLevel_PointClampBorder(Texture2DArray DepthTexture, SamplerState DepthSampler, float2 SamplePos, int ArrayIndex, int MipLevel, float4 BorderColor)
{
  return DepthTexture.SampleLevel(DepthSampler, float3(SamplePos, ArrayIndex), MipLevel);
}
#endif
