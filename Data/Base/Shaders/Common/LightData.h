#pragma once

// clang-format off

#if SHADING_QUALITY != SHADING_QUALITY_NORMAL
#error "Functions in LightData.h are only for NORMAL shading quality. Todo: Split up file"
#endif

#include "Platforms.h"
#include "ConstantBufferMacros.h"

#define LIGHT_TYPE_POINT 0
#define LIGHT_TYPE_SPOT 1
#define LIGHT_TYPE_DIR 2
#define LIGHT_TYPE_FILL_ADDITIVE 3
#define LIGHT_TYPE_FILL_MODULATE_INDIRECT 4

struct EZ_SHADER_STRUCT ezPerLightData
{
  UINT1(colorAndType);
  FLOAT1(intensity);
  UINT1(direction); // 10 bits fixed point per axis
  UINT1(shadowDataOffsetAndFadeOut); // 20 bits offset, 12 bits fade out

  FLOAT3(position);
  FLOAT1(invSqrAttRadius);

  UINT1(spotOrFillParams); // spot: scale and offset as 16 bit floats, fill: falloff exponent and directionality as 16 bit floats
  FLOAT1(specularMultiplier);
  UINT1(cookieParams0); // x: cookie index, y: cookie right dir z as 16 bit float
  UINT1(cookieParams1); // xy: cookie right dir xy as 16 bit floats
};

#if EZ_ENABLED(PLATFORM_SHADER)
  StructuredBuffer<ezPerLightData> perLightDataBuffer;

  uint GetLightType(ezPerLightData data)
  {
    return (data.colorAndType >> 24) & 0xFF;
  }

  float3 GetLightColor(ezPerLightData data)
  {
    return RGB8ToFloat3(data.colorAndType);
  }

  float3 GetLightDirection(ezPerLightData data)
  {
    return normalize(RGB10ToFloat3(data.direction) * 2.0 - 1.0);
  }
#else
  static_assert(sizeof(ezPerLightData) == 48);
#endif

struct EZ_SHADER_STRUCT ezPointShadowData
{
  FLOAT4(shadowParams); // x = slope bias, y = constant bias, z = penumbra size in texel, w = unused
  MAT4(worldToLightMatrix)[6];
};

struct EZ_SHADER_STRUCT ezSpotShadowData
{
  FLOAT4(shadowParams); // x = slope bias, y = constant bias, z = penumbra size in texel, w = unused
  MAT4(worldToLightMatrix);
};

struct EZ_SHADER_STRUCT ezDirShadowData
{
  FLOAT4(shadowParams); // x = slope bias, y = constant bias, z = penumbra size in texel, w = last cascade index
  MAT4(worldToLightMatrix);
  FLOAT4(shadowParams2); // x = cascade border threshold, y = xy dither multiplier, z = z dither multiplier, w = penumbra size increment
  FLOAT4(fadeOutParams); // x = xy fadeout scale offset (fp16), y = z fadeout scale offset (fp16), z = distance fadeout scale, w = distance fadeout offset
  FLOAT4(cascadeScaleOffset)[6]; // interleaved, maxNumCascades - 1 since first cascade has identity scale and offset
  FLOAT4(atlasScaleOffset)[4];

};

#define GET_SHADOW_PARAMS_INDEX(baseOffset) ((baseOffset) + 0)
#define GET_WORLD_TO_LIGHT_MATRIX_INDEX(baseOffset, index) ((baseOffset) + 1 + 4 * (index))
#define GET_SHADOW_PARAMS2_INDEX(baseOffset) ((baseOffset) + 5)
#define GET_FADE_OUT_PARAMS_INDEX(baseOffset) ((baseOffset) + 6)
#define GET_CASCADE_SCALE_INDEX(baseOffset, index) ((baseOffset) + 7 + 2 * (index))
#define GET_CASCADE_OFFSET_INDEX(baseOffset, index) ((baseOffset) + 8 + 2 * (index))
#define GET_ATLAS_SCALE_OFFSET_INDEX(baseOffset, index) ((baseOffset) + 13 + (index))

#if EZ_ENABLED(PLATFORM_SHADER)
  StructuredBuffer<float4> shadowDataBuffer;
#endif

#define DECAL_USE_NORMAL (1 << 0)
#define DECAL_USE_ORM (1 << 1)
#define DECAL_USE_EMISSIVE (1 << 2)
#define DECAL_BLEND_MODE_COLORIZE (1 << 7)
#define DECAL_WRAP_AROUND (1 << 8)
#define DECAL_MAP_NORMAL_TO_GEOMETRY (1 << 9)

struct EZ_SHADER_STRUCT ezPerDecalData
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

#if EZ_ENABLED(PLATFORM_SHADER)
  StructuredBuffer<ezPerDecalData> perDecalDataBuffer;
#else // C++
  static_assert(sizeof(ezPerDecalData) == 96);
#endif

struct ezPerDecalAtlasData
{
  UINT1(scale);
  UINT1(offset);
};

#if EZ_ENABLED(PLATFORM_SHADER)
  StructuredBuffer<ezPerDecalAtlasData> perDecalAtlasDataBuffer;
#else // C++
  static_assert(sizeof(ezPerDecalAtlasData) == 8);
#endif

#define REFLECTION_PROBE_IS_SPHERE (1 << 31)
#define REFLECTION_PROBE_IS_PROJECTED (1 << 30)
#define REFLECTION_PROBE_INDEX_BITMASK 0x3FFFFFFF
#define GET_REFLECTION_PROBE_INDEX(index) ((index) & REFLECTION_PROBE_INDEX_BITMASK)

  struct EZ_SHADER_STRUCT ezPerReflectionProbeData
  {
    TRANSFORM(WorldToProbeProjectionMatrix);
    FLOAT4(Scale);
    FLOAT4(ProbePosition);
    FLOAT4(PositiveFalloff);
    FLOAT4(NegativeFalloff);
    FLOAT4(InfluenceScale);
    FLOAT4(InfluenceShift);
    UINT1(Index);
    UINT1(Padding1);
    UINT1(Padding2);
    UINT1(Padding3);
  };

#if EZ_ENABLED(PLATFORM_SHADER)
  StructuredBuffer<ezPerReflectionProbeData> perPerReflectionProbeDataBuffer;
#else // C++
  static_assert(sizeof(ezPerReflectionProbeData) == 160);
#endif

  CONSTANT_BUFFER(ezClusteredDataConstants, 3)
  {
    FLOAT1(DepthSliceScale);
    FLOAT1(DepthSliceBias);
    FLOAT2(InvTileSize);

    UINT1(NumLights);
    UINT1(NumDecals);

    UINT1(BrightestDirectionalLightIndex); // aka sun
    UINT1(SkyIrradianceIndex);

    FLOAT1(FogHeight);
    FLOAT1(FogHeightFalloff);
    FLOAT1(FogDensityAtCameraPos);
    FLOAT1(FogDensity);
    COLOR4F(FogColor);
    FLOAT1(FogInvSkyDistance);
};

#define NUM_CLUSTERS_X 16
#define NUM_CLUSTERS_Y 8
#define NUM_CLUSTERS_Z 24
#define NUM_CLUSTERS_XY (NUM_CLUSTERS_X * NUM_CLUSTERS_Y)
#define NUM_CLUSTERS (NUM_CLUSTERS_X * NUM_CLUSTERS_Y * NUM_CLUSTERS_Z)

#define LIGHT_BITMASK 0x3FF
#define DECAL_SHIFT 10
#define DECAL_BITMASK 0x3FF
#define PROBE_SHIFT 20
#define PROBE_BITMASK 0x3FF

#define GET_LIGHT_INDEX(index) ((index) & LIGHT_BITMASK)
#define GET_DECAL_INDEX(index) ((index >> DECAL_SHIFT) & DECAL_BITMASK)
#define GET_PROBE_INDEX(index) ((index >> PROBE_SHIFT) & PROBE_BITMASK)

struct ezPerClusterData
{
  UINT1(offset);
  UINT1(counts);
};

#if EZ_ENABLED(PLATFORM_SHADER)
  StructuredBuffer<ezPerClusterData> perClusterDataBuffer;
  StructuredBuffer<uint> clusterItemBuffer;

  ezPerLightData GetBrightestDirectionalLightData()
  {
    if (BrightestDirectionalLightIndex != 0xFFFFFFFF)
    {
      return perLightDataBuffer[BrightestDirectionalLightIndex];
    }
    
    return (ezPerLightData)0;
  }
#endif

// clang-format on
