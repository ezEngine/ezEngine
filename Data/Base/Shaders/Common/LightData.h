#pragma once

#if SHADING_QUALITY != SHADING_QUALITY_NORMAL
#error "Functions in LightData.h are only for NORMAL shading quality. Todo: Split up file"
#endif

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

struct EZ_ALIGN_16(ezDirShadowData)
{
  FLOAT4(shadowParams); // x = slope bias, y = constant bias, z = penumbra size in texel, w = num cascades
  MAT4(worldToLightMatrix);
  FLOAT4(shadowParams2); // x = cascade border threshold, y = xy dither multiplier, z = z dither multiplier, w = penumbra size increment
  FLOAT4(fadeOutParams); // x = xy fadeout scale, y = xy fadeout offset, z = z fadeout scale, w = z fadeout offset
  FLOAT4(cascadeScaleOffset)[6]; // interleaved, maxNumCascades - 1 since first cascade has identity scale and offset
  FLOAT4(atlasScaleOffset)[4];

};

#define GET_SHADOW_PARAMS_INDEX(baseOffset) (baseOffset + 0)
#define GET_WORLD_TO_LIGHT_MATRIX_INDEX(baseOffset, index) (baseOffset + 1 + 4 * (index))
#define GET_SHADOW_PARAMS2_INDEX(baseOffset) (baseOffset + 5)
#define GET_FADE_OUT_PARAMS_INDEX(baseOffset) (baseOffset + 6)
#define GET_CASCADE_SCALE_INDEX(baseOffset, index) (baseOffset + 7 + 2 * (index))
#define GET_CASCADE_OFFSET_INDEX(baseOffset, index) (baseOffset + 8 + 2 * (index))
#define GET_ATLAS_SCALE_OFFSET_INDEX(baseOffset, index) (baseOffset + 13 + (index))

#if EZ_ENABLED(PLATFORM_DX11)
  StructuredBuffer<float4> shadowDataBuffer;
#endif

#define DECAL_USE_NORMAL (1 << 0)
#define DECAL_USE_ORM (1 << 1)
#define DECAL_USE_EMISSIVE (1 << 2)
#define DECAL_BLEND_MODE_COLORIZE (1 << 7)
#define DECAL_WRAP_AROUND (1 << 8)
#define DECAL_MAP_NORMAL_TO_GEOMETRY (1 << 9)

struct EZ_ALIGN_16(ezPerDecalData)
{
  TRANSFORM(worldToDecalMatrix);

  UINT1(applyOnlyToId);
  UINT1(decalFlags);
  UINT1(angleFadeParams); // scale and offset as 16 bit floats
  UINT1(baseColor);

  UINT1(emissiveColorRG); // as 16 bit floats
  UINT1(emissiveColorBA); // as 16 bit floats

  UINT1(baseColorAtlasScale); // xy as 16 bit floats
  UINT1(baseColorAtlasOffset); // xy as 16 bit floats

  UINT1(normalAtlasScale); // xy as 16 bit floats
  UINT1(normalAtlasOffset); // xy as 16 bit floats

  UINT1(ormAtlasScale); // xy as 16 bit floats
  UINT1(ormAtlasOffset); // xy as 16 bit floats
};

#if EZ_ENABLED(PLATFORM_DX11)
  StructuredBuffer<ezPerDecalData> perDecalDataBuffer;
#else // C++
  EZ_CHECK_AT_COMPILETIME(sizeof(ezPerDecalData) == 96);
#endif

CONSTANT_BUFFER(ezClusteredDataConstants, 3)
{
  FLOAT1(DepthSliceScale);
  FLOAT1(DepthSliceBias);
  FLOAT2(InvTileSize);

  UINT1(NumLights);
  UINT1(NumDecals);
  UINT1(Padding);
  
  UINT1(SkyIrradianceIndex);  

  FLOAT1(FogHeight);
  FLOAT1(FogHeightFalloff);
  FLOAT1(FogDensityAtCameraPos);
  FLOAT1(FogDensity);
  COLOR4F(FogColor);
};

#define NUM_CLUSTERS_X 16
#define NUM_CLUSTERS_Y 8
#define NUM_CLUSTERS_Z 24
#define NUM_CLUSTERS_XY (NUM_CLUSTERS_X * NUM_CLUSTERS_Y)
#define NUM_CLUSTERS (NUM_CLUSTERS_X * NUM_CLUSTERS_Y * NUM_CLUSTERS_Z)

#define LIGHT_BITMASK 0x3FF
#define DECAL_SHIFT 10
#define DECAL_BITMASK 0x3FF

#define GET_LIGHT_INDEX(index) (index & LIGHT_BITMASK)
#define GET_DECAL_INDEX(index) ((index >> DECAL_SHIFT) & DECAL_BITMASK)

struct ezPerClusterData
{
  UINT1(offset);
  UINT1(counts);
};

#if EZ_ENABLED(PLATFORM_DX11)
  StructuredBuffer<ezPerClusterData> perClusterDataBuffer;
  StructuredBuffer<uint> clusterItemBuffer;
#endif
