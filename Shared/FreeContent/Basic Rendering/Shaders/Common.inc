#include <Shaders/Common/GlobalConstants.h>

CONSTANT_BUFFER(PerObject, 1)
{
  MAT4(mvp);
};

#ifdef PLATFORM_DX11

Texture2D TileTex : register(t0);
SamplerState TileSampler : register(s0);

struct VS_IN
{
  float3 pos : POSITION;
  float3 norm : NORMAL;
  float2 tex0 : TEXCOORD0;
};

struct VS_OUT
{
  float4 pos : SV_Position;
  float3 norm : NORMAL;
  float2 tex0 : TEXCOORD0;
};

typedef VS_OUT PS_IN;

#endif



