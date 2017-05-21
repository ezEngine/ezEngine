#pragma once

#include <Shaders/Common/GlobalConstants.h>
#include <Shaders/Common/LightData.h>
#include <Shaders/Common/BRDF.h>

#if USE_SSAO
  Texture2D SSAOTexture;
  SamplerState PointClampSampler;
#endif

Texture2D ShadowAtlasTexture;
SamplerComparisonState ShadowSampler;

Texture2D NoiseTexture;
SamplerState PointSampler;

///////////////////////////////////////////////////////////////////////////////////

ezPerClusterData GetClusterData(float3 screenPosition)
{
  // clustered data lookup
  float linearDepth = screenPosition.z;
  uint depthSlice = uint(clamp(log2(linearDepth) * DepthSliceScale + DepthSliceBias, 0, NUM_CLUSTERS_Z - 1));
  uint3 clusterCoord = uint3(screenPosition.xy * InvTileSize, depthSlice);
  uint clusterIndex = clusterCoord.z * NUM_CLUSTERS_XY + clusterCoord.y * NUM_CLUSTERS_X + clusterCoord.x;

  return perClusterDataBuffer[clusterIndex];
}

float DistanceAttenuation(float sqrDistance, float invSqrAttRadius)
{
  float attenuation = (1.0 / max(sqrDistance, 0.01 * 0.01)); // min distance is 1 cm;

  float factor = sqrDistance * invSqrAttRadius;
  float smoothFactor = saturate(1.0 - factor * factor);

  return attenuation * smoothFactor * smoothFactor;
}

float SpotAttenuation(float3 normalizedLightVector, float3 lightDir, float2 spotParams)
{
  float cosAngle = dot(lightDir, normalizedLightVector);
  float attenuation = saturate(cosAngle * spotParams.x + spotParams.y);

  return attenuation * attenuation;
}

float MicroShadow(float occlusion, float3 normal, float3 lightDir)
{
  float aperture = 2.0f * occlusion * occlusion;
  return saturate(abs(dot(normal, lightDir)) + aperture - 1.0f);
}

uint GetPointShadowFaceIndex(float3 dir)
{
  if (abs(dir.z) >= abs(dir.x) && abs(dir.z) >= abs(dir.y))
  {
    return (dir.z < 0.0) ? 5 : 4;
  }
  else if (abs(dir.y) >= abs(dir.x))
  {
    return (dir.y < 0.0) ? 3 : 2;
  }

  return (dir.x < 0.0) ? 1 : 0;
}

float CalculateShadow(float3 shadowPosition, float2x2 randomRotation, float penumbraSize)
{
  float2 offsets[] =
  {
    float2(-0.7071,  0.7071),
    float2(-0.0000, -0.8750),
    float2( 0.5303,  0.5303),
    float2(-0.6250, -0.0000),
    float2( 0.3536, -0.3536),
    float2(-0.0000,  0.3750),
    float2(-0.1768, -0.1768),
    float2( 0.1250,  0.0000),
  };

#if 0
  return ShadowAtlasTexture.SampleCmpLevelZero(ShadowSampler, shadowPosition.xy, shadowPosition.z);
#else
  float shadowTerm = 0.0f;
  for (int i = 0; i < 8; ++i)
  {
    float2 offset = mul(randomRotation, offsets[i]) * penumbraSize;

    float2 samplePos = shadowPosition.xy + offset;
    float sampleDepth = shadowPosition.z;
    shadowTerm += ShadowAtlasTexture.SampleCmpLevelZero(ShadowSampler, samplePos, sampleDepth);
  }

  return shadowTerm / 8.0f;
#endif
}

float3 CalculateLighting(ezMaterialData matData, ezPerClusterData clusterData, float3 screenPosition)
{
  float3 viewVector = matData.normalizedViewVector;

  float3 totalLight = 0.0f;

  float noise = InterleavedGradientNoise(screenPosition.xy) * 2.0f * PI;
  float2 randomAngle; sincos(noise, randomAngle.x, randomAngle.y);
  float2x2 randomRotation = {randomAngle.x, -randomAngle.y, randomAngle.y, randomAngle.x};

  uint firstItemIndex = clusterData.offset;
  uint lastItemIndex = firstItemIndex + clusterData.counts;

  [loop]
  for (uint i = firstItemIndex; i < lastItemIndex; ++i)
  {
    uint lightIndex = clusterItemBuffer[i];

    ezPerLightData lightData = perLightDataBuffer[lightIndex];
    uint type = (lightData.colorAndType >> 24) & 0xFF;

    float3 lightDir = normalize(RGB10ToFloat3(lightData.direction) * 2.0f - 1.0f);
    float3 lightVector = lightDir;
    float attenuation = 1.0f;
    float distanceToLight = 1.0f;

    [branch]
    if (type != LIGHT_TYPE_DIR)
    {
      lightVector = lightData.position - matData.worldPosition;
      float sqrDistance = dot(lightVector, lightVector);

      attenuation = DistanceAttenuation(sqrDistance, lightData.invSqrAttRadius);

      distanceToLight = sqrDistance * lightData.invSqrAttRadius;
      lightVector *= rsqrt(sqrDistance);

      [branch]
      if (type == LIGHT_TYPE_SPOT)
      {
        float2 spotParams = RG16FToFloat2(lightData.spotParams);
        attenuation *= SpotAttenuation(lightVector, lightDir, spotParams);
      }
    }

    attenuation *= saturate(dot(matData.worldNormal, lightVector));

    if (attenuation > 0.0f)
    {
      attenuation *= MicroShadow(matData.occlusion, matData.worldNormal, lightVector);

      if (lightData.shadowDataOffset != 0xFFFFFFFF)
      {
        uint shadowDataOffset = lightData.shadowDataOffset;
        float4 shadowParams = shadowDataBuffer[GET_SHADOW_PARAMS_OFFSET(shadowDataOffset)];
        
        // normal offset bias
        float normalOffsetBias = shadowParams.x * distanceToLight;
        float normalOffsetScale = normalOffsetBias - dot(matData.vertexNormal, lightVector) * normalOffsetBias;
        float3 offsettedWorldPos = matData.worldPosition + matData.vertexNormal * normalOffsetScale;
        
        uint matrixIndex = 0;
        if (type == LIGHT_TYPE_POINT)
        {
          matrixIndex = GetPointShadowFaceIndex(-lightVector);
        }

        uint matrixOffset = GET_WORLD_TO_LIGHT_MATRIX_OFFSET(shadowDataOffset, matrixIndex);

        // manual matrix multiplication because d3d compiler generates very inefficient code otherwise
        float4 shadowPosition = shadowDataBuffer[matrixOffset + 0] * offsettedWorldPos.x + shadowDataBuffer[matrixOffset + 3];
        shadowPosition += shadowDataBuffer[matrixOffset + 1] * offsettedWorldPos.y;
        shadowPosition += shadowDataBuffer[matrixOffset + 2] * offsettedWorldPos.z;
        
        // constant bias
        float constantBias = shadowParams.y;
        shadowPosition.z -= constantBias;
        
        shadowPosition.xyz /= shadowPosition.w;
        
        float shadowTerm = CalculateShadow(shadowPosition, randomRotation, shadowParams.z);

        if (type != LIGHT_TYPE_DIR)
        {
          shadowTerm = lerp(1.0f, shadowTerm, shadowParams.w);
        }

        attenuation *= shadowTerm;
      }

      float intensity = lightData.intensity;
      float3 lightColor = RGB8ToFloat3(lightData.colorAndType);

      totalLight += DefaultShading(matData, lightVector, viewVector, float2(1.0f, 1.0f)) * lightColor * (intensity * attenuation);
    }
  }

  totalLight *= (1.0f / PI);

  float occlusion = matData.occlusion;
  #if USE_SSAO
    float ssao = SSAOTexture.SampleLevel(PointClampSampler, screenPosition.xy * ViewportSize.zw, 0.0f).r;
    occlusion = min(occlusion, ssao);
    #if APPLY_SSAO_TO_DIRECT_LIGHTING
      totalLight *= occlusion;
    #endif
  #endif

  // simple two color diffuse ambient
  float3 ambientLight = lerp(AmbientBottomColor.rgb, AmbientTopColor.rgb, matData.worldNormal.z * 0.5f + 0.5f);
  totalLight += matData.diffuseColor * ambientLight * occlusion;

  return totalLight;
}

float3 ApplyFog(float3 color, float3 worldPosition)
{
  float distance = length(CameraPosition - worldPosition);
  float amount = saturate(1.0f - exp(-distance*0.001));
  float3 fogColor = float3(0.5,0.6,0.7);

  return lerp(color, fogColor, amount);
}
