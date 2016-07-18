#pragma once

#if EZ_ENABLED(PLATFORM_DX11)

// HLSL

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
#define MAT3(Name) float3x3 Name
#define MAT4(Name) float4x4 Name
#define TRANSFORM(Name) Transform Name
#define COLOR(Name) float4 Name

#elif EZ_ENABLED(PLATFORM_OPENGL)

// GLSL

#define CONSTANT_BUFFER(Name, Slot) layout(shared) Name
#define STRUCTURED_BUFFER(Name, Type)
#define FLOAT1(Name) float Name
#define FLOAT2(Name) vec2 Name
#define FLOAT3(Name) vec3 Name
#define FLOAT4(Name) vec4 Name
#define INT1(Name) int Name
#define INT2(Name) ivec2 Name
#define INT3(Name) ivec3 Name
#define INT4(Name) ivec4 Name
#define MAT3(Name) mat3 Name
#define MAT4(Name) mat4 Name
#define COLOR(Name) vec4 Name

#else

// C++

#include <RendererCore/Shader/Types.h>

#define CONSTANT_BUFFER(Name, Slot) EZ_ALIGN_16(struct) Name
#define STRUCTURED_BUFFER(Name, Type)
#define FLOAT1(Name) float Name
#define FLOAT2(Name) ezVec2 Name
#define FLOAT3(Name) ezVec3 Name
#define FLOAT4(Name) ezVec4 Name
#define INT1(Name) int Name
#define INT2(Name) ezVec2I32 Name
#define INT3(Name) ezVec3I32 Name
#define INT4(Name) ezVec4I32 Name
#define MAT3(Name) ezShaderMat3 Name
#define MAT4(Name) ezMat4 Name
#define TRANSFORM(Name) ezShaderTransform Name
#define COLOR(Name) ezColor Name

#endif

