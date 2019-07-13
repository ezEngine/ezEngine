#pragma once

#include <Shaders/Common/Common.h>

// defines:
// USE_WORLDPOS
// USE_NORMAL
// USE_TANGENT
// USE_TEXCOORD0
// USE_TEXCOORD1
// USE_COLOR0
// USE_COLOR1
// USE_SKINNING
// USE_DEBUG_INTERPOLATOR
// CUSTOM_INTERPOLATOR

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

  #if defined(USE_COLOR0)
    float4 Color0 : COLOR0;
    
    #if defined(USE_COLOR1)
      float4 Color1 : COLOR1;
    #endif
  #endif

  #if defined(USE_SKINNING)
    float4 BoneWeights : BONEWEIGHTS0;
    uint4 BoneIndices : BONEINDICES0;
  #endif

  uint InstanceID : SV_InstanceID;
  uint VertexID : SV_VertexID;
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
    float2 TexCoord0 : TEXCOORD0;

    #if defined(USE_TEXCOORD1)
      float2 TexCoord1 : TEXCOORD1;
    #endif
  #endif

  #if defined(USE_COLOR0)
    float4 Color0 : COLOR0;
    
    #if defined(USE_COLOR1)
      float4 Color1 : COLOR1;
    #endif
  #endif
  
  #if defined(USE_DEBUG_INTERPOLATOR)
    float4 DebugInterpolator : DEBUG_INTERPOLATOR;
  #endif

  #if defined(CUSTOM_INTERPOLATOR)
    CUSTOM_INTERPOLATOR
  #endif

  // If CAMERA_MODE is CAMERA_MODE_STEREO, every even instance is for the left eye and every odd is for the right eye.
    uint InstanceID : SV_InstanceID;

  #if defined(PIXEL_SHADER) && defined(CAMERA_MODE)
    #if CAMERA_MODE == CAMERA_MODE_STEREO
      uint RenderTargetArrayIndex : SV_RenderTargetArrayIndex;
    #endif
  #endif

  #if defined(PIXEL_SHADER) && defined(TWO_SIDED)
    #if TWO_SIDED == TRUE
      uint FrontFace : SV_IsFrontFace;
    #endif
  #endif
};

typedef VS_OUT PS_IN;
