#pragma once

// This geometry shader is a pass-through that leaves the geometry unmodified and sets the render target array index.

#if CAMERA_MODE == CAMERA_MODE_STEREO

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
    float2 TexCoord0 : TEXCOORD0;

    #if defined(USE_TEXCOORD1)
      float2 TexCoord1 : TEXCOORD1;
    #endif
  #endif

	#if defined(USE_COLOR)
		float4 Color : COLOR;
	#endif

	// If CAMERA_MODE is CAMERA_MODE_STEREO, every even instance is for the left eye and every odd is for the right eye.
	uint InstanceID : SV_InstanceID;
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
          output.TexCoord0 = input[i].TexCoord0;
        #endif
        
        #if defined(USE_TEXCOORD1)
          output.TexCoord1 = input[i].TexCoord1;
        #endif

        #if defined(USE_COLOR)
          output.Color = input[i].Color;
        #endif

        output.InstanceID = input[i].InstanceID;
        output.RenderTargetArrayIndex = input[i].InstanceID % 2;

        outStream.Append(output);
    }
}

#endif