#pragma once

#ifndef USE_WORLDPOS
#  define USE_WORLDPOS
#endif

#include <Shaders/Common/GlobalConstants.h>
#include <Shaders/Common/ObjectConstants.h>
#include <Shaders/Materials/MaterialInterpolator.h>

struct VS_GLOBALS
{
  VS_IN Input;
};
static VS_GLOBALS G;

#if defined(USE_OBJECT_POSITION_OFFSET)
float3 GetObjectPositionOffset(ezPerInstanceData data);
#endif

#if defined(USE_WORLD_POSITION_OFFSET)
float3 GetWorldPositionOffset(ezPerInstanceData data, float3 worldPosition);
#endif

#if defined(USE_VERTEX_DEPTH_BIAS)
float GetVertexDepthBias();
#endif

#if defined(USE_SKINNING)

float3 SkinPosition(float3 ObjectSpacePosition, float4 BoneWeights, uint4 BoneIndices)
{
  float3 OutPos = mul(TransformToMatrix(skinningTransforms[BoneIndices.x]), float4(ObjectSpacePosition, 1.0)).xyz * BoneWeights.x;
  OutPos += mul(TransformToMatrix(skinningTransforms[BoneIndices.y]), float4(ObjectSpacePosition, 1.0)).xyz * BoneWeights.y;
  OutPos += mul(TransformToMatrix(skinningTransforms[BoneIndices.z]), float4(ObjectSpacePosition, 1.0)).xyz * BoneWeights.z;
  OutPos += mul(TransformToMatrix(skinningTransforms[BoneIndices.w]), float4(ObjectSpacePosition, 1.0)).xyz * BoneWeights.w;

  return OutPos;
}

float3 SkinDirection(float3 ObjectSpaceDirection, float4 BoneWeights, uint4 BoneIndices)
{
  float3 OutDir = mul(TransformToRotation(skinningTransforms[BoneIndices.x]), ObjectSpaceDirection) * BoneWeights.x;
  OutDir += mul(TransformToRotation(skinningTransforms[BoneIndices.y]), ObjectSpaceDirection) * BoneWeights.y;
  OutDir += mul(TransformToRotation(skinningTransforms[BoneIndices.z]), ObjectSpaceDirection) * BoneWeights.z;
  OutDir += mul(TransformToRotation(skinningTransforms[BoneIndices.w]), ObjectSpaceDirection) * BoneWeights.w;

  return OutDir;
}

#endif

VS_OUT FillVertexData(VS_IN Input)
{
#if CAMERA_MODE == CAMERA_MODE_STEREO
  s_ActiveCameraEyeIndex = Input.InstanceID % 2;
#endif

  G.Input = Input;

  ezPerInstanceData data = GetInstanceData();

  float4x4 objectToWorld = TransformToMatrix(data.ObjectToWorld);
  float3x3 objectToWorldNormal = TransformToRotation(data.ObjectToWorldNormal);

  float3 objectPosition = Input.Position;
#if defined(USE_OBJECT_POSITION_OFFSET)
  objectPosition += GetObjectPositionOffset(data);
#endif

#if defined(USE_SKINNING)
  objectPosition = SkinPosition(objectPosition, Input.BoneWeights, Input.BoneIndices);
#endif

  VS_OUT Output;
  Output.WorldPosition = mul(objectToWorld, float4(objectPosition, 1.0)).xyz;
#if defined(USE_WORLD_POSITION_OFFSET)
  Output.WorldPosition += GetWorldPositionOffset(data, Output.WorldPosition);
#endif

  Output.Position = mul(GetWorldToScreenMatrix(), float4(Output.WorldPosition, 1.0));

#if defined(USE_VERTEX_DEPTH_BIAS)
  float depthBiasScale = 1.5f / (1 << 16);
  Output.Position.z += GetVertexDepthBias() * depthBiasScale * Output.Position.w;
#endif

  Output.Position.z = max(Output.Position.z, MaxZValue);

#if defined(USE_NORMAL)
  float3 normal = Input.Normal * 2.0 - 1.0;

#  if defined(USE_SKINNING)
  normal = SkinDirection(normal, Input.BoneWeights, Input.BoneIndices);
#  endif

  Output.Normal = normalize(mul(objectToWorldNormal, normal));
#endif

#if defined(USE_TANGENT)
  float3 tangent = Input.Tangent.xyz * 2.0 - 1.0;

#  if defined(USE_SKINNING)
  tangent = SkinDirection(tangent, Input.BoneWeights, Input.BoneIndices);
#  endif

  float handednessCorrection = Input.Tangent.w * 2.0 - 1.0;
  float3 bitangent = cross(normal, tangent) * handednessCorrection;

  Output.Tangent = normalize(mul(objectToWorldNormal, tangent));
  Output.BiTangent = normalize(mul(objectToWorldNormal, bitangent));
#endif

#if defined(USE_TEXCOORD0)
  Output.TexCoord0 = Input.TexCoord0;
#  if defined(USE_TEXCOORD1)
  Output.TexCoord1 = Input.TexCoord1;
#  endif
#endif

#if defined(USE_COLOR0)
  Output.Color0 = Input.Color0;
#  if defined(USE_COLOR1)
  Output.Color1 = Input.Color1;
#  endif
#endif

#if defined(USE_DEBUG_INTERPOLATOR)
  Output.DebugInterpolator = float4(0.1, 0.1, 0.1, 1.0);

#  if defined(USE_SKINNING)
  if (RenderPass == EDITOR_RENDER_PASS_BONE_WEIGHTS)
  {
    Output.DebugInterpolator = Input.BoneWeights;
  }
#  endif
#endif

  Output.InstanceID = Input.InstanceID;

#if defined(CAMERA_MODE)
#  if CAMERA_MODE == CAMERA_MODE_STEREO
#    if VERTEX_SHADER_RENDER_TARGET_ARRAY_INDEX == TRUE
  Output.RenderTargetArrayIndex = Input.InstanceID % 2;
#    endif
#  endif
#endif

  return Output;
}
