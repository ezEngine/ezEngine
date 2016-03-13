cbuffer PerFrame : register(b0)
{
  float time;
};

cbuffer PerObject : register(b1)
{
  float4x4 mvp : packoffset(c0);
  float4 ObjectColor : packoffset(c4);
};

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
