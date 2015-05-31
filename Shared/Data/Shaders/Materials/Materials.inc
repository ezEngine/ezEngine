#include <Shaders/Common/GlobalConstants.h>

CONSTANT_BUFFER(MaterialCB, 0)
{
  MAT4(ModelMatrix);
  MAT4(ViewProjectionMatrix);
  FLOAT4(AddColor);
};

CONSTANT_BUFFER(PerObject, 1)
{
  MAT4(world);
  MAT4(mvp);
  INT1(GameObjectID);
  INT1(PartIndex);
};

#ifdef PLATFORM_DX11

struct VS_IN
{
  float3 Position : POSITION;
  float3 Normal : NORMAL;
  float2 TexCoord0 : TEXCOORD0;
};

struct VS_IN_COLORED
{
  float3 Position : POSITION;
  float4 Color : COLOR;
};

struct VS_OUT
{
  float4 Position : SV_Position;
  float3 Normal : NORMAL;
  float2 TexCoord0 : TEXCOORD0;
};

struct VS_OUT_COLORED
{
  float4 Position : SV_Position;
  float4 Color : COLOR;
};

typedef VS_OUT PS_IN;
typedef VS_OUT_COLORED PS_IN_COLORED;

#endif



