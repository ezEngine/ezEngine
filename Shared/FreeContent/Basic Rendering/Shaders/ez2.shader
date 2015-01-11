[PLATFORMS] 
ALL
!DX11_SM40_93

[PERMUTATIONS]

COLORED
COLORVALUE

[VERTEXSHADER]

#include "Common.inc"

#if defined(DX11_SM40_93) || defined(DX11_SM40) || defined(DX11_SM41) || defined(DX11_SM50)

VS_OUT main(VS_IN Input)
{
  VS_OUT RetVal;
  RetVal.pos = mul(mvp, float4(Input.pos, 1.0f));
  RetVal.norm = Input.norm; // TODO
  RetVal.tex0 = Input.tex0 * 2.5f;

  return RetVal;
}

#endif


[PIXELSHADER]

#include "Common.inc"

#if defined(DX11_SM40_93) || defined(DX11_SM40) || defined(DX11_SM41) || defined(DX11_SM50)

cbuffer ColorBuffer
{
  float4 CustomColor;
};


Texture2D TexDiffuse;
SamplerState TexDiffuseSampler;

float4 main(PS_IN Input) : SV_Target
{
#if COLORED
  //return TexDiffuse.Sample(TexDiffuseSampler, Input.norm.xz * 4);
  return float4(1.0, 1.0, 0.0, 1.0);
#else
  return CustomColor;
#endif
}

#endif

