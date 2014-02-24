
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

VS_OUT vs_main(VS_IN Input)
{
  VS_OUT RetVal;
  RetVal.pos = mul(mvp, float4(Input.pos, 1.0f));
  RetVal.norm = Input.norm; // TODO
  RetVal.tex0 = Input.tex0 * 2.5f;

  return RetVal;
}

float4 ps_main(PS_IN Input) : SV_Target
{
  return float4(Input.tex0, 0.0f, 1.0f);
  //return float4(TileTex.Sample(TileSampler, Input.tex0).rgba);
  //return float4(Input.norm, 1.0f);
}
