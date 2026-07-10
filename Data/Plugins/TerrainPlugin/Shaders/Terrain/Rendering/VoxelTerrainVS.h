#pragma once

/// Vertex shader helpers for the voxel mesh terrain. Include this in the [VERTEXSHADER] section.
///
/// Requires: USE_WORLDPOS and USE_DATAOFFSETS defined before this file is included (they are
/// consumed by the MaterialInterpolator.h that this file includes).
/// Declares FillVoxelTerrainVertexOutput(uint vertexID).

#include <Shaders/Common/GlobalConstants.h>
#include <Shaders/Common/ObjectConstants.h>
#include <Shaders/Materials/MaterialInterpolator.h>
#include <Shaders/Terrain/Generation/VoxelMeshConstants.h>
#include <Shaders/Terrain/Rendering/VoxelMeshRenderConstants.h>

/// Vertex buffer written by surface nets shaders; read as SRV using the index from VoxelIndices.
StructuredBuffer<VoxelGpuVertex> VoxelVertices BIND_GROUP(BG_DRAW_CALL);

/// Index buffer written by surface nets shaders; emulates indexed drawing without a real index buffer.
StructuredBuffer<uint> VoxelIndices BIND_GROUP(BG_DRAW_CALL);

VS_OUT FillVoxelTerrainVertexOutput(uint vertexID)
{
  // Emulate indexed drawing: look up the real vertex index, then fetch the vertex.
  const uint idx = VoxelIndices[vertexID];
  const VoxelGpuVertex v = VoxelVertices[idx];

  const ezPerInstanceData instanceData = perInstanceData[GET_PUSH_CONSTANT(VoxelMeshRenderConstants, InstanceDataOffset)];
  const float4x4 objectToWorld = TransformToMatrix(instanceData.ObjectToWorld);
  const float3x3 objectToWorldNormal = TransformToRotation(instanceData.ObjectToWorldNormal);

  const float3 worldPos = mul(objectToWorld, float4(v.Position, 1.0f)).xyz;
  const float3 worldNormal = normalize(mul(objectToWorldNormal, v.Normal));
  const float3 worldTangent = normalize(mul(objectToWorldNormal, v.Tangent));
  const float3 worldBitan = cross(worldNormal, worldTangent) * v.BitangentSign;

  VS_OUT Output;
  Output.Position = mul(GetWorldToScreenMatrix(), float4(worldPos, 1.0f));
  Output.WorldPosition = worldPos;
  Output.Normal = worldNormal;
  Output.Tangent = worldTangent;
  Output.BiTangent = worldBitan;
  Output.TexCoord0 = float2(v.MaterialStrength, 0.0f);
  Output.DataOffsets = uint3(GET_PUSH_CONSTANT(VoxelMeshRenderConstants, InstanceDataOffset), v.Material, 0);
  return Output;
}
