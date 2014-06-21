#if defined(GL3) || defined(GL4)

#version 430

#endif

#if defined(DX11_SM40_93) || defined(DX11_SM40) || defined(DX11_SM41) || defined(DX11_SM50)


cbuffer PerFrame : register(b0)
{
  float time;
};

cbuffer PerObject : register(b1)
{
  float4x4 mvp : packoffset(c0);
};

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