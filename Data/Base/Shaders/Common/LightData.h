#pragma once

#include "Platforms.h"
#include "ConstantBufferMacros.h"

#define LIGHT_TYPE_POINT 0
#define LIGHT_TYPE_SPOT 1
#define LIGHT_TYPE_DIR 2
#define LIGHT_TYPE_MASK 0x7

#define LIGHT_HAS_SHADOWS (1 << 3)
#define LIGHT_HAS_PROJECTOR (1 << 4)

struct EZ_ALIGN_16(ezPerLightData)
{
  UINT1(colorAndType);
  FLOAT1(intensity);
  UINT1(direction); // 10 bits fixed point per axis
  UINT1(shadowDataIndex);

  FLOAT3(position);
  FLOAT1(invSqrAttRadius);

  UINT1(spotParams); // scale and offset as 16 bit floats
  UINT1(projectorAtlasOffset); // xy as 16 bit floats
  UINT1(projectorAtlasScale); // xy as 16 bit floats

  UINT1(reserved);
};

#if EZ_ENABLED(PLATFORM_DX11)
  StructuredBuffer<ezPerLightData> perLightData;
#else
  EZ_CHECK_AT_COMPILETIME(sizeof(ezPerLightData) == 48);
#endif

CONSTANT_BUFFER(ezClusteredDataConstants, 3)
{
  FLOAT1(DepthSliceScale);
  FLOAT1(DepthSliceBias);
  FLOAT2(InvTileSize);
  
  UINT1(NumLights);
  UINT3(Padding);

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
  StructuredBuffer<ezPerClusterData> perClusterData;
  StructuredBuffer<uint> clusterItemList;
#endif
