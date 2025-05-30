#pragma once

#if SHADING_QUALITY != SHADING_QUALITY_NORMAL
#  error "Functions in Lighting.h are only for QUALITY_NORMAL shading quality. Todo: Split up file"
#endif

#include <Shaders/Common/AmbientCubeBasis.h>
#include <Shaders/Common/BRDF.h>
#include <Shaders/Common/GlobalConstants.h>
#include <Shaders/Common/LightData.h>

Texture2DArray SSAOTexture;

Texture2D ShadowAtlasTexture;
SamplerComparisonState ShadowSampler;

Texture2D DecalAtlasBaseColorTexture;
Texture2D DecalAtlasNormalTexture;
Texture2D DecalAtlasORMTexture;
Texture2D DecalRuntimeAtlasTexture;
SamplerState DecalAtlasSampler;

TextureCubeArray ReflectionSpecularTexture;
Texture2D SkyIrradianceTexture;
#define NUM_REFLECTION_MIPS 6

Texture2DArray SceneDepth;
Texture2DArray SceneColor;
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
  float3 sceneColor = SceneColor.SampleLevel(SceneColorSampler, float3(screenPosition.xy * ViewportSize.zw, s_ActiveCameraEyeIndex), 0.0f).rgb;
  return sceneColor;
}

float SampleSceneDepth(float2 screenPosition)
{
  int2 screenXY = clamp(screenPosition.xy, 0, ViewportSize.xy - 1);

  float depthFromZBuffer = SceneDepth.Load(int4(screenXY, s_ActiveCameraEyeIndex, 0)).r;
  return LinearizeZBufferDepth(depthFromZBuffer);
}

float3 SampleScenePosition(float2 screenPosition)
{
  float2 normalizedScreenPosition = screenPosition.xy * ViewportSize.zw;
  float depthFromZBuffer = SceneDepth.SampleLevel(PointClampSampler, float3(normalizedScreenPosition, s_ActiveCameraEyeIndex), 0.0f).r;
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
  const float2 offsets[] = {
    float2(0, 0),
    float2(0, 1),
    float2(1, 0),
    float2(0, -1),
    float2(-1, 0),
  };

#if 0
  return SSAOTexture.SampleLevel(PointClampSampler, float3(screenPosition.xy * ViewportSize.zw, s_ActiveCameraEyeIndex), 0.0f).r;
#else
  float totalSSAO = 0.0f;
  float totalWeight = 0.0f;

  [unroll] for (int i = 0; i < 5; ++i)
  {
    float2 samplePos = (screenPosition.xy + offsets[i]) * ViewportSize.zw;
    float2 ssaoAndDepth = SSAOTexture.SampleLevel(PointClampSampler, float3(samplePos, s_ActiveCameraEyeIndex), 0.0f).rg;
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
  // Simple spiral pattern with every other sample flipped around the origin,
  // source: https://c-core-games.com/Generator/SpiralPatterns.html?p=O&n=8
  const float2 offsets[] = {
    {0.1250f, 0.0000f},
    {-0.1768f, -0.1768f},
    {0.0000f, 0.3750f},
    {0.3536f, -0.3536f},
    {-0.6250f, 0.0000f},
    {0.5303f, 0.5303f},
    {-0.0000f, -0.8750f},
    {-0.7071f, 0.7071f},
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

// Enable to only sample the last cascade for directional lights (which will always enclose the whole shadow range)
// #define SHADOW_FORCE_LAST_CASCADE

float CalculateShadowTerm(float3 worldPosition, float3 vertexNormal, float3 lightVector, float distanceToLight, uint type,
  uint shadowDataOffsetAndFadeOut, float noise, float2x2 randomRotation, float extraPenumbraScale, inout float subsurfaceShadow, out float3 debugColor)
{
  float3 debugColors[] = {
    float3(1, 0, 0),
    float3(1, 1, 0),
    float3(0, 1, 0),
    float3(0, 1, 1),
    float3(0, 0, 1),
    float3(1, 0, 1),
  };

  uint shadowDataOffset = shadowDataOffsetAndFadeOut & 0xFFFFF;
  float4 shadowParams = shadowDataBuffer[GET_SHADOW_PARAMS_INDEX(shadowDataOffset)];

  float viewDistance = length(GetCameraPosition() - worldPosition);

  // normal offset bias
  float normalOffsetBias = shadowParams.x * distanceToLight;
  float normalOffsetScale = normalOffsetBias - dot(vertexNormal, lightVector) * normalOffsetBias;
  worldPosition += vertexNormal * normalOffsetScale;

  float constantBias = shadowParams.y;
  float penumbraSize = shadowParams.z;
  float fadeOut = 1.0;

  float4 shadowPosition;

  [branch] if (type == LIGHT_TYPE_DIR)
  {
    uint matrixIndex = GET_WORLD_TO_LIGHT_MATRIX_INDEX(shadowDataOffset, 0);

    // manual matrix multiplication because d3d compiler generates very inefficient code otherwise
    shadowPosition.xyz = shadowDataBuffer[matrixIndex + 0].xyz * worldPosition.x + shadowDataBuffer[matrixIndex + 3].xyz;
    shadowPosition.xyz += shadowDataBuffer[matrixIndex + 1].xyz * worldPosition.y;
    shadowPosition.xyz += shadowDataBuffer[matrixIndex + 2].xyz * worldPosition.z;
    shadowPosition.w = 1.0f;

    uint lastCascadeIndex = asuint(shadowParams.w);
    float4 shadowParams2 = shadowDataBuffer[GET_SHADOW_PARAMS2_INDEX(shadowDataOffset)];

#if defined(SHADOW_FORCE_LAST_CASCADE)
    uint cascadeIndex = lastCascadeIndex;
    float4 cascadeScale = shadowDataBuffer[GET_CASCADE_SCALE_INDEX(shadowDataOffset, cascadeIndex - 1)];
    float4 cascadeOffset = shadowDataBuffer[GET_CASCADE_OFFSET_INDEX(shadowDataOffset, cascadeIndex - 1)];
    shadowPosition.xyz = shadowPosition.xyz * cascadeScale.xyz + cascadeOffset.xyz;
    float penumbraSizeScale = cascadeScale.x;

#else
    float4 firstCascadePosition = shadowPosition;
    float penumbraSizeScale = 1.0f;

    float2 threshold = float2(shadowParams2.x, 1.0f) - shadowParams2.yz * noise;

    uint cascadeIndex = 0;
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
#endif

    constantBias /= penumbraSizeScale;
    penumbraSize = (penumbraSize + shadowParams2.w * viewDistance) * penumbraSizeScale;

    if (cascadeIndex == lastCascadeIndex)
    {
      float4 fadeOutParams = shadowDataBuffer[GET_FADE_OUT_PARAMS_INDEX(shadowDataOffset)];
      float xyMax = max(abs(shadowPosition.x), abs(shadowPosition.y));
      float2 xyScaleOffset = RG16FToFloat2(asuint(fadeOutParams.x));
      float2 zScaleOffset = RG16FToFloat2(asuint(fadeOutParams.y));
      float xyFadeOut = saturate(xyMax * xyScaleOffset.x + xyScaleOffset.y);
      float zFadeOut = saturate(shadowPosition.z * zScaleOffset.x + zScaleOffset.y);
      float distanceFadeOut = saturate(viewDistance * fadeOutParams.z + fadeOutParams.w);

      fadeOut = max(min(xyFadeOut, zFadeOut), distanceFadeOut);
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

    fadeOut = (shadowDataOffsetAndFadeOut >> 20) / 4095.0;
  }

  [branch] if (fadeOut > 0.0f)
  {
    // constant bias
    shadowPosition.z -= constantBias;

    shadowPosition.xyz /= shadowPosition.w;

    float shadowTerm = SampleShadow(shadowPosition.xyz, randomRotation, penumbraSize * extraPenumbraScale);

    // fade out
    shadowTerm = lerp(1.0f, shadowTerm, fadeOut);

    subsurfaceShadow = shadowTerm;

    return shadowTerm;
  }

  return 1.0f;
}

float3 SampleLightCookie(ezPerLightData lightData, float3 worldPosition)
{
  uint cookieParams0 = lightData.cookieParams0;
  uint cookieParams1 = lightData.cookieParams1;

  ezPerDecalAtlasData atlasData = perDecalAtlasDataBuffer[cookieParams0 & 0x7FFF];
  if (atlasData.scale != 0)
  {
    float3 forwardDir = -GetLightDirection(lightData);
    float3 rightDir = float3(RG16FToFloat2(cookieParams1), f16tof32(cookieParams0 >> 16));
    float3 upDir = cross(rightDir, forwardDir);

    // Transform to cookie space
    float3 localPos = worldPosition - lightData.position;
    localPos = float3(dot(localPos, rightDir), dot(localPos, upDir), dot(localPos, forwardDir));
    localPos.xy /= localPos.z;

    float2 uv = localPos.xy * RG16FToFloat2(atlasData.scale) + RG16FToFloat2(atlasData.offset);

    float4 cookie = DecalRuntimeAtlasTexture.SampleLevel(DecalAtlasSampler, uv, 0);
    return cookie.rgb * cookie.a;
  }

  return 1;
}

// Frostbite course notes
float computeDistanceBaseRoughness(float distIntersectionToShadedPoint, float distIntersectionToProbeCenter, float linearRoughness)
{
  // To avoid artifacts we clamp to the original linearRoughness
  // which introduces an acceptable bias and allows conservation
  // of mirror reflection behavior for a smooth surface.
  float newLinearRoughness = clamp(distIntersectionToShadedPoint / distIntersectionToProbeCenter * linearRoughness,
    0, linearRoughness);
  return lerp(newLinearRoughness, linearRoughness, linearRoughness);
}

float3 ComputeReflection(inout ezMaterialData matData, float3 viewVector, ezPerClusterData clusterData)
{
  uint firstItemIndex = clusterData.offset;
  uint lastItemIndex = firstItemIndex + GET_PROBE_INDEX(clusterData.counts);

  const float3 positionWS = matData.worldPosition;
  const float3 reflDirectionWS = reflect(-viewVector, matData.worldNormal);

  float4 ref = float4(0, 0, 0, 0);

  [loop] for (uint i = firstItemIndex; i < lastItemIndex; ++i)
  {
    const uint itemIndex = clusterItemBuffer[i];
    const uint probeIndex = GET_PROBE_INDEX(itemIndex);

    const ezPerReflectionProbeData probeData = perPerReflectionProbeDataBuffer[probeIndex];
    const uint index = GET_REFLECTION_PROBE_INDEX(probeData.Index);
    const bool bIsSphere = (probeData.Index & REFLECTION_PROBE_IS_SPHERE) > 0;
    const bool bIsProjected = (probeData.Index & REFLECTION_PROBE_IS_PROJECTED) > 0;

    // There are three spaces here:
    // CubeMap space: The space in which we can sample the cube map. This space is unscaled compared to world space. We are just rotating and moving to the location the cube map was captured.
    // Projection space: A [-1, 1] cube in all directions that encompases the projected space that the cube map is projected to in case of box projections.
    // Influence space: A [-1, 1] cube in all directions that encompases the influence space in which this cube map is sampled. This space is the projection space scaled down and shifted.
    const float4x4 worldToProbeProjectionMatrix = float4x4(
      probeData.WorldToProbeProjectionMatrix.r0 * probeData.Scale.x,
      probeData.WorldToProbeProjectionMatrix.r1 * probeData.Scale.y,
      probeData.WorldToProbeProjectionMatrix.r2 * probeData.Scale.z, float4(0, 0, 0, 1));
    const float3x3 worldToProbeNormalMatrix = float3x3(
      float3(worldToProbeProjectionMatrix._m00, worldToProbeProjectionMatrix._m01, worldToProbeProjectionMatrix._m02),
      float3(worldToProbeProjectionMatrix._m10, worldToProbeProjectionMatrix._m11, worldToProbeProjectionMatrix._m12),
      float3(worldToProbeProjectionMatrix._m20, worldToProbeProjectionMatrix._m21, worldToProbeProjectionMatrix._m22));
    const float3x3 worldToProbeCubeMapNormalMatrix = TransformToRotation(probeData.WorldToProbeProjectionMatrix);
    const float3 probeProjectionPosition = mul(worldToProbeProjectionMatrix, float4(matData.worldPosition, 1.0f)).xyz;

    float alpha;
    float3 intersectPositionWS;
    if (bIsSphere)
    {
      float3 probeInfluencePosition = probeProjectionPosition;
      probeInfluencePosition -= probeData.InfluenceShift.xyz;
      probeInfluencePosition /= probeData.InfluenceScale.xyz;
      // return probeInfluencePosition;
      //  Boundary clamp. For spheres projection and influence space are identical.
      float dist = length(probeInfluencePosition);
      if (dist > 1.0f)
        continue;

      // Compute falloff alpha
      alpha = saturate((1.0f - dist) / probeData.PositiveFalloff.x);

      {
        // Intersection with sphere, convert to unit sphere space
        // Transform in local unit parallax sphere (scaled and rotated)
        float3 probeProjectionReflDirection = mul(worldToProbeNormalMatrix, reflDirectionWS);
        float reflDirectionLength = length(probeProjectionReflDirection);
        probeProjectionReflDirection /= reflDirectionLength;

        // https://en.wikipedia.org/wiki/Line%E2%80%93sphere_intersection
        // As the ray dir is normalized and the sphere is radius 1 and at pos 0,0,0 - we can optimize the quadratic equation ad^2 + bd + c = 0.
        const float b = dot(probeProjectionReflDirection, probeProjectionPosition);
        const float c = dot(probeProjectionPosition, probeProjectionPosition) - 1;
        // We ignore the determinant sign check (determinant >= 0 for hits) as we know that we are always within the sphere and thus always hit.
        const float determinant = b * b - c;

        // The optimized ray-sphere intersection above only works with the normalized probeProjectionReflDirection.
        // Thus, we need to adjust the length when applying in world space.
        const float distance = (sqrt(determinant) - b) / reflDirectionLength;
        intersectPositionWS = matData.worldPosition + distance * reflDirectionWS;
      }
    }
    else
    {
      float3 probeInfluencePosition = probeProjectionPosition;
      probeInfluencePosition -= probeData.InfluenceShift.xyz;
      probeInfluencePosition /= probeData.InfluenceScale.xyz;

      // Boundary clamp. Check whether the pixel position in probe influence space is inside the [-1, 1] cube.
      {
        float3 dist = abs(probeInfluencePosition);
        float maxDist = max(dist.x, max(dist.y, dist.z));
        if (maxDist > 1.0f)
          continue;
      }
      const float3 unitary = float3(1.0f, 1.0f, 1.0f);

      // Compute falloff alpha
      {
        // Compute min distance from all planes.
        float3 firstPlaneDist = (unitary - probeInfluencePosition);
        float3 secondPlaneDist = (unitary + probeInfluencePosition);

        float3 positiveFalloff = firstPlaneDist / probeData.PositiveFalloff.xyz;
        float3 negativeFalloff = secondPlaneDist / probeData.NegativeFalloff.xyz;
        alpha = saturate(min(
          min(negativeFalloff.x, min(negativeFalloff.y, negativeFalloff.z)),
          min(positiveFalloff.x, min(positiveFalloff.y, positiveFalloff.z))));
      }

      // Box projection taken from: https://seblagarde.wordpress.com/2012/09/29/image-based-lighting-approaches-and-parallax-corrected-cubemap/
      {
        // Intersection with OBB convert to unit box space
        // Transform in local unit parallax cube space (scaled and rotated)
        float3 probeProjectionReflDirection = mul(worldToProbeNormalMatrix, reflDirectionWS);

        float3 firstPlaneDist = (unitary - probeProjectionPosition);
        float3 secondPlaneDist = (unitary + probeProjectionPosition);

        float3 firstPlaneIntersect = firstPlaneDist / probeProjectionReflDirection;
        float3 secondPlaneIntersect = -secondPlaneDist / probeProjectionReflDirection;
        float3 furthestPlane = max(firstPlaneIntersect, secondPlaneIntersect);
        float distance = min(furthestPlane.x, min(furthestPlane.y, furthestPlane.z));

        // Use Distance in WS directly to recover intersection.
        intersectPositionWS = positionWS + reflDirectionWS * distance;
      }
    }

    // The blob post above assumes that the cube maps are always rendered from world space without any rotation
    // in the rendering of the cube map itself. However, we do rotate the rendering of the cube maps so we can
    // correctly clamp the far plane for each side of the cube. Thus, we can't take the world space dir and use
    // it as a reflection lookup. We need to transform it into the cube map space. However, the image in the cube
    // map is not squished according to the AABB so we ONLY need to apply the rotational part of the transform,
    // ignoring scale.
    const float3 cubemapPositionWS = probeData.ProbePosition.xyz;
    const float3 reflectionDir = bIsProjected ? CubeMapDirection(mul(worldToProbeCubeMapNormalMatrix, intersectPositionWS - cubemapPositionWS).xyz) : CubeMapDirection(reflDirectionWS);
    const float roughness = computeDistanceBaseRoughness(length(intersectPositionWS - matData.worldPosition), length(intersectPositionWS - cubemapPositionWS), matData.roughness);
    // Sample the cube map
    const float4 coord = float4(reflectionDir, index);
    const float mipLevel = MipLevelFromRoughness(roughness, NUM_REFLECTION_MIPS);
    float4 indirectLight = ReflectionSpecularTexture.SampleLevel(LinearSampler, coord, mipLevel) * alpha;

    // Accumulate contribution.
    const float remainingWeight = 1.0f - ref.w;
    ref += indirectLight * remainingWeight;

    if (ref.a >= 1.0f)
      return ref.rgb;
  }

  // Sample global fallback probe.
  float3 reflectionDir = CubeMapDirection(reflDirectionWS);
  float4 coord = float4(reflectionDir, 0);
  float mipLevel = MipLevelFromRoughness(matData.roughness, NUM_REFLECTION_MIPS);
  float4 indirectLight = ReflectionSpecularTexture.SampleLevel(LinearSampler, coord, mipLevel);
  ref += indirectLight * (1.0f - ref.w);

  return ref.rgb;
}

// Returns unsaturated NdotL
float EvaluatePBRLight(float3 worldPosition, float3 worldNormal, ezPerLightData lightData, uint type, out float3 lightVector, out float attenuation, out float distanceToLight)
{
  float3 lightDir = GetLightDirection(lightData);
  lightVector = lightDir;
  attenuation = 1.0;
  distanceToLight = 1.0;

  [branch] if (type <= LIGHT_TYPE_SPOT)
  {
    lightVector = lightData.position - worldPosition;
    float sqrDistance = dot(lightVector, lightVector);

    attenuation = DistanceAttenuation(sqrDistance, lightData.invSqrAttRadius);

    distanceToLight = sqrDistance * lightData.invSqrAttRadius;
    lightVector *= rsqrt(sqrDistance);

    if (type == LIGHT_TYPE_SPOT)
    {
      float2 spotParams = RG16FToFloat2(lightData.spotOrFillParams);
      attenuation *= SpotAttenuation(lightVector, lightDir, spotParams);
    }
  }

  float NdotL = dot(worldNormal, lightVector);
  return NdotL;
}

void EvaluateFillLight(float3 worldPosition, float3 worldNormal, float3 diffuseColor, float directionality, ezPerLightData lightData, uint type, inout float3 diffuseLight, inout float3 indirectLightModulation)
{
  float distanceToLight = 1.0f;
  float3 lightVector = NormalizeAndGetLength(lightData.position - worldPosition, distanceToLight);

  float attenuation = saturate(1.0 - distanceToLight * lightData.invSqrAttRadius);

  float2 fillParams = RG16FToFloat2(lightData.spotOrFillParams);
  attenuation = pow(attenuation, fillParams.x);

  directionality = min(directionality, fillParams.y);
  float NdotL = dot(worldNormal, lightVector);
  attenuation *= saturate(lerp(1.0, NdotL, directionality));
  attenuation *= lightData.intensity;

  float3 lightColor = GetLightColor(lightData);
  if (type == LIGHT_TYPE_FILL_ADDITIVE)
  {
    diffuseLight += diffuseColor * lightColor * attenuation;
  }
  else if (type == LIGHT_TYPE_FILL_MODULATE_INDIRECT)
  {
    indirectLightModulation *= max(lerp(1.0, lightColor, attenuation), 0.0);
  }
}

AccumulatedLight CalculateLighting(ezMaterialData matData, ezPerClusterData clusterData, float3 screenPosition, bool applySSAO)
{
  float3 viewVector = normalize(GetCameraPosition() - matData.worldPosition);

  AccumulatedLight totalLight = InitializeLight(0.0f, 0.0f);
  float3 indirectLightModulation = 1.0f;

  float noise = InterleavedGradientNoise(screenPosition.xy);
  float2 randomAngle;
  sincos(noise * 2.0f * PI, randomAngle.x, randomAngle.y);
  float2x2 randomRotation = {randomAngle.x, -randomAngle.y, randomAngle.y, randomAngle.x};

  uint firstItemIndex = clusterData.offset;
  uint lastItemIndex = firstItemIndex + GET_LIGHT_INDEX(clusterData.counts);

  [loop] for (uint i = firstItemIndex; i < lastItemIndex; ++i)
  {
    uint itemIndex = clusterItemBuffer[i];
    uint lightIndex = GET_LIGHT_INDEX(itemIndex);

    ezPerLightData lightData = perLightDataBuffer[lightIndex];
    uint type = GetLightType(lightData);

    [branch] if (type <= LIGHT_TYPE_DIR)
    {
      float3 lightVector;
      float attenuation = 1.0;
      float distanceToLight = 1.0;
      float NdotL = saturate(EvaluatePBRLight(matData.worldPosition, matData.worldNormal, lightData, type, lightVector, attenuation, distanceToLight));

#if !defined(USE_MATERIAL_SUBSURFACE_COLOR)
      [branch] if (attenuation * NdotL > 0.0f)
#endif
      {
        attenuation *= MicroShadow(matData.occlusion, matData.worldNormal, lightVector);

        float3 debugColor = 1.0f;
        float shadowTerm = 1.0;
        float subsurfaceShadow = 1.0;

        [branch] if (lightData.shadowDataOffsetAndFadeOut != 0)
        {
          float extraPenumbraScale = 1.0;

          shadowTerm = CalculateShadowTerm(matData.worldPosition, matData.vertexNormal, lightVector, distanceToLight, type, lightData.shadowDataOffsetAndFadeOut, noise, randomRotation, extraPenumbraScale, subsurfaceShadow, debugColor);
        }

        attenuation *= lightData.intensity;
        float3 lightColor = GetLightColor(lightData);

        if (lightData.cookieParams0 != 0)
        {
          lightColor *= SampleLightCookie(lightData, matData.worldPosition);
        }

        // debug cascade or point face selection
#if 0
        lightColor = lerp(1.0f, debugColor, 0.5f);
#endif

        AccumulateLight(totalLight, DefaultShading(matData, lightVector, viewVector), lightColor * (attenuation * shadowTerm), lightData.specularMultiplier);

#if defined(USE_MATERIAL_SUBSURFACE_COLOR)
        AccumulateLight(totalLight, SubsurfaceShading(matData, lightVector, viewVector), lightColor * (attenuation * subsurfaceShadow));
#endif
      }
    }
    else // Fill Light
    {
      EvaluateFillLight(matData.worldPosition, matData.worldNormal, matData.diffuseColor, 1.0, lightData, type, totalLight.diffuseLight, indirectLightModulation);
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
  totalLight.diffuseLight += matData.diffuseColor * indirectLightModulation * skyLight * occlusion;

  // indirect specular
  totalLight.specularLight += matData.specularColor * indirectLightModulation * ComputeReflection(matData, viewVector, clusterData) * occlusion;

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

  [loop] for (uint i = firstItemIndex; i < lastItemIndex; ++i)
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
    // fade *= min(borderFade.x, min(borderFade.y, borderFade.z));
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

        [branch] if (decalFlags & DECAL_MAP_NORMAL_TO_GEOMETRY)
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
  float3 refractionColor = SceneColor.SampleLevel(SceneColorSampler, float3(refractCoords, s_ActiveCameraEyeIndex), 0.0f).rgb;

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

float3 ApplyFog(float3 color, float3 worldPosition, float fogAmount)
{
  float3 fogColor = FogColor.rgb;
  if (FogInvSkyDistance > 0.0)
  {
    float distance = 0;
    float3 viewVector = NormalizeAndGetLength(worldPosition - GetCameraPosition(), distance);
    float4 coord = float4(CubeMapDirection(viewVector), 0);
    float mipLevel = saturate(1.0 - distance * FogInvSkyDistance) * NUM_REFLECTION_MIPS;
    fogColor *= ReflectionSpecularTexture.SampleLevel(LinearSampler, coord, mipLevel).rgb * 2.0;
  }

  return lerp(fogColor, color, fogAmount);
}

float3 ApplyFog(float3 color, float3 worldPosition)
{
  if (FogDensity > 0.0)
  {
    return ApplyFog(color, worldPosition, GetFogAmount(worldPosition));
  }

  return color;
}
