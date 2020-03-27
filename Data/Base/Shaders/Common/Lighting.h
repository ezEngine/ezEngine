#pragma once

#if SHADING_QUALITY != SHADING_QUALITY_NORMAL
#error "Functions in Lighting.h are only for QUALITY_NORMAL shading quality. Todo: Split up file"
#endif

#include <Shaders/Common/GlobalConstants.h>
#include <Shaders/Common/LightData.h>
#include <Shaders/Common/AmbientCubeBasis.h>
#include <Shaders/Common/BRDF.h>

Texture2D SSAOTexture;

Texture2D ShadowAtlasTexture;
SamplerComparisonState ShadowSampler;

Texture2D DecalAtlasBaseColorTexture;
Texture2D DecalAtlasNormalTexture;
Texture2D DecalAtlasORMTexture;
SamplerState DecalAtlasSampler;

TextureCubeArray ReflectionSpecularTexture;
Texture2D SkyIrradianceTexture;

Texture2D SceneDepth;
Texture2D SceneColor;
SamplerState SceneColorSampler;

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

///////////////////////////////////////////////////////////////////////////////////

float3 SampleSceneColor(float2 screenPosition)
{
  float3 sceneColor = SceneColor.SampleLevel(SceneColorSampler, screenPosition.xy * ViewportSize.zw, 0.0f).rgb;
  return sceneColor;
}

float SampleSceneDepth(float2 screenPosition)
{
  float depthFromZBuffer = SceneDepth.SampleLevel(PointClampSampler, screenPosition.xy * ViewportSize.zw, 0.0f).r;
  return LinearizeZBufferDepth(depthFromZBuffer);
}

float3 SampleScenePosition(float2 screenPosition)
{
  float2 normalizedScreenPosition = screenPosition.xy * ViewportSize.zw;
  float depthFromZBuffer = SceneDepth.SampleLevel(PointClampSampler, normalizedScreenPosition, 0.0f).r;
  float4 fullScreenPosition = float4(normalizedScreenPosition * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), depthFromZBuffer, 1.0f);
  float4 scenePosition = mul(GetScreenToWorldMatrix(), fullScreenPosition);
  return scenePosition.xyz / scenePosition.w;
}

float DepthFade(float3 screenPosition, float fadeDistance)
{
  float distance = SampleSceneDepth(screenPosition.xy) - screenPosition.z;
  return saturate(distance / fadeDistance);
}

///////////////////////////////////////////////////////////////////////////////////

float SampleSSAO(float3 screenPosition)
{
  const float2 offsets[] =
  {
    float2(0, 0),
    float2(0, 1),
    float2(1, 0),
    float2(0, -1),
    float2(-1, 0)
  };

#if 0
  return SSAOTexture.SampleLevel(PointClampSampler, screenPosition.xy * ViewportSize.zw, 0.0f).r;
#else
  float totalSSAO = 0.0f;
  float totalWeight = 0.0f;

  [unroll]
  for (int i = 0; i < 5; ++i)
  {
    float2 samplePos = (screenPosition.xy + offsets[i]) * ViewportSize.zw;
    float2 ssaoAndDepth = SSAOTexture.SampleLevel(PointClampSampler, samplePos, 0.0f).rg;
    float weight = saturate(1 - abs(screenPosition.z - ssaoAndDepth.y) * 2.0f);

    totalSSAO += ssaoAndDepth.x * weight;
    totalWeight += weight;
  }

  return totalWeight > 0.0f ? totalSSAO / totalWeight : 1.0f;
#endif
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

float SampleShadow(float3 shadowPosition, float2x2 randomRotation, float penumbraSize)
{
  const float2 offsets[] =
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

float CalculateShadowTerm(ezMaterialData matData, float3 lightVector, float distanceToLight, uint type,
  uint shadowDataOffset, float noise, float2x2 randomRotation, out float subsurfaceShadow, out float3 debugColor)
{
  float3 debugColors[] =
  {
    float3(1, 0, 0),
    float3(1, 1, 0),
    float3(0, 1, 0),
    float3(0, 1, 1),
    float3(0, 0, 1),
    float3(1, 0, 1)
  };

  float4 shadowParams = shadowDataBuffer[GET_SHADOW_PARAMS_INDEX(shadowDataOffset)];

  // normal offset bias
  float normalOffsetBias = shadowParams.x * distanceToLight;
  float normalOffsetScale = normalOffsetBias - dot(matData.vertexNormal, lightVector) * normalOffsetBias;
  float3 worldPosition = matData.worldPosition + matData.vertexNormal * normalOffsetScale;

  float constantBias = shadowParams.y;
  float penumbraSize = shadowParams.z;
  float fadeOut = shadowParams.w;

  float4 shadowPosition;

  [branch]
  if (type == LIGHT_TYPE_DIR)
  {
    uint matrixIndex = GET_WORLD_TO_LIGHT_MATRIX_INDEX(shadowDataOffset, 0);

    // manual matrix multiplication because d3d compiler generates very inefficient code otherwise
    shadowPosition.xyz = shadowDataBuffer[matrixIndex + 0].xyz * worldPosition.x + shadowDataBuffer[matrixIndex + 3].xyz;
    shadowPosition.xyz += shadowDataBuffer[matrixIndex + 1].xyz * worldPosition.y;
    shadowPosition.xyz += shadowDataBuffer[matrixIndex + 2].xyz * worldPosition.z;
    shadowPosition.w = 1.0f;

    float4 firstCascadePosition = shadowPosition;
    float penumbraSizeScale = 1.0f;

    float4 shadowParams2 = shadowDataBuffer[GET_SHADOW_PARAMS2_INDEX(shadowDataOffset)];
    float2 threshold = float2(shadowParams2.x, 1.0f) - shadowParams2.yz * noise;

    uint cascadeIndex = 0;
    uint lastCascadeIndex = asuint(shadowParams.w);
    while (cascadeIndex < lastCascadeIndex)
    {
      if (all(abs(shadowPosition.xyz) < threshold.xxy))
        break;

      float4 cascadeScale = shadowDataBuffer[GET_CASCADE_SCALE_INDEX(shadowDataOffset, cascadeIndex)];
      float4 cascadeOffset = shadowDataBuffer[GET_CASCADE_OFFSET_INDEX(shadowDataOffset, cascadeIndex)];
      shadowPosition.xyz = firstCascadePosition.xyz * cascadeScale.xyz + cascadeOffset.xyz;
      penumbraSizeScale = cascadeScale.x;

      ++cascadeIndex;
    }

    constantBias /= penumbraSizeScale;
    float viewDistance = length(GetCameraPosition() - matData.worldPosition);
    penumbraSize = (penumbraSize + shadowParams2.w * viewDistance) * penumbraSizeScale;

    if (cascadeIndex == lastCascadeIndex)
    {
      float4 fadeOutParams = shadowDataBuffer[GET_FADE_OUT_PARAMS_INDEX(shadowDataOffset)];
      float xyMax = max(abs(shadowPosition.x), abs(shadowPosition.y));
      float xyFadeOut = saturate(xyMax * fadeOutParams.x + fadeOutParams.y);
      float zFadeOut = saturate(shadowPosition.z * fadeOutParams.z + fadeOutParams.w);

      fadeOut = min(xyFadeOut, zFadeOut);
    }
    else
    {
      fadeOut = 1.0f;
    }

    float4 atlasScaleOffset = shadowDataBuffer[GET_ATLAS_SCALE_OFFSET_INDEX(shadowDataOffset, cascadeIndex)];
    shadowPosition.xy = shadowPosition.xy * atlasScaleOffset.xy + atlasScaleOffset.zw;

    debugColor = debugColors[fadeOut > 0 ? cascadeIndex : cascadeIndex + 1];
  }
  else
  {
    uint index = 0;
    if (type == LIGHT_TYPE_POINT)
    {
      index = GetPointShadowFaceIndex(-lightVector);
    }

    uint matrixIndex = GET_WORLD_TO_LIGHT_MATRIX_INDEX(shadowDataOffset, index);

    // manual matrix multiplication because d3d compiler generates very inefficient code otherwise
    shadowPosition = shadowDataBuffer[matrixIndex + 0] * worldPosition.x + shadowDataBuffer[matrixIndex + 3];
    shadowPosition += shadowDataBuffer[matrixIndex + 1] * worldPosition.y;
    shadowPosition += shadowDataBuffer[matrixIndex + 2] * worldPosition.z;

    debugColor = debugColors[matrixIndex];
  }

  [branch]
  if (fadeOut > 0.0f)
  {
    // constant bias
    shadowPosition.z -= constantBias;

    shadowPosition.xyz /= shadowPosition.w;

    float shadowTerm = SampleShadow(shadowPosition.xyz, randomRotation, penumbraSize);

    // fade out
    shadowTerm = lerp(1.0f, shadowTerm, fadeOut);

    subsurfaceShadow = shadowTerm;

    return shadowTerm;
  }

  return 1.0f;
}

AccumulatedLight CalculateLighting(ezMaterialData matData, ezPerClusterData clusterData, float3 screenPosition, bool applySSAO)
{
  float3 viewVector = normalize(GetCameraPosition() - matData.worldPosition);

  AccumulatedLight totalLight = InitializeLight(0.0f, 0.0f);

  float noise = InterleavedGradientNoise(screenPosition.xy);
  float2 randomAngle; sincos(noise * 2.0f * PI, randomAngle.x, randomAngle.y);
  float2x2 randomRotation = {randomAngle.x, -randomAngle.y, randomAngle.y, randomAngle.x};

  uint firstItemIndex = clusterData.offset;
  uint lastItemIndex = firstItemIndex + GET_LIGHT_INDEX(clusterData.counts);

  [loop]
  for (uint i = firstItemIndex; i < lastItemIndex; ++i)
  {
    uint itemIndex = clusterItemBuffer[i];
    uint lightIndex = GET_LIGHT_INDEX(itemIndex);

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

    float NdotL = saturate(dot(matData.worldNormal, lightVector));

  #if !defined(USE_MATERIAL_SUBSURFACE_COLOR)
    [branch]
    if (attenuation * NdotL > 0.0f)
  #endif
    {
      attenuation *= MicroShadow(matData.occlusion, matData.worldNormal, lightVector);

      float3 debugColor = 1.0f;
      float shadowTerm = 1.0;
      float subsurfaceShadow = 1.0;

      [branch]
      if (lightData.shadowDataOffset != 0xFFFFFFFF)
      {
        uint shadowDataOffset = lightData.shadowDataOffset;

        shadowTerm = CalculateShadowTerm(matData, lightVector, distanceToLight, type,
          shadowDataOffset, noise, randomRotation, subsurfaceShadow, debugColor);
      }

      attenuation *= lightData.intensity;
      float3 lightColor = RGB8ToFloat3(lightData.colorAndType);

      // debug cascade or point face selection
      #if 0
        lightColor = lerp(1.0f, debugColor, 0.5f);
      #endif

      AccumulateLight(totalLight, DefaultShading(matData, lightVector, viewVector), lightColor * (attenuation * shadowTerm));

      #if defined(USE_MATERIAL_SUBSURFACE_COLOR)
        AccumulateLight(totalLight, SubsurfaceShading(matData, lightVector, viewVector), lightColor * (attenuation * subsurfaceShadow));        
      #endif
    }
  }
  
  // normalize brdf
  totalLight.diffuseLight *= (1.0f / PI);
  totalLight.specularLight *= (1.0f / PI);

  float occlusion = matData.occlusion;

  if (applySSAO)
  {
    float ssao = SampleSSAO(screenPosition);
    occlusion *= ssao;
  }

  // sky light in ambient cube basis
  float3 skyLight = EvaluateAmbientCube(SkyIrradianceTexture, SkyIrradianceIndex, matData.worldNormal).rgb;
  totalLight.diffuseLight += matData.diffuseColor * skyLight * occlusion;
  
  // enable once we have proper sky visibility
  /*#if defined(USE_MATERIAL_SUBSURFACE_COLOR)
    skyLight = EvaluateAmbientCube(SkyIrradianceTexture, SkyIrradianceIndex, -matData.worldNormal).rgb;
    totalLight.diffuseLight += matData.subsurfaceColor * skyLight * occlusion;
  #endif*/

  return totalLight;
}

void ApplyDecals(inout ezMaterialData matData, ezPerClusterData clusterData, uint gameObjectId)
{
  uint firstItemIndex = clusterData.offset;
  uint lastItemIndex = firstItemIndex + GET_DECAL_INDEX(clusterData.counts);

  uint applyOnlyToId = (gameObjectId & (1 << 31)) ? gameObjectId : 0;
  
  float3 worldPosDdx = ddx(matData.worldPosition);
  float3 worldPosDdy = ddy(matData.worldPosition);

  [loop]
  for (uint i = firstItemIndex; i < lastItemIndex; ++i)
  {
    uint itemIndex = clusterItemBuffer[i];
    uint decalIndex = GET_DECAL_INDEX(itemIndex);

    ezPerDecalData decalData = perDecalDataBuffer[decalIndex];
    if (decalData.applyOnlyToId != applyOnlyToId)
      continue;

    float4x4 worldToDecalMatrix = TransformToMatrix(decalData.worldToDecalMatrix);
    float3 decalPosition = mul(worldToDecalMatrix, float4(matData.worldPosition, 1.0f)).xyz;

    if (any(abs(decalPosition) >= 1.0f))
      continue;

    uint decalFlags = decalData.decalFlags;
    float2 angleFadeParams = RG16FToFloat2(decalData.angleFadeParams);

    float3 decalNormal = normalize(mul((float3x3)worldToDecalMatrix, matData.vertexNormal));

    float fade = saturate(-decalNormal.z * angleFadeParams.x + angleFadeParams.y);
    fade *= fade;

    float3 borderFade = 1.0f - decalPosition * decalPosition;
    //fade *= min(borderFade.x, min(borderFade.y, borderFade.z));
    fade *= borderFade.z;

    if (fade > 0.0f)
    {
      if (decalFlags & DECAL_WRAP_AROUND)
      {
        decalPosition.xy += decalNormal.xy * decalPosition.z;
        decalPosition.xy = clamp(decalPosition.xy, -1.0f, 1.0f);
      }
      
      float2 decalPositionDdx = mul((float3x3)worldToDecalMatrix, worldPosDdx).xy;
      float2 decalPositionDdy = mul((float3x3)worldToDecalMatrix, worldPosDdy).xy;
      
      float3 decalWorldNormal;
      if (decalFlags & DECAL_USE_NORMAL)
      {
        float2 normalAtlasScale = RG16FToFloat2(decalData.normalAtlasScale);
        float2 normalAtlasOffset = RG16FToFloat2(decalData.normalAtlasOffset);
        float2 normalAtlasUv = decalPosition.xy * normalAtlasScale + normalAtlasOffset;
        float2 normalAtlasDdx = decalPositionDdx * normalAtlasScale;
        float2 normalAtlasDdy = decalPositionDdy * normalAtlasScale;
        
        float3 decalTangentNormal = DecodeNormalTexture(DecalAtlasNormalTexture.SampleGrad(DecalAtlasSampler, normalAtlasUv, normalAtlasDdx, normalAtlasDdy));
        
        [branch]
        if (decalFlags & DECAL_MAP_NORMAL_TO_GEOMETRY)
        {
          float3 xAxis = normalize(cross(worldToDecalMatrix._m10_m11_m12, matData.vertexNormal));
          decalWorldNormal = decalTangentNormal.x * xAxis;          
          decalWorldNormal += decalTangentNormal.y * cross(matData.vertexNormal, xAxis);
          decalWorldNormal += decalTangentNormal.z * matData.vertexNormal;
        }
        else
        {
          decalWorldNormal = decalTangentNormal.x * normalize(worldToDecalMatrix._m00_m01_m02);
          decalWorldNormal += decalTangentNormal.y * normalize(worldToDecalMatrix._m10_m11_m12);
          decalWorldNormal -= decalTangentNormal.z * normalize(worldToDecalMatrix._m20_m21_m22);
        }
      }
      
      float2 baseAtlasScale = RG16FToFloat2(decalData.baseColorAtlasScale);
      float2 baseAtlasOffset = RG16FToFloat2(decalData.baseColorAtlasOffset);
      float2 baseAtlasUv = decalPosition.xy * baseAtlasScale + baseAtlasOffset;
      float2 baseAtlasDdx = decalPositionDdx * baseAtlasScale;
      float2 baseAtlasDdy = decalPositionDdy * baseAtlasScale;

      float4 decalBaseColor = RGBA8ToFloat4(decalData.baseColor);
      decalBaseColor *= DecalAtlasBaseColorTexture.SampleGrad(DecalAtlasSampler, baseAtlasUv, baseAtlasDdx, baseAtlasDdy);
      fade *= decalBaseColor.a;
      
      if (decalFlags & DECAL_USE_NORMAL)
      {
        matData.worldNormal = lerp(matData.worldNormal, decalWorldNormal, fade);
      }
      
      float decalMetallic = 0.0f;
      if (decalFlags & DECAL_USE_ORM)
      {
        float2 ormAtlasScale = RG16FToFloat2(decalData.ormAtlasScale);
        float2 ormAtlasOffset = RG16FToFloat2(decalData.ormAtlasOffset);
        float2 ormAtlasUv = decalPosition.xy * ormAtlasScale + ormAtlasOffset;
        float2 ormAtlasDdx = decalPositionDdx * ormAtlasScale;
        float2 ormAtlasDdy = decalPositionDdy * ormAtlasScale;
        
        float3 decalORM = DecalAtlasORMTexture.SampleGrad(DecalAtlasSampler, ormAtlasUv, ormAtlasDdx, ormAtlasDdy).rgb;
        
        matData.occlusion = lerp(matData.occlusion, decalORM.r, fade);
        matData.roughness = lerp(matData.roughness, decalORM.g, fade);
        decalMetallic = decalORM.b;
      }
      
      float3 decalDiffuseColor = lerp(decalBaseColor.rgb, 0.0f, decalMetallic);
      if (decalFlags & DECAL_BLEND_MODE_COLORIZE)
      {
        matData.diffuseColor = Colorize(matData.diffuseColor, decalDiffuseColor, fade);
      }
      else
      {
        matData.diffuseColor = lerp(matData.diffuseColor, decalDiffuseColor, fade);
      }
      
      float3 decalSpecularColor = lerp(0.04f, decalBaseColor.rgb, decalMetallic);
      matData.specularColor = lerp(matData.specularColor, decalSpecularColor, fade);
      
      float3 decalEmissiveColor = RGBA16FToFloat4(decalData.emissiveColorRG, decalData.emissiveColorBA).rgb;
      
      if (decalFlags & DECAL_USE_EMISSIVE)
      {
        float2 ormAtlasScale = RG16FToFloat2(decalData.ormAtlasScale);
        float2 ormAtlasOffset = RG16FToFloat2(decalData.ormAtlasOffset);
        float2 ormAtlasUv = decalPosition.xy * ormAtlasScale + ormAtlasOffset;
        float2 ormAtlasDdx = decalPositionDdx * ormAtlasScale;
        float2 ormAtlasDdy = decalPositionDdy * ormAtlasScale;
        
        decalEmissiveColor *= SrgbToLinear(DecalAtlasORMTexture.SampleGrad(DecalAtlasSampler, ormAtlasUv, ormAtlasDdx, ormAtlasDdy).rgb);
      }
      matData.emissiveColor += decalEmissiveColor * fade;
      
      matData.opacity = max(matData.opacity, fade);      
    }
  }
  
  matData.worldNormal = normalize(matData.worldNormal);
}

float4 CalculateRefraction(float3 worldPosition, float3 worldNormal, float IoR, float thickness, float3 tintColor, float newOpacity = 1.0f)
{
  float3 normalizedViewVector = normalize(GetCameraPosition() - worldPosition);
  float r = 1.0f / IoR;
  float NdotV = dot(worldNormal, normalizedViewVector);
  float k = 1.0f - r * r * (1.0f - NdotV * NdotV);
  float3 refractVector = r * -normalizedViewVector + (r * NdotV - sqrt(k)) * worldNormal;

  float4 projectedRefractVector = mul(GetWorldToScreenMatrix(), float4(worldPosition + refractVector * thickness, 1.0f));
  projectedRefractVector.xy /= projectedRefractVector.w;

  float2 refractCoords = projectedRefractVector.xy * float2(0.5f, -0.5f) + 0.5f;
  float3 refractionColor = SceneColor.SampleLevel(SceneColorSampler, refractCoords, 0.0f).rgb;

  float fresnel = pow(1.0f - NdotV, 5.0f);
  refractionColor *= tintColor * (1.0f - fresnel);

  return float4(refractionColor, newOpacity);
}

void ApplyRefraction(inout ezMaterialData matData, inout AccumulatedLight light)
{
  light.diffuseLight = lerp(matData.refractionColor.rgb, light.diffuseLight, matData.opacity);
  matData.opacity = matData.refractionColor.a;
}

float GetFogAmount(float3 worldPosition)
{
  float3 cameraToWorldPos = worldPosition - GetCameraPosition();
  float fogDensity = FogDensity;

  if (FogHeightFalloff != 0.0)
  {
    float range = FogHeightFalloff * cameraToWorldPos.z;
    fogDensity *= saturate((exp(FogHeightFalloff * worldPosition.z + FogHeight) - FogDensityAtCameraPos) / range);
  }

  return saturate(exp(-fogDensity * length(cameraToWorldPos)));
}

float3 ApplyFog(float3 color, float fogAmount)
{
  return lerp(FogColor.xyz, color, fogAmount);
}

float3 ApplyFog(float3 color, float3 worldPosition)
{
  if (FogDensity > 0.0)
  {
    return ApplyFog(color, GetFogAmount(worldPosition));
  }

  return color;
}
