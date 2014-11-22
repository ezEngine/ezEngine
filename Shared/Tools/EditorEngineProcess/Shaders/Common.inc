cbuffer PerFrame : register(b0)
{
  float time;
};

cbuffer PerObject : register(b1)
{
  float4x4 mvp : packoffset(c0);
};

struct VS_IN
{
  float3 pos : POSITION;
};

struct VS_OUT
{
  float4 pos : SV_Position;
};

typedef VS_OUT PS_IN;

