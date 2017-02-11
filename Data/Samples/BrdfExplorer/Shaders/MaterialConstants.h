#include <Shaders/Common/GlobalConstants.h>

CONSTANT_BUFFER(ezMaterialConstants, 1)
{
  MAT4(ViewMatrix);
  FLOAT1(Roughness);
  FLOAT1(ExposureBias);
  COLOR4F(DiffuseColor);
  COLOR4F(SpecularColor);
  BOOL(InverseTonemap);
};

#ifdef PLATFORM_DX11

struct VS_IN
{
  float3 Position : POSITION;
  float3 Normal : NORMAL;
};

struct VS_OUT
{
  float4 Position : SV_Position;
  float3 Normal : NORMAL;
  float3 WorldPosition : TEXCOORD0;
};

typedef VS_OUT PS_IN;

#endif