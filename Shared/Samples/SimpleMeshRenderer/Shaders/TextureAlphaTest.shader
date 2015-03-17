[PLATFORMS] 
ALL

[PERMUTATIONS]


[VERTEXSHADER]

#include "Common.inc"

#ifdef PLATFORM_DX11

VS_OUT main(VS_IN Input)
{
  VS_OUT RetVal;
  RetVal.Position = mul(mvp, float4(Input.Position, 1.0f));
  RetVal.TexCoord0 = Input.TexCoord0;

  return RetVal;
}

#endif


[PIXELSHADER]

#include "Common.inc"

#ifdef PLATFORM_DX11

Texture2D TexDiffuse;
SamplerState TexDiffuseSampler;

Texture2D TexAlphaMask;
SamplerState TexAlphaMaskSampler;

float4 main(PS_IN Input) : SV_Target
{
  float fMask = TexAlphaMask.Sample(TexAlphaMaskSampler, Input.TexCoord0).r;

  clip(fMask < 0.1f ? -1 : 1);
  return TexDiffuse.Sample(TexDiffuseSampler, Input.TexCoord0);
}

#endif

