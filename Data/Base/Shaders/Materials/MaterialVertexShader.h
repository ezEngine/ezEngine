#pragma once

#define USE_WORLDPOS

#include <Shaders/Common/GlobalConstants.h>
#include <Shaders/Common/ObjectConstants.h>
#include <Shaders/Materials/MaterialInterpolator.h>

#if defined(USE_OBJECT_POSITION_OFFSET)
  float3 GetObjectPositionOffset(VS_IN Input, ezPerInstanceData data);
#endif

#if defined(USE_WORLD_POSITION_OFFSET)
  float3 GetWorldPositionOffset(VS_IN Input, ezPerInstanceData data, float3 worldPosition);
#endif

#if defined(USE_SKINNING)

float4 SkinPosition(float4 ObjectSpacePosition, float4 BoneWeights, uint4 BoneIndices)
{
  float4 OutPos  = mul(skinningMatrices[BoneIndices.x], ObjectSpacePosition) * BoneWeights.x;
         OutPos += mul(skinningMatrices[BoneIndices.y], ObjectSpacePosition) * BoneWeights.y;
         OutPos += mul(skinningMatrices[BoneIndices.z], ObjectSpacePosition) * BoneWeights.z;
         OutPos += mul(skinningMatrices[BoneIndices.w], ObjectSpacePosition) * BoneWeights.w;

  return OutPos;
}

float3 SkinDirection(float3 ObjectSpaceDirection, float4 BoneWeights, uint4 BoneIndices)
{
  float3 OutDir  = mul((float3x3)skinningMatrices[BoneIndices.x], ObjectSpaceDirection) * BoneWeights.x;
         OutDir += mul((float3x3)skinningMatrices[BoneIndices.y], ObjectSpaceDirection) * BoneWeights.y;
         OutDir += mul((float3x3)skinningMatrices[BoneIndices.z], ObjectSpaceDirection) * BoneWeights.z;
         OutDir += mul((float3x3)skinningMatrices[BoneIndices.w], ObjectSpaceDirection) * BoneWeights.w;

  return OutDir;
}

#endif

VS_OUT FillVertexData(VS_IN Input)
{
#if CAMERA_MODE == CAMERA_MODE_STEREO
  s_ActiveCameraEyeIndex = Input.InstanceID % 2;
#endif

  ezPerInstanceData data = GetInstanceData(Input);

  float4x4 objectToWorld = TransformToMatrix(data.ObjectToWorld);
  float3x3 objectToWorldNormal = TransformToRotation(data.ObjectToWorldNormal);

  float3 objectPosition = Input.Position;
  #if defined(USE_OBJECT_POSITION_OFFSET)
    objectPosition += GetObjectPositionOffset(Input, data);
  #endif

  float4 objPos = float4(objectPosition, 1.0);

  #if defined(USE_SKINNING)
  objPos = SkinPosition(objPos, Input.BoneWeights, Input.BoneIndices);
  #endif

  VS_OUT Output;
  Output.WorldPosition = mul(objectToWorld, objPos).xyz;
  #if defined(USE_WORLD_POSITION_OFFSET)
    Output.WorldPosition += GetWorldPositionOffset(Input, data, Output.WorldPosition);
  #endif

  Output.Position = mul(GetWorldToScreenMatrix(), float4(Output.WorldPosition, 1.0));

  #if defined(USE_NORMAL)

    float3 normal = Input.Normal;

    #if defined(USE_SKINNING)
    normal = SkinDirection(normal, Input.BoneWeights, Input.BoneIndices);
    #endif

    Output.Normal = normalize(mul(objectToWorldNormal, normal));
  #endif

  #if defined(USE_TANGENT)
    float3 tangent = Input.Tangent;
    float handednessCorrection = 2.0f - dot(tangent, tangent);
    float3 biTangent = cross(Input.Normal, tangent) * handednessCorrection;

    #if defined(USE_SKINNING)
      tangent = SkinDirection(tangent, Input.BoneWeights, Input.BoneIndices);
      biTangent = SkinDirection(biTangent, Input.BoneWeights, Input.BoneIndices);
    #endif

    Output.Tangent = normalize(mul(objectToWorldNormal, tangent));
    Output.BiTangent = normalize(mul(objectToWorldNormal, biTangent));
  #endif

  #if defined(USE_TEXCOORD0)
    #if defined(USE_TEXCOORD1)
      Output.TexCoords = float4(Input.TexCoord0, Input.TexCoord1);
    #else
      Output.TexCoords = Input.TexCoord0;
    #endif
  #endif

  #if defined(USE_COLOR)
    Output.Color = Input.Color;
  #endif

  #if defined(USE_NORMAL) && (RENDER_PASS == RENDER_PASS_EDITOR)
    if (RenderPass == EDITOR_RENDER_PASS_BONE_WEIGHTS)
    {
      #if defined(USE_SKINNING)
        Output.Normal = Input.BoneWeights;
      #else
        Output.Normal = float4(0.1, 0.1, 0.1, 1.0);
      #endif
    }
  #endif

  Output.InstanceID = Input.InstanceID;

  return Output;
}
