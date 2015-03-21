#include <Shaders/Common/GlobalConstants.h>

CONSTANT_BUFFER(MaterialCB, 0)
{
  MAT4(ModelMatrix);
  MAT4(ViewProjectionMatrix);
};

CONSTANT_BUFFER(PerObject, 1)
{
  MAT4(world);
  MAT4(mvp);
};

#ifdef PLATFORM_DX11

struct VS_IN
{
  float3 Position : POSITION;
  float3 Normal : NORMAL;
  float2 TexCoord0 : TEXCOORD0;
};

struct VS_OUT
{
  float4 Position : SV_Position;
  float3 Normal : NORMAL;
  float2 TexCoord0 : TEXCOORD0;
};

typedef VS_OUT PS_IN;

#endif



