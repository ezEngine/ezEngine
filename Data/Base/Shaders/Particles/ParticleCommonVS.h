#pragma once

#define SHADOW_FORCE_LAST_CASCADE

#include <Shaders/Common/GlobalConstants.h>
#include <Shaders/Materials/MaterialInterpolator.h>
#include <Shaders/Particles/ParticleSystemConstants.h>
#if SHADING_QUALITY == SHADING_QUALITY_NORMAL
#  include <Shaders/Common/Lighting.h>
#elif SHADING_QUALITY == SHADING_QUALITY_SIMPLIFIED
#  include <Shaders/Common/LightingSimplified.h>
#else
#  error "Unknown shading quality configuration."
#endif

static float2 QuadTexCoords[6] =
  {
    float2(0.0, 0.0),
    float2(1.0, 0.0),
    float2(1.0, 1.0),
    float2(0.0, 0.0),
    float2(1.0, 1.0),
    float2(0.0, 1.0),
};

uint CalcQuadParticleDataIndex(uint VertexID)
{
  return VertexID / 6;
}

uint CalcQuadParticleVertexIndex(uint VertexID)
{
  return VertexID % 6;
}

struct Quad
{
  float4 worldPosition;
  float4 screenPosition;
  float3 normal;
};

Quad CalcQuadOutputPositionWithTangents(uint vertexIndex, float3 inPosition, float3 inTangentX, float3 inTangentZ, float inSize)
{
  float3 offsetRight = inTangentX * ((QuadTexCoords[vertexIndex].x - 0.5) * inSize);
  float3 offsetUp = inTangentZ * ((QuadTexCoords[vertexIndex].y - 0.5) * -inSize);

  Quad quad;
  quad.worldPosition = mul(ObjectToWorldMatrix, float4(inPosition + offsetRight + offsetUp, 1));
  quad.screenPosition = mul(GetWorldToScreenMatrix(), quad.worldPosition);

  float3 centerNormal = normalize(mul((float3x3)ObjectToWorldMatrix, cross(inTangentZ, inTangentX)));
  float3 cornerNormal = normalize(quad.worldPosition.xyz - inPosition);
  quad.normal = normalize(lerp(centerNormal, cornerNormal, NormalCurvature));

  return quad;
}

Quad CalcQuadOutputPositionWithAlignedAxis(uint vertexIndex, float3 inPosition, float3 inTangentX, float3 inTangentZ, float inSize)
{
  float stretch = -inTangentZ.x;

  float3 axisDir = normalize(inTangentX);
  float3 orthoDir = normalize(cross(inTangentX, GetCameraDirForwards()));

  float3 offsetRight = orthoDir * ((QuadTexCoords[vertexIndex].x - 0.5) * inSize);
  float3 offsetUp = axisDir * ((1.0 - QuadTexCoords[vertexIndex].y) * inSize * stretch);

  Quad quad;
  quad.worldPosition = mul(ObjectToWorldMatrix, float4(inPosition + offsetRight + offsetUp, 1));
  quad.screenPosition = mul(GetWorldToScreenMatrix(), quad.worldPosition);

  float3 centerNormal = cross(axisDir, orthoDir);
  float3 cornerNormal = normalize(offsetRight);
  quad.normal = normalize(mul((float3x3)ObjectToWorldMatrix, lerp(centerNormal, cornerNormal, NormalCurvature)));

  return quad;
}

Quad CalcQuadOutputPositionAsBillboard(uint vertexIndex, float3 inPosition, float rotationOffset, float rotationSpeed, float inSize)
{
  float2 angles;
  sincos(rotationOffset + rotationSpeed * TotalEffectLifeTime, angles.x, angles.y);
  float2x2 rotation = {angles.x, -angles.y, angles.y, angles.x};

  float2 quadCorners = mul(rotation, QuadTexCoords[vertexIndex] - 0.5);

  float3 offsetRight = GetCameraDirRight() * (quadCorners.x * inSize);
  float3 offsetUp = GetCameraDirUp() * (quadCorners.y * -inSize);

  Quad quad;
  quad.worldPosition = mul(ObjectToWorldMatrix, float4(inPosition, 1)) + float4(offsetRight + offsetUp, 0);
  quad.screenPosition = mul(GetWorldToScreenMatrix(), quad.worldPosition);

  float3 cornerNormal = normalize(quad.worldPosition.xyz - inPosition);
  quad.normal = normalize(lerp(-GetCameraDirForwards(), cornerNormal, NormalCurvature));

  return quad;
}

float4 ComputeTextureAtlasRect(uint numVarsX, uint numVarsY, float varLerp, float4 texCoordOffsetAndSize)
{
  if (numVarsX > 1 || numVarsY > 1)
  {
    uint numVars = numVarsX * numVarsY;
    uint idxVar = (uint)(numVars * varLerp);
    uint varY = idxVar / numVarsX;
    uint varX = (idxVar - (varY * numVarsX));
    texCoordOffsetAndSize.zw = texCoordOffsetAndSize.zw / float2(numVarsX, numVarsY);
    texCoordOffsetAndSize.xy = texCoordOffsetAndSize.xy + texCoordOffsetAndSize.zw * float2(varX, varY);
  }

  return texCoordOffsetAndSize;
}

float4 ComputeAtlasRectRandomAnimated(uint numVarsX, uint numVarsY, float varLerp, uint numAnimsX, uint numAnimsY, float animLerp)
{
  float4 texCoordOffsetAndSize = float4(0, 0, 1, 1);

  texCoordOffsetAndSize = ComputeTextureAtlasRect(numVarsX, numVarsY, varLerp, texCoordOffsetAndSize);
  texCoordOffsetAndSize = ComputeTextureAtlasRect(numAnimsX, numAnimsY, animLerp, texCoordOffsetAndSize);

  return texCoordOffsetAndSize;
}

float2 ComputeAtlasTexCoordRandomAnimated(float2 baseTexCoord, uint numVarsX, uint numVarsY, float varLerp, uint numAnimsX, uint numAnimsY, float animLerp)
{
  float4 texCoordOffsetAndSize = ComputeAtlasRectRandomAnimated(numVarsX, numVarsY, varLerp, numAnimsX, numAnimsY, animLerp);

  return texCoordOffsetAndSize.xy + baseTexCoord * texCoordOffsetAndSize.zw;
}

float3 CalculateParticleLighting(float4 screenPosition, float3 worldPosition, float3 normal)
{
  float3 totalLight = 0.0f;

#if SHADING_QUALITY == SHADING_QUALITY_NORMAL
  float2 normalizedScreenPos = (screenPosition.xy / screenPosition.w) * float2(0.5, -0.5) + 0.5;
  float3 screenPos = float3(normalizedScreenPos * ViewportSize.xy, screenPosition.w);
  ezPerClusterData clusterData = GetClusterData(screenPos);

  float3 viewVector = normalize(GetCameraPosition() - worldPosition);

  uint firstItemIndex = clusterData.offset;
  uint lastItemIndex = firstItemIndex + GET_LIGHT_INDEX(clusterData.counts);

  [loop] for (uint i = firstItemIndex; i < lastItemIndex; ++i)
  {
    uint itemIndex = clusterItemBuffer[i];
    uint lightIndex = GET_LIGHT_INDEX(itemIndex);

    ezPerLightData lightData = perLightDataBuffer[lightIndex];
    uint type = (lightData.colorAndType >> 24) & 0xFF;

    float3 lightDir = normalize(RGB10ToFloat3(lightData.direction) * 2.0f - 1.0f);
    float3 lightVector = lightDir;
    float attenuation = 1.0f;
    float distanceToLight = 1.0f;

    [branch] if (type != LIGHT_TYPE_DIR)
    {
      lightVector = lightData.position - worldPosition;
      float sqrDistance = dot(lightVector, lightVector);

      attenuation = DistanceAttenuation(sqrDistance, lightData.invSqrAttRadius);

      distanceToLight = sqrDistance * lightData.invSqrAttRadius;
      lightVector *= rsqrt(sqrDistance);

      [branch] if (type == LIGHT_TYPE_SPOT)
      {
        float2 spotParams = RG16FToFloat2(lightData.spotParams);
        attenuation *= SpotAttenuation(lightVector, lightDir, spotParams);
      }
    }

    float nDotL = dot(normal, lightVector);
    attenuation *= saturate(lerp(1, nDotL, LightDirectionality));

    [branch] if (attenuation > 0.0f)
    {
      float3 debugColor = 1.0f;
      float shadowTerm = 1.0;
      float subsurfaceShadow = 1.0;

      [branch] if (lightData.shadowDataOffset != 0xFFFFFFFF)
      {
        uint shadowDataOffset = lightData.shadowDataOffset;

        // Zero normal effectively disables normal offset bias. Since our particles may have curved normals the
        // normal offset bias can create weird artifacts and we don't need this bias on particles as they can't have self-shadow issues.
        // Our usual noise also doesn't make sense with vertex lighting so we use a fixed shadow filter kernel rotation here.
        float3 vertexNormal = float3(0, 0, 0);
        float noise = 0.0;
        float2x2 fixedRotation = {1, 0, 0, 1};
        float extraPenumbraScale = 4.0;

        shadowTerm = CalculateShadowTerm(worldPosition, vertexNormal, lightVector, distanceToLight, type,
          shadowDataOffset, noise, fixedRotation, extraPenumbraScale, subsurfaceShadow, debugColor);
      }

      attenuation *= lightData.intensity;
      float3 lightColor = RGB8ToFloat3(lightData.colorAndType);

      totalLight += lightColor * (attenuation * shadowTerm);
    }
  }

  // normalize brdf
  totalLight *= (1.0f / PI);
#endif

  // sky light in ambient cube basis
  float3 skyLight = EvaluateAmbientCube(SkyIrradianceTexture, SkyIrradianceIndex, normal).rgb;
  totalLight += skyLight;

  return totalLight;
}
