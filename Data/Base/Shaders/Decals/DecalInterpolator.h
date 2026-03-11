#pragma once

#include <Shaders/Common/Common.h>

struct VS_IN
{
  float3 Position : POSITION;
};

struct VS_OUT
{
  float4 Position : SV_Position;

  float2 TexCoord0 : TEXCOORD0;
};

typedef VS_OUT PS_IN;
