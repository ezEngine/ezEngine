[PLATFORMS] 
ALL

[PERMUTATIONS]


[VERTEXSHADER]

#include "Common.inc"

VS_OUT main(VS_IN input)
{
  VS_OUT output;
  output.pos = mul(mvp, float4(input.pos, 1.0));
  output.color = input.color;

  return output;
}

[PIXELSHADER]

#include "Common.inc"

float4 main(PS_IN input) : SV_Target
{
  return input.color / 4.0 * 2.0;
}

