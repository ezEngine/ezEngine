#include "CommonConstants.h"

struct VS_IN
{
  float3 pos : POSITION;
  float2 texcoord0 : TEXCOORD0;
};

struct VS_OUT
{
  float4 pos : SV_Position;
  float2 texcoord0 : TEXCOORD0;
};

typedef VS_OUT PS_IN;
