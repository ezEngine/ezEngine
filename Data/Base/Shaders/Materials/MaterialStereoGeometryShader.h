#pragma once

// This geometry shader is a pass-through that leaves the geometry unmodified and sets the render target array index.

#if CAMERA_STEREO == TRUE

#define USE_WORLDPOS

#include "MaterialInterpolator.h"

#if defined(CUSTOM_INTERPOLATOR)
  #error "Stereo geometry shader does not support custom interpolators!"
#endif

// TODO: Can we do this without copy pasting the VS_OUT struct from MaterialInterpolator?
// DX doesn't like it if we pretend that VS_OUT had 
struct GS_OUT
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

	// If CAMERA_STEREO is true, every even instance is for the left eye and every odd is for the right eye.
	uint InstanceOffset : INSTANCEOFFSET;
	uint RenderTargetArrayIndex : SV_RenderTargetArrayIndex;
};


[maxvertexcount(3)]
void main(triangle VS_OUT input[3], inout TriangleStream<GS_OUT> outStream)
{
    GS_OUT output;
    [unroll(3)] for (int i = 0; i < 3; ++i)
    {
        output.Position = input[i].Position;

        #if defined(USE_WORLDPOS)
          output.WorldPosition = input[i].WorldPosition;
        #endif

        #if defined(USE_NORMAL)
          output.Normal = input[i].Normal;
        #endif

        #if defined(USE_TANGENT)
          output.Tangent = input[i].Tangent;
          output.BiTangent = input[i].BiTangent;
        #endif

        #if defined(USE_TEXCOORD0)
          output.TexCoords = input[i].TexCoords;
        #endif

        #if defined(USE_COLOR)
          output.Color = input[i].Color;
        #endif

        output.InstanceOffset = input[i].InstanceOffset;
        output.RenderTargetArrayIndex = input[i].InstanceOffset % 2;

        outStream.Append(output);
    }
}

#endif