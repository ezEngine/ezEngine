[PLATFORMS] 
ALL

[PERMUTATIONS]

COLORED

[VERTEXSHADER]

#include "Common.inc"

#ifdef PLATFORM_DX11

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

#ifdef PLATFORM_DX11

CONSTANT_BUFFER(ColorBuffer, 2)
{
  COLOR(CustomColor);
};


Texture2D TexDiffuse;
SamplerState TexDiffuseSampler;

float4 main(PS_IN Input) : SV_Target
{
#if COLORED
  //return TexDiffuse.Sample(TexDiffuseSampler, Input.norm.xz * 4);
  return AmbientColor;//float4(1.0, 1.0, 0.0, 1.0);
#else
  return CustomColor;
#endif
}

#endif

