[PLATFORMS] 
ALL
!DX11_SM40_93

[VERTEXSHADER]

#include "Common.inc"

VS_OUT main(VS_IN In)
{
  VS_OUT Out;
  Out.pos = mul(mvp, float4(In.pos, 1.0f));
  Out.norm = mul((float3x3)world, In.norm);
  Out.tex0 = In.tex0;

  return Out;
}


[PIXELSHADER]

#include "Common.inc"

float4 main(PS_IN In) : SV_Target
{
  float3 albedo = float3(0.5, 0.55, 0.6);
  float3 lightDir = float3(0.4, 0.5, 1.0);
  return float4(albedo * saturate(dot(In.norm, normalize(lightDir))), 1.0);
}


