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

/// Must match TERRAIN_MATERIAL_CELL_STEP in Generation/HeightfieldBakeConstants.h, which documents it.
/// Redefined rather than included: that header also declares the bake constant buffer, which collides.
#define TERRAIN_MATERIAL_CELL_STEP 4

/// Baked heights produced by the compute shader, one float per grid vertex.
StructuredBuffer<float> TerrainHeights BIND_GROUP(BG_DRAW_CALL);

/// Per-vertex packed normals from TerrainNormalsCS: XY as 16-bit floats in a uint32.
StructuredBuffer<uint> TerrainNormals BIND_GROUP(BG_DRAW_CALL);

/// Per-cell top-4 explicit material indices baked by Step3 (uint per material cell).
/// Packing: mat0|mat1<<8|mat2<<16|mat3<<24, ordered by descending brush weight (slot 0 = strongest).
StructuredBuffer<uint> TerrainCellMaterials BIND_GROUP(BG_DRAW_CALL);

/// Per-cell-corner blend weights baked by Step3 (uint per cell-corner,
/// ((CellsPerSide+8)/TERRAIN_MATERIAL_CELL_STEP)²×4 entries, covering the rendered cells and the
/// 4-cell border ring the skirt is drawn over).
/// Layout: cellIndex*4 + cornerSlot (TL=0,TR=1,BL=2,BR=3).
/// Packing: w0|w1<<8|w2<<16|w3<<24 (8-bit unorm). Fallback weight = max(0, 1-w0-w1-w2-w3).
/// Sentinel 0xFFFFFFFF marks a corner whose vertex was carved away.
StructuredBuffer<uint> TerrainWeights BIND_GROUP(BG_DRAW_CALL);

/// One bit per stored-grid vertex, set when the vertex was carved away (bit i&31 of word i>>5).
/// Full resolution, unlike the material weights: carving is a per-vertex decision.
StructuredBuffer<uint> TerrainCarveMask BIND_GROUP(BG_DRAW_CALL);

/// Decode a packed terrain normal (XY as f16, Z reconstructed).
float3 DecodeTerrainNormal(uint packed)
{
  float nx = f16tof32(packed & 0xFFFFu);
  float ny = f16tof32(packed >> 16u);
  float nz = sqrt(max(0.0, 1.0 - nx * nx - ny * ny));
  return float3(nx, ny, nz);
}

/// Unpack a corner's 4 blend weights (8-bit unorm each) into [0, 1] floats.
/// The carve sentinel unpacks to zero weight, leaving the implicit fallback material.
float4 UnpackWeights(uint packed)
{
  if (packed == 0xFFFFFFFFu)
    return float4(0.0, 0.0, 0.0, 0.0);

  return float4(packed & 0xFFu, (packed >> 8) & 0xFFu, (packed >> 16) & 0xFFu, (packed >> 24) & 0xFFu) * (1.0 / 255.0);
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

  // Material data is baked over the whole stored grid: the rendered cells plus the 4-cell border
  // ring, hence the +4 that turns a buffer coordinate into a stored-grid one.
  const int matCellsPerSide = (cellsFull + 8) / TERRAIN_MATERIAL_CELL_STEP;

  const int storedVtxX = bufX + 4;
  const int storedVtxY = bufY + 4;

  // The material cell comes from the cell being drawn, not from the vertex: it selects the index set
  // in TerrainCellMaterials, which is nointerpolation and therefore taken from the provoking vertex
  // alone. Deriving it per vertex lets a quad's corners pick different cells, and the non-provoking
  // vertices' weights are then read against the wrong set.
  const int cellStoredX = ((int)cellX - skirt) * step + 4;
  const int cellStoredY = ((int)cellY - skirt) * step + 4;
  const int matCellX = clamp(cellStoredX / TERRAIN_MATERIAL_CELL_STEP, 0, matCellsPerSide - 1);
  const int matCellY = clamp(cellStoredY / TERRAIN_MATERIAL_CELL_STEP, 0, matCellsPerSide - 1);
  const uint fullCellIndex = (uint)(matCellY * matCellsPerSide + matCellX);

  // Position within that material cell. The material step is at least the coarsest LOD's vertex step
  // and both grids share an origin, so a rendered cell never straddles a cell boundary and these stay
  // in [0, 1] without clamping at any LOD.
  const float fx = (float)(storedVtxX - matCellX * TERRAIN_MATERIAL_CELL_STEP) / (float)TERRAIN_MATERIAL_CELL_STEP;
  const float fy = (float)(storedVtxY - matCellY * TERRAIN_MATERIAL_CELL_STEP) / (float)TERRAIN_MATERIAL_CELL_STEP;

  // All four corners come from this one cell: Step3 remaps each corner against its own cell's
  // material set, so entries from neighbouring cells cannot be mixed.
  const uint cornerBase = fullCellIndex * 4u;
  const uint packedTL = TerrainWeights[cornerBase + 0u];
  const uint packedTR = TerrainWeights[cornerBase + 1u];
  const uint packedBL = TerrainWeights[cornerBase + 2u];
  const uint packedBR = TerrainWeights[cornerBase + 3u];

  // Carve test at full resolution, so carving stays as sharp as the geometry: testing the
  // interpolated corners instead would carve a whole material cell when one corner was carved.
  // NaN position causes the spec to cull any triangle touching this vertex.
  const uint storedVtxIdx = (uint)(storedVtxY * pitch + storedVtxX);
  if ((TerrainCarveMask[storedVtxIdx >> 5u] & (1u << (storedVtxIdx & 31u))) != 0u)
  {
    VS_OUT Discarded = (VS_OUT)0;
    const float fNaN = asfloat(0x7FC00000u);
    Discarded.Position = float4(fNaN, fNaN, fNaN, fNaN);
    return Discarded;
  }

  float h = TerrainHeights[idx];

#if defined(USE_NORMAL)
  float3 localNormal = DecodeTerrainNormal(TerrainNormals[idx]);
#endif

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
#if defined(USE_NORMAL)
    float3 targetN = localNormal;
#endif
    if (ox && !oy)
    {
      targetH = 0.5 * (TerrainHeights[idx - sx] + TerrainHeights[idx + sx]);
#if defined(USE_NORMAL)
      targetN = 0.5 * (DecodeTerrainNormal(TerrainNormals[idx - sx]) + DecodeTerrainNormal(TerrainNormals[idx + sx]));
#endif
    }
    else if (!ox && oy)
    {
      targetH = 0.5 * (TerrainHeights[idx - sp] + TerrainHeights[idx + sp]);
#if defined(USE_NORMAL)
      targetN = 0.5 * (DecodeTerrainNormal(TerrainNormals[idx - sp]) + DecodeTerrainNormal(TerrainNormals[idx + sp]));
#endif
    }
    else if (ox && oy)
    {
      // The coarse quad is split TR-BL (see QuadOffsets/CornerSlot above), so the center vertex
      // lies exactly on that diagonal in the coarser LOD — its true height/normal there is the
      // average of the TR and BL corners only, not a bilinear blend of all 4 corners.
      targetH = 0.5 * (TerrainHeights[idx + sx - sp] + TerrainHeights[idx - sx + sp]);
#if defined(USE_NORMAL)
      targetN = 0.5 * (DecodeTerrainNormal(TerrainNormals[idx + sx - sp]) + DecodeTerrainNormal(TerrainNormals[idx - sx + sp]));
#endif
    }

    h = lerp(h, targetH, fade);
#if defined(USE_NORMAL)
    localNormal = normalize(lerp(localNormal, targetN, fade));
#endif
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

  const float3 worldPos = mul(objectToWorld, float4(localPos, 1.0f)).xyz;

#if defined(USE_NORMAL)
  const float3 worldNormal = normalize(mul(objectToWorldNormal, localNormal));
#endif

#if defined(USE_TANGENT)
  // Analytical tangent for a heightfield: lies in the XZ plane, perpendicular to the normal.
  // Derived from dP/du = (GridSpacing, 0, dH/dx), orthogonalized: float3(n.z, 0, -n.x).
  const float3 localTangent = normalize(float3(localNormal.z, 0.0, -localNormal.x));
  const float3 localBitangent = cross(localNormal, localTangent);

  const float3 worldTangent = normalize(mul(objectToWorldNormal, localTangent));
  const float3 worldBitangent = normalize(mul(objectToWorldNormal, localBitangent));
#endif

  VS_OUT Output;
  Output.Position = mul(GetWorldToScreenMatrix(), float4(worldPos, 1.0f));
  Output.WorldPosition = worldPos;

#if defined(USE_NORMAL)
  Output.Normal = worldNormal;
#endif

#if defined(USE_TANGENT)
  Output.Tangent = worldTangent;
  Output.BiTangent = worldBitangent;
#endif

#if defined(USE_TEXCOORD0)
  // A carved corner holds the sentinel, not weights. Its own vertex is culled, but it still
  // contributes to surviving vertices in the same cell, so it unpacks to zero weight.
  const float4 wTL = UnpackWeights(packedTL);
  const float4 wTR = UnpackWeights(packedTR);
  const float4 wBL = UnpackWeights(packedBL);
  const float4 wBR = UnpackWeights(packedBR);
  const float4 w = lerp(lerp(wTL, wTR, fx), lerp(wBL, wBR, fx), fy);

  // TexCoord0: w0, w1. MatWeightsHi: w2, w3 (via CUSTOM_INTERPOLATOR).
  // The PS reconstructs w_fallback = max(0, 1 - w0 - w1 - w2 - w3).
  Output.TexCoord0 = float2(w.x, w.y);
  Output.MatWeightsHi = float2(w.z, w.w);
#endif

  // DataOffsets.x: instance data offset.
  // DataOffsets.y: 4 cell-level material indices — identical for all 6 vertices of this cell.
  Output.DataOffsets = uint3(
    GET_PUSH_CONSTANT(HeightfieldRenderConstants, InstanceDataOffset),
    TerrainCellMaterials[fullCellIndex],
    0u);
  return Output;
}
