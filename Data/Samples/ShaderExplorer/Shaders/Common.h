#include <Shaders/Common/GlobalConstants.h>

CONSTANT_BUFFER(ezMaterialConstants, 0)
{
  MAT4(ViewMatrix);
};

#ifdef PLATFORM_DX11

struct VS_IN
{
  float3 Position : POSITION;
};

struct VS_OUT
{
  float4 Position : SV_Position;
  float2 FragCoord : TEXCOORD0;
};

typedef VS_OUT PS_IN;

#endif



