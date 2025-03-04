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
// VERTEX_SHADER_RENDER_TARGET_ARRAY_INDEX

struct VS_IN
{
  float3 Position : POSITION;

#if defined(USE_NORMAL)
  float3 Normal : NORMAL;
#endif

#if defined(USE_TANGENT)
  float4 Tangent : TANGENT;
#endif

#if defined(USE_TEXCOORD0)
  float2 TexCoord0 : TEXCOORD0;

#  if defined(USE_TEXCOORD1)
  float2 TexCoord1 : TEXCOORD1;
#  endif
#endif

#if defined(USE_COLOR0)
  float4 Color0 : COLOR0;

#  if defined(USE_COLOR1)
  float4 Color1 : COLOR1;
#  endif
#endif

#if defined(USE_SKINNING)
  float4 BoneWeights : BONEWEIGHTS0;
  uint4 BoneIndices : BONEINDICES0;
#endif

  uint InstanceID : SV_InstanceID;
  uint VertexID : SV_VertexID;
};

#if defined(VERTEX_SHADER) || defined(HULL_SHADER) || defined(DOMAIN_SHADER)
#  if defined(CAMERA_MODE)
#    if CAMERA_MODE == CAMERA_MODE_STEREO
#      if VERTEX_SHADER_RENDER_TARGET_ARRAY_INDEX == TRUE
#        define RENDER_TARGET_ARRAY_INDEX
#      endif
#    endif
#  endif
#  define STAGE_TEMPLATE VS_OUT
#  include <Shaders/Materials/MaterialInterpolatorTemplate.h>
#  undef STAGE_TEMPLATE

#elif defined(GEOMETRY_SHADER)
#  if defined(CAMERA_MODE)
#    if CAMERA_MODE == CAMERA_MODE_STEREO
#      if VERTEX_SHADER_RENDER_TARGET_ARRAY_INDEX == FALSE
#        define STAGE_TEMPLATE VS_OUT
#        include <Shaders/Materials/MaterialInterpolatorTemplate.h>
#        undef STAGE_TEMPLATE

#        define RENDER_TARGET_ARRAY_INDEX
#        define STAGE_TEMPLATE GS_OUT
#        include <Shaders/Materials/MaterialInterpolatorTemplate.h>
#        undef STAGE_TEMPLATE
#      endif
#    endif
#  endif

#elif defined(PIXEL_SHADER)
#  if defined(CAMERA_MODE)
#    if CAMERA_MODE == CAMERA_MODE_STEREO
#      define RENDER_TARGET_ARRAY_INDEX
#    endif
#  endif
#  define STAGE_TEMPLATE PS_IN
#  include <Shaders/Materials/MaterialInterpolatorTemplate.h>
#  undef STAGE_TEMPLATE
#endif

// typedef VS_OUT PS_IN;
