#pragma once

#include <Shaders/Common/GlobalConstants.h>
#include <Shaders/Common/LightData.h>
#include <Shaders/Common/BRDF.h>

#if USE_SSAO
  Texture2D SSAOTexture;
  SamplerState PointClampSampler;
#endif

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

float3 CalculateLighting(ezMaterialData matData, float2 screenPosition)
{
  float3 viewVector = matData.normalizedViewVector;

  float3 totalLight = 0.0f;

  for (uint i = 0; i < NumLights; ++i)
  {
    ezPerLightData lightData = perLightData[i];
    uint typeAndFlags = (lightData.colorAndType >> 24) & 0xFF;
    uint type = typeAndFlags & LIGHT_TYPE_MASK;

    float3 lightDir = normalize(RGB10ToFloat3(lightData.direction) * 2.0f - 1.0f);
    float3 lightVector = lightDir;
    float attenuation = 1.0f;

    [branch]
    if (type != LIGHT_TYPE_DIR)
    {
      lightVector = lightData.position - matData.worldPosition;
      float sqrDistance = dot(lightVector, lightVector);

      attenuation = DistanceAttenuation(sqrDistance, lightData.invSqrAttRadius);

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

      float intensity = lightData.intensity;
      float3 lightColor = RGB8ToFloat3(lightData.colorAndType);

      totalLight += DefaultShading(matData, lightVector, viewVector, float2(1.0f, 1.0f)) * lightColor * (intensity * attenuation);
    }
  }

  totalLight *= (1.0f / PI);

  float occlusion = matData.occlusion;
  #if USE_SSAO
    float ssao = SSAOTexture.Sample(PointClampSampler, screenPosition.xy).r;
    occlusion = min(occlusion, ssao);
  #endif

  // simple two color diffuse ambient
  float3 ambientLight = lerp(AmbientBottomColor.rgb, AmbientTopColor.rgb, matData.worldNormal.z * 0.5f + 0.5f);
  totalLight += matData.diffuseColor * ambientLight * occlusion;

  return totalLight;
}
