#include "CommonConstants.h"

struct VS_IN
{
  float3 pos : POSITION;
};

struct VS_OUT
{
  float4 pos : SV_Position;
  float4 color : COLOR;
  float3 normal : NORMAL;
  float2 texcoord0 : TEXCOORD0;
};

typedef VS_OUT PS_IN;
