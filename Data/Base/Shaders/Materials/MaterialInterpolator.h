#pragma once

#include <Shaders/Common/Common.h>

// defines:
// USE_WORLDPOS
// USE_NORMAL
// USE_TANGENT
// USE_TEXCOORD0
// USE_TEXCOORD1
// USE_COLOR

struct VS_IN
{
	float3 Position : POSITION;

	#if defined(USE_NORMAL)
		float3 Normal : NORMAL;
	#endif

	#if defined(USE_TANGENT)
		float3 Tangent : TANGENT;
	#endif

	#if defined(USE_TEXCOORD0)
		float2 TexCoord0 : TEXCOORD0;

		#if defined(USE_TEXCOORD1)
			float2 TexCoord1 : TEXCOORD1;
		#endif
	#endif

	#if defined(USE_COLOR)
		float4 Color : COLOR;
	#endif

	#if INSTANCING
		uint InstanceID : SV_InstanceID;
	#endif
};

struct VS_IN_COLORED
{
  float3 Position : POSITION;
  float4 Color : COLOR;
};

struct VS_OUT
{
	float4 Position : SV_Position;

	#if defined(USE_WORLDPOS)
		float3 WorldPosition : WORLDPOS;
	#endif

	#if defined(USE_NORMAL)
		float3 Normal : NORMAL;
	#endif

	#if defined(USE_TANGENT)
		float3 Tangent : TANGENT;
		float3 BiTangent : BITANGENT;
	#endif

	#if defined(USE_TEXCOORD0)
		#if defined(USE_TEXCOORD1)
			float4 TexCoords : TEXCOORD0;
		#else
			float2 TexCoords : TEXCOORD0;
		#endif
	#endif

	#if defined(USE_COLOR)
		float4 Color : COLOR;
	#endif

	#if defined(CUSTOM_INTERPOLATOR)
		CUSTOM_INTERPOLATOR
	#endif

	#if INSTANCING
		uint InstanceID : SV_InstanceID;
	#endif

	#if defined(PIXEL_SHADER) && TWO_SIDED == TRUE
		uint FrontFace : SV_IsFrontFace;
	#endif
};

struct VS_OUT_COLORED
{
  float4 Position : SV_Position;
  float4 Color : COLOR;
};

typedef VS_OUT PS_IN;
typedef VS_OUT_COLORED PS_IN_COLORED;
