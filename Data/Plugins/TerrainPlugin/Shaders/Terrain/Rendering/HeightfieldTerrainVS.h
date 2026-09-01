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

  const uint renderCells = GET_PUSH_CONSTANT(HeightfieldRenderConstants, RenderCellsPerSide);
  const int step = (int)GET_PUSH_CONSTANT(HeightfieldRenderConstants, VertexStep);
  const int skirt = (int)GET_PUSH_CONSTANT(HeightfieldRenderConstants, SkirtCells);
  const int cellsFull = (int)GET_PUSH_CONSTANT(HeightfieldRenderConstants, CellsPerSide);
  const int pitch = (int)GET_PUSH_CONSTANT(HeightfieldRenderConstants, VertexIdxPitch);

  const uint cellX = cellIndex % renderCells;
  const uint cellY = cellIndex / renderCells;

  // gx/gy: vertex coordinates within the rendered grid (0 .. renderCells), including skirt rings.
  const uint gx = cellX + QuadOffsets[vertInCell].x;
  const uint gy = cellY + QuadOffsets[vertInCell].y;

  // Buffer coordinate relative to the inner-grid origin. Negative / beyond [0, cellsFull] means skirt,
  // which reads into the 4-vertex border ring that the bake computed for normals.
  const int bufX = ((int)gx - skirt) * step;
  const int bufY = ((int)gy - skirt) * step;

  const bool isSkirt = (bufX < 0) || (bufX > cellsFull) || (bufY < 0) || (bufY > cellsFull);

  const int idx = (int)GET_PUSH_CONSTANT(HeightfieldRenderConstants, FirstVertexIdx) + bufY * pitch + bufX;

  // Material/weight buffers are baked per full-resolution cell. Map this rendered cell to the covering
  // full-resolution cell (clamped so skirt cells reuse the edge cell's material).
  const int cellBufX = ((int)cellX - skirt) * step;
  const int cellBufY = ((int)cellY - skirt) * step;
  const uint fullCellIndex = (uint)(clamp(cellBufY, 0, cellsFull - 1) * cellsFull + clamp(cellBufX, 0, cellsFull - 1));

  // Read per-cell-corner weights (unique slot per corner per cell — no sharing between cells).
  const int wCellX = clamp(bufX, 0, cellsFull - 1);
  const int wCellY = clamp(bufY, 0, cellsFull - 1);
  const uint wSlot = (uint)clamp(bufX - wCellX, 0, 1) + (uint)clamp(bufY - wCellY, 0, 1) * 2u;
  const uint weightCellIndex = (uint)(clamp(wCellY, 0, cellsFull - 1) * cellsFull + clamp(wCellX, 0, cellsFull - 1));

  const uint vtxWeightPacked = TerrainWeights[weightCellIndex * 4u + wSlot];

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

  // LOD fade: pull the vertices that vanish at the next-coarser level toward the position they
  // interpolate to there. A vertex vanishes when its inner-grid index is odd along an axis; it then
  // moves toward the average of its two (or four, at a diagonal) surviving neighbors. Skirt vertices
  // are excluded — they read the outermost border ring and have no outer neighbor to sample.
  // The normal is faded the same way: the coarse triangulation at fade=1 interpolates the normal
  // linearly across the same neighbors, so the fine-grid normal at a vanishing vertex must be blended
  // toward that average too, or lighting still shows the fine-grid bump at fade=1.
  const float fade = GET_PUSH_CONSTANT(HeightfieldRenderConstants, LodFade);
  if (fade > 0.0 && !isSkirt)
  {
    const int gvx = (int)gx - skirt;
    const int gvy = (int)gy - skirt;
    const bool ox = (gvx & 1) != 0;
    const bool oy = (gvy & 1) != 0;
    const int sx = step;
    const int sp = step * pitch;

    float targetH = h;
    float3 targetN = localNormal;
    if (ox && !oy)
    {
      targetH = 0.5 * (TerrainHeights[idx - sx] + TerrainHeights[idx + sx]);
      targetN = 0.5 * (DecodeTerrainNormal(TerrainNormals[idx - sx]) + DecodeTerrainNormal(TerrainNormals[idx + sx]));
    }
    else if (!ox && oy)
    {
      targetH = 0.5 * (TerrainHeights[idx - sp] + TerrainHeights[idx + sp]);
      targetN = 0.5 * (DecodeTerrainNormal(TerrainNormals[idx - sp]) + DecodeTerrainNormal(TerrainNormals[idx + sp]));
    }
    else if (ox && oy)
    {
      // The coarse quad is split TR-BL (see QuadOffsets/CornerSlot above), so the center vertex
      // lies exactly on that diagonal in the coarser LOD — its true height/normal there is the
      // average of the TR and BL corners only, not a bilinear blend of all 4 corners.
      targetH = 0.5 * (TerrainHeights[idx + sx - sp] + TerrainHeights[idx - sx + sp]);
      targetN = 0.5 * (DecodeTerrainNormal(TerrainNormals[idx + sx - sp]) + DecodeTerrainNormal(TerrainNormals[idx - sx + sp]));
    }

    h = lerp(h, targetH, fade);
    localNormal = normalize(lerp(localNormal, targetN, fade));
  }

  // Skirt vertices keep their real height and normal but are pushed down to hide seams between patches.
  // The border ring is always 4 full-resolution buffer units wide (SkirtCells * VertexStep == 4 at any
  // LOD), so the ring distance can be measured directly in buffer units and stays comparable across LODs.
  // Ramping the offset in with smoothstep (instead of jumping to -SkirtDepth on the first skirt vertex)
  // avoids a visible crease at the patch edge.
  const int distX = max(max(0, -bufX), bufX - cellsFull);
  const int distY = max(max(0, -bufY), bufY - cellsFull);
  const float ringT = saturate((float)max(distX, distY) / 4.0);
  const float skirtFalloff = ringT * ringT * (3.0 - 2.0 * ringT);
  const float zOffset = -GET_PUSH_CONSTANT(HeightfieldRenderConstants, SkirtDepth) * skirtFalloff;

  // Apply the object-to-world transform so the patch respects rotation and scale.
  const ezPerInstanceData instanceData = perInstanceData[GET_PUSH_CONSTANT(HeightfieldRenderConstants, InstanceDataOffset)];
  const float4x4 objectToWorld = TransformToMatrix(instanceData.ObjectToWorld);
  const float3x3 objectToWorldNormal = TransformToRotation(instanceData.ObjectToWorldNormal);

  const float gridSpacing = GET_PUSH_CONSTANT(HeightfieldRenderConstants, GridSpacing);
  const float3 localPos = float3((float)bufX * gridSpacing, (float)bufY * gridSpacing, h + zOffset);

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
    TerrainCellMaterials[fullCellIndex],
    0u);
  return Output;
}
