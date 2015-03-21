[PLATFORMS] 
ALL

[PERMUTATIONS]


[VERTEXSHADER]

#include "Common.inc"

#ifdef PLATFORM_DX11

VS_OUT main(VS_IN Input)
{
  VS_OUT Output;
  Output.Position = mul(mvp, float4(Input.Position, 1.0f));
  Output.Normal = Input.Normal;
  Output.TexCoord0 = Input.TexCoord0;

  return Output;
}

#endif


[PIXELSHADER]

#include "Common.inc"

#ifdef PLATFORM_DX11

Texture2D TexDiffuse;
SamplerState TexDiffuseSampler;

float4 main(PS_IN Input) : SV_Target
{
  return TexDiffuse.Sample(TexDiffuseSampler, Input.TexCoord0);
}

#endif

