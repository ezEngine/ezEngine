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

float4 main(PS_IN Input) : SV_Target
{
#if COLORED
  return float4(0.0f, (COLORVALUE / 255.0) * Input.tex0, 1.0f);
#else
  return float4(0.0f, 0.0f, COLORVALUE / 255.0, 1.0f);
#endif
  //return float4(TileTex.Sample(TileSampler, Input.tex0).rgba);
  //return float4(Input.norm, 1.0f);
}

#endif

