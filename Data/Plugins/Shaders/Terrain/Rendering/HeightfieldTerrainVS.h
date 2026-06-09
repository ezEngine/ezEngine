#pragma once

/// Vertex shader helpers for the heightfield terrain. Include this in the [VERTEXSHADER] section.
///
/// Requires: USE_WORLDPOS, USE_DATAOFFSETS, and CUSTOM_INTERPOLATOR defined before this file is
/// included (they are consumed by MaterialInterpolator.h). The caller must define:
///   #define CUSTOM_INTERPOLATOR float2 MatWeightsHi : TEXCOORD2;
/// which carries w2 and w3 to the pixel shader alongside w0 and w1 in TexCoord0.
/// Declares FillHeightfieldTerrainVertexOutput(uint vertexID).

#include <Shaders/Common/GlobalConstants.h>
#include <Shaders/Common/ObjectConstants.h>
#include <Shaders/Materials/MaterialInterpolator.h>
#include <Shaders/Terrain/Rendering/HeightfieldRenderConstants.h>

/// Baked heights produced by the compute shader, one float per grid vertex.
StructuredBuffer<float> TerrainHeights BIND_GROUP(BG_DRAW_CALL);

/// Per-vertex packed normals from TerrainNormalsCS: XY as 16-bit floats in a uint32.
StructuredBuffer<uint> TerrainNormals BIND_GROUP(BG_DRAW_CALL);

/// Per-cell top-4 explicit material indices baked by Step3 (uint per rendered cell).
/// Packing: mat0|mat1<<8|mat2<<16|mat3<<24, ordered by descending brush weight (slot 0 = strongest).
StructuredBuffer<uint> TerrainCellMaterials BIND_GROUP(BG_DRAW_CALL);

/// Per-cell-corner blend weights baked by Step3 (uint per cell-corner, CellsPerSide²×4 entries).
/// Layout: cellIndex*4 + cornerSlot (TL=0,TR=1,BL=2,BR=3).
/// Packing: w0|w1<<8|w2<<16|w3<<24 (8-bit unorm). Fallback weight = max(0, 1-w0-w1-w2-w3).
/// Sentinel 0xFFFFFFFF marks a carved corner; the VS outputs NaN for those vertices.
StructuredBuffer<uint> TerrainWeights BIND_GROUP(BG_DRAW_CALL);

/// Decode a packed terrain normal (XY as f16, Z reconstructed).
float3 DecodeTerrainNormal(uint packed)
{
  float nx = f16tof32(packed & 0xFFFFu);
  float ny = f16tof32(packed >> 16u);
  float nz = sqrt(max(0.0, 1.0 - nx * nx - ny * ny));
  return float3(nx, ny, nz);
}

VS_OUT FillHeightfieldTerrainVertexOutput(uint vertexID)
{
  // Decode cell and corner from vertexID.
  // Each cell (quad) is rendered as 2 triangles = 6 vertices.
  // Triangle 0: corners (0,0),(1,0),(0,1)
  // Triangle 1: corners (1,0),(1,1),(0,1)
  static const uint2 QuadOffsets[6] =
    {
      {0, 0},
      {1, 0},
      {0, 1},
      {1, 0},
      {1, 1},
      {0, 1},
    };

  // Corner slot in the per-cell-corner weight buffer: TL=0, TR=1, BL=2, BR=3.
  // Maps QuadOffsets[vertInCell].x + QuadOffsets[vertInCell].y * 2.
  static const uint CornerSlot[6] = {0, 1, 2, 1, 3, 2};

  const uint cellIndex = vertexID / 6;
  const uint vertInCell = vertexID % 6;

  const uint cellX = cellIndex % GET_PUSH_CONSTANT(HeightfieldRenderConstants, CellsPerSide);
  const uint cellY = cellIndex / GET_PUSH_CONSTANT(HeightfieldRenderConstants, CellsPerSide);

  // lx/ly: 0-based vertex coordinates within the render grid.
  const uint lx = cellX + QuadOffsets[vertInCell].x;
  const uint ly = cellY + QuadOffsets[vertInCell].y;

  const uint idx = GET_PUSH_CONSTANT(HeightfieldRenderConstants, FirstVertexIdx) + ly * GET_PUSH_CONSTANT(HeightfieldRenderConstants, VertexIdxPitch) + lx;

  // Read per-cell-corner weights (unique slot per corner per cell — no sharing between cells).
  const uint vtxWeightPacked = TerrainWeights[cellIndex * 4u + CornerSlot[vertInCell]];

  // Carve sentinel: Step3 writes 0xFFFFFFFF for carved corners.
  // NaN position causes the spec to cull any triangle touching this vertex.
  if (vtxWeightPacked == 0xFFFFFFFFu)
  {
    VS_OUT Discarded = (VS_OUT)0;
    const float fNaN = asfloat(0x7FC00000u);
    Discarded.Position = float4(fNaN, fNaN, fNaN, fNaN);
    return Discarded;
  }

  float h = TerrainHeights[idx];
  float3 localNormal = DecodeTerrainNormal(TerrainNormals[idx]);

  // Apply the object-to-world transform so the patch respects rotation and scale.
  const ezPerInstanceData instanceData = perInstanceData[GET_PUSH_CONSTANT(HeightfieldRenderConstants, InstanceDataOffset)];
  const float4x4 objectToWorld = TransformToMatrix(instanceData.ObjectToWorld);
  const float3x3 objectToWorldNormal = TransformToRotation(instanceData.ObjectToWorldNormal);

  const float3 localPos = float3((float)lx * GET_PUSH_CONSTANT(HeightfieldRenderConstants, GridSpacing), (float)ly * GET_PUSH_CONSTANT(HeightfieldRenderConstants, GridSpacing), h);

  // Analytical tangent for a heightfield: lies in the XZ plane, perpendicular to the normal.
  // Derived from dP/du = (GridSpacing, 0, dH/dx), orthogonalized: float3(n.z, 0, -n.x).
  const float3 localTangent = normalize(float3(localNormal.z, 0.0, -localNormal.x));
  const float3 localBitangent = cross(localNormal, localTangent);

  const float3 worldPos = mul(objectToWorld, float4(localPos, 1.0f)).xyz;
  const float3 worldNormal = normalize(mul(objectToWorldNormal, localNormal));
  const float3 worldTangent = normalize(mul(objectToWorldNormal, localTangent));
  const float3 worldBitangent = normalize(mul(objectToWorldNormal, localBitangent));

  // Unpack 4 per-vertex weights (8-bit unorm) relative to the cell's top-4 explicit material set.
  const float w0 = (vtxWeightPacked & 0xFFu) * (1.0 / 255.0);
  const float w1 = ((vtxWeightPacked >> 8) & 0xFFu) * (1.0 / 255.0);
  const float w2 = ((vtxWeightPacked >> 16) & 0xFFu) * (1.0 / 255.0);
  const float w3 = ((vtxWeightPacked >> 24) & 0xFFu) * (1.0 / 255.0);

  VS_OUT Output;
  Output.Position = mul(GetWorldToScreenMatrix(), float4(worldPos, 1.0f));
  Output.WorldPosition = worldPos;
  Output.Normal = worldNormal;
  Output.Tangent = worldTangent;
  Output.BiTangent = worldBitangent;

  // TexCoord0: w0, w1. MatWeightsHi: w2, w3 (via CUSTOM_INTERPOLATOR).
  // The PS reconstructs w_fallback = max(0, 1 - w0 - w1 - w2 - w3).
  Output.TexCoord0 = float2(w0, w1);
  Output.MatWeightsHi = float2(w2, w3);

  // DataOffsets.x: instance data offset.
  // DataOffsets.y: 4 cell-level material indices — identical for all 6 vertices of this cell.
  Output.DataOffsets = uint3(
    GET_PUSH_CONSTANT(HeightfieldRenderConstants, InstanceDataOffset),
    TerrainCellMaterials[cellIndex],
    0u);
  return Output;
}
