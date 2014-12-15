[PLATFORMS] 
ALL

[PERMUTATIONS]

[VERTEXSHADER]

#include "Common.inc"

VS_OUT main(VS_IN Input)
{
  VS_OUT RetVal;
  RetVal.pos = mul(mvp, float4(Input.pos, 1.0));
  RetVal.texcoord0 = Input.pos.xy + 0.5;
  RetVal.normal = Input.pos.xyz;

  return RetVal;
}

[PIXELSHADER]

#include "Common.inc"


TextureCube TexDiffuse : register(t0);
SamplerState TexDiffuseSampler : register(s0);

float4 main(PS_IN Input) : SV_Target
{
  //return float4(normalize(Input.normal) * 0.5 + 0.5, 1);
  return TexDiffuse.Sample(TexDiffuseSampler, normalize(Input.normal));
}
