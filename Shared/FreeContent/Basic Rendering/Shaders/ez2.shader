[PLATFORMS] 
ALL
!DX11_SM40_93

[PERMUTATIONS]

COLORED
COLORVALUE

[VERTEXSHADER]

#include "Common.inc"

#if defined(GL3) || defined(GL4)

layout(binding = 0, shared) uniform PerFrame
{
  float time;
};

layout(binding = 1, shared) uniform PerObject
{
  mat4 mvp;
};

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNorm;
layout(location = 2) in vec2 inTex0;

layout(location = 0) out vec3 outNorm;
layout(location = 1) out vec2 outTex0;

void main()
{
  gl_Position = mvp * vec4(inPos, 1.0);
  outNorm = inNorm;
  outTex0 = inTex0 * 2.5;
}

#endif

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

#if defined(GL3) || defined(GL4)

layout(location = 0) in vec3 inNorm;
layout(location = 1) in vec2 inTex0;


layout(location = 0, index = 0) out vec4 outFragColor;

void main()
{
#if COLORED
  outFragColor = vec4(0.0, COLORVALUE / 255.0, 0.0, 1.0);
#else
  outFragColor = vec4((COLORVALUE / 255.0) * inTex0, 0.0, 1.0);
#endif
}

#endif

#if defined(DX11_SM40_93) || defined(DX11_SM40) || defined(DX11_SM41) || defined(DX11_SM50)

Texture2D TexDiffuse;
SamplerState TexDiffuseSampler;

float4 main(PS_IN Input) : SV_Target
{
  

#if COLORED
  //return TexDiffuse.Sample(TexDiffuseSampler, Input.norm.xz * 4);
  return float4(0.0, 1.0, 0.0, 1.0);
#else
  return float4(COLORVALUE / 255.0, some_color, 0.0f, 1.0f);
#endif
}

#endif

