#pragma once

#include "Platforms.h"
#include "ConstantBufferMacros.h"

#define LIGHT_TYPE_POINT 0
#define LIGHT_TYPE_SPOT 1
#define LIGHT_TYPE_DIR 2

struct EZ_ALIGN_16(ezPerLightData)
{
  UINT1(colorAndType);
  FLOAT1(intensity);
  UINT1(direction); // 10 bits fixed point per axis
  UINT1(shadowDataOffset);

  FLOAT3(position);
  FLOAT1(invSqrAttRadius);

  UINT1(spotParams); // scale and offset as 16 bit floats
  UINT1(projectorAtlasOffset); // xy as 16 bit floats
  UINT1(projectorAtlasScale); // xy as 16 bit floats

  UINT1(reserved);
};

#if EZ_ENABLED(PLATFORM_DX11)
  StructuredBuffer<ezPerLightData> perLightDataBuffer;
#else
  EZ_CHECK_AT_COMPILETIME(sizeof(ezPerLightData) == 48);
#endif

struct EZ_ALIGN_16(ezPointShadowData)
{
  FLOAT4(shadowParams); // x = slope bias, y = constant bias, z = penumbra size in texel, w = fadeout
  MAT4(worldToLightMatrix)[6];
};

struct EZ_ALIGN_16(ezSpotShadowData)
{
  FLOAT4(shadowParams); // x = slope bias, y = constant bias, z = penumbra size in texel, w = fadeout
  MAT4(worldToLightMatrix);
};

#define GET_SHADOW_PARAMS_OFFSET(baseOffset) (baseOffset + 0)
#define GET_WORLD_TO_LIGHT_MATRIX_OFFSET(baseOffset, index) (baseOffset + 1 + 4 * index)

#if EZ_ENABLED(PLATFORM_DX11)
  StructuredBuffer<float4> shadowDataBuffer;
#endif

CONSTANT_BUFFER(ezClusteredDataConstants, 3)
{
  FLOAT1(DepthSliceScale);
  FLOAT1(DepthSliceBias);
  FLOAT2(InvTileSize);

  UINT1(NumLights);
  FLOAT1(ShadowTexelSize);
  UINT2(Padding);

  COLOR4F(AmbientTopColor);
  COLOR4F(AmbientBottomColor);
};

#define NUM_CLUSTERS_X 16
#define NUM_CLUSTERS_Y 8
#define NUM_CLUSTERS_Z 24
#define NUM_CLUSTERS_XY (NUM_CLUSTERS_X * NUM_CLUSTERS_Y)
#define NUM_CLUSTERS (NUM_CLUSTERS_X * NUM_CLUSTERS_Y * NUM_CLUSTERS_Z)

struct ezPerClusterData
{
  UINT1(offset);
  UINT1(counts);
};

#if EZ_ENABLED(PLATFORM_DX11)
  StructuredBuffer<ezPerClusterData> perClusterDataBuffer;
  StructuredBuffer<uint> clusterItemBuffer;
#endif
