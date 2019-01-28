#pragma once

#if EZ_ENABLED(PLATFORM_DX11)

// HLSL

#define EZ_ALIGN_16(decl) decl

struct Transform
{
	float4 r0;
	float4 r1;
	float4 r2;
};

float4x4 TransformToMatrix(Transform t)
{
	return float4x4(t.r0, t.r1, t.r2, float4(0, 0, 0, 1));
}

float4 TransformToPosition(Transform t)
{
	return float4(t.r0.w, t.r1.w, t.r2.w, 1);
}

float3x3 TransformToRotation(Transform t)
{
	return float3x3(t.r0.xyz, t.r1.xyz, t.r2.xyz);
}

#define CONSTANT_BUFFER(Name, Slot) cbuffer Name : register(b##Slot)
#define STRUCTURED_BUFFER(Name, Type) StructuredBuffer<Type> Name
#define FLOAT1(Name) float Name
#define FLOAT2(Name) float2 Name
#define FLOAT3(Name) float3 Name
#define FLOAT4(Name) float4 Name
#define INT1(Name) int Name
#define INT2(Name) int2 Name
#define INT3(Name) int3 Name
#define INT4(Name) int4 Name
#define UINT1(Name) uint Name
#define UINT2(Name) uint2 Name
#define UINT3(Name) uint3 Name
#define UINT4(Name) uint4 Name
#define MAT3(Name) float3x3 Name
#define MAT4(Name) float4x4 Name
#define TRANSFORM(Name) Transform Name
#define COLOR4F(Name) float4 Name
#define COLOR4UB(Name) uint Name
#define BOOL(Name) bool Name
#define PACKEDHALF2(Name1, Name2, CombinedName) uint CombinedName
#define PACKEDCOLOR4H(Name) uint EZ_CONCAT(Name,RG); uint EZ_CONCAT(Name,GB)

#define UNPACKHALF2(Name1, Name2, CombinedName) float Name1 = f16tof32(CombinedName); float Name2 = f16tof32(CombinedName >> 16)
#define UNPACKCOLOR4H(Name) RGBA16FToFloat4(EZ_CONCAT(Name,RG), EZ_CONCAT(Name,GB))

#else

// C++

#include <RendererCore/Shader/Types.h>
#include <Foundation/Basics/Platform/Common.h>

#define CONSTANT_BUFFER(Name, Slot) struct EZ_ALIGN_16(Name)
#define STRUCTURED_BUFFER(Name, Type)
#define FLOAT1(Name) float Name
#define FLOAT2(Name) ezVec2 Name
#define FLOAT3(Name) ezVec3 Name
#define FLOAT4(Name) ezVec4 Name
#define INT1(Name) int Name
#define INT2(Name) ezVec2I32 Name
#define INT3(Name) ezVec3I32 Name
#define INT4(Name) ezVec4I32 Name
#define UINT1(Name) ezUInt32 Name
#define UINT2(Name) ezVec2U32 Name
#define UINT3(Name) ezVec3U32 Name
#define UINT4(Name) ezVec4U32 Name
#define MAT3(Name) ezShaderMat3 Name
#define MAT4(Name) ezMat4 Name
#define TRANSFORM(Name) ezShaderTransform Name
#define COLOR4F(Name) ezColor Name
#define COLOR4UB(Name) ezColorGammaUB Name
#define BOOL(Name) ezShaderBool Name
#define PACKEDHALF2(Name1, Name2, CombinedName) ezFloat16 Name1; ezFloat16 Name2
#define PACKEDCOLOR4H(Name) ezColorLinear16f Name

#endif

