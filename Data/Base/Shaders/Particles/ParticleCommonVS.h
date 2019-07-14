#pragma once

#define USE_TEXCOORD0
#define USE_COLOR0

#include <Shaders/Common/GlobalConstants.h>
#include <Shaders/Materials/MaterialInterpolator.h>
#include <Shaders/Particles/ParticleSystemConstants.h>
#include <Shaders/Common/Lighting.h>

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

float4 CalcQuadOutputPositionWithTangents(uint vertexIndex, float3 inPosition, float3 inTangentX, float3 inTangentZ, float inSize)
{
  float4 position = float4(inPosition, 1);

  float4 tangentX = float4(inTangentX, 0);
  float4 tangentZ = float4(inTangentZ, 0);

  float4 offsetRight = tangentX * (QuadTexCoords[vertexIndex].x - 0.5) * inSize;
  float4 offsetUp = tangentZ * (QuadTexCoords[vertexIndex].y - 0.5) * -inSize;

  float4 worldPosition = mul(ObjectToWorldMatrix, position + offsetRight + offsetUp);
  float4 screenPosition = mul(GetWorldToScreenMatrix(), worldPosition);

  return screenPosition;
}

float4 CalcQuadOutputPositionWithAlignedAxis(uint vertexIndex, float3 inPosition, float3 inTangentX, float3 inTangentZ, float inSize)
{
  float stretch = inTangentZ.x;

  float4 position = float4(inPosition, 1);

  float4 axisDir = float4(normalize(inTangentX), 0);
  float4 orthoDir = float4(normalize(cross(inTangentX, GetCameraDirForwards())), 0);

  float4 offsetRight = orthoDir * (QuadTexCoords[vertexIndex].x - 0.5) * inSize;
  //float4 offsetUp = axisDir * (0.5 - (QuadTexCoords[vertexIndex].y - 0.5)) * inSize * stretch;
  float4 offsetUp = axisDir * (1.0 - QuadTexCoords[vertexIndex].y) * inSize * stretch;

  float4 worldPosition = mul(ObjectToWorldMatrix, position + offsetRight + offsetUp);
  float4 screenPosition = mul(GetWorldToScreenMatrix(), worldPosition);

  return screenPosition;
}

float3x3 CreateRotationMatrixY(float radians)
{
  float fsin = sin(radians);
  float fcos = cos(radians);

  return float3x3(fcos, 0, fsin, 0, 1, 0, -fsin, 0, fcos);
}

float4 CalcQuadOutputPositionAsBillboard(uint vertexIndex, float3 centerPosition, float rotationOffset, float rotationSpeed, float inSize)
{
  //float4 position = TransformToPosition(inTransform);
  float4 position = float4(centerPosition, 1);
  //float3x3 rotation = TransformToRotation(inTransform);
  float3x3 rotation = CreateRotationMatrixY(rotationOffset + rotationSpeed * WorldTime);

  float3 offsetRight = GetCameraDirRight() * (QuadTexCoords[vertexIndex].x - 0.5) * inSize;
  float3 offsetUp = GetCameraDirUp() * (QuadTexCoords[vertexIndex].y - 0.5) * -inSize;

  float3 offsetRightCS = mul(GetWorldToCameraMatrix(), float4(offsetRight, 0)).xzy;
  float3 offsetUpCS = mul(GetWorldToCameraMatrix(), float4(offsetUp, 0)).xzy;

  offsetRightCS = mul(rotation, offsetRightCS);
  offsetUpCS = mul(rotation, offsetUpCS);

  float4 worldPosition = mul(ObjectToWorldMatrix, position);
  float4 cameraPosition = mul(GetWorldToCameraMatrix(), worldPosition);
  cameraPosition.xzy = cameraPosition.xzy + offsetRightCS + offsetUpCS;
  float4 screenPosition = mul(GetCameraToScreenMatrix(), cameraPosition);

  return screenPosition;
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

