#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererFoundation/RendererFoundationDLL.h>
#include <TerrainPlugin/TerrainPluginDLL.h>

using ezMaterialResourceHandle = ezTypedResourceHandle<class ezMaterialResource>;

/// Render data submitted by ezTerrainPatchComponent.
///
/// Carries the handles and parameters needed by ezTerrainHeightfieldRenderer to issue the draw call.
/// The vertex shader generates terrain geometry from SV_VertexID using the height buffer as input.
class EZ_TERRAINPLUGIN_DLL ezTerrainHeightfieldRenderData : public ezInstanceableRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTerrainHeightfieldRenderData, ezInstanceableRenderData);

public:
  /// Material asset that provides the terrain shader and texture/parameter bindings.
  ezMaterialResourceHandle m_hMaterial;

  /// Persistent GPU buffer containing baked height values (float per vertex).
  /// Bound as SRV (StructuredBuffer<float>) in the vertex shader.
  ezGALBufferHandle m_hHeightBuffer;

  /// Persistent GPU buffer containing packed XY normal components (uint per vertex).
  /// Bound as SRV (StructuredBuffer<uint>) in the vertex shader. Z is reconstructed from XY.
  ezGALBufferHandle m_hNormalBuffer;

  /// Per-cell top-4 material indices baked by Step3 (uint per cell).
  /// Packing: mat0 | (mat1 << 8) | (mat2 << 16) | (mat3 << 24), ordered by descending brush weight (slot 0 = strongest).
  ezGALBufferHandle m_hCellMaterialBuffer;

  /// Per-cell-corner 4-weight pack baked by Step3 (uint per cell-corner, CellsPerSide²×4 entries).
  /// Layout: cellIndex * 4 + cornerSlot, cornerSlot = TL:0, TR:1, BL:2, BR:3.
  /// Packing: w0 | (w1 << 8) | (w2 << 16) | (w3 << 24) (8-bit unorm). w_fallback reconstructed in VS.
  /// Sentinel 0xFFFFFFFF marks carved corners; the VS outputs NaN for those vertices.
  ezGALBufferHandle m_hVertexWeightBuffer;

  /// Number of rendered quads per side (= ezTerrainResolution enum value, e.g. 128).
  ezUInt32 m_uiCellsPerSide = 128;

  /// World-space distance between adjacent vertices.
  float m_fGridSpacing = 1.0f;

  /// Material slot index (0–7) used as the implicit fallback layer when no explicit brush covers a vertex.
  ezUInt32 m_uiDefaultMaterialIndex = 0;
};

/// Render data for GPU-direct voxel mesh rendering.
///
/// Carries the GPU-side vertex and index buffer handles (StructuredBuffer SRVs written by surface nets shaders)
/// and a primitive count from the VoxelMeshCounts readback. No CPU vertex/index data is needed.
class EZ_TERRAINPLUGIN_DLL ezTerrainVoxelRenderData : public ezInstanceableRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTerrainVoxelRenderData, ezInstanceableRenderData);

public:
  ezMaterialResourceHandle m_hMaterial;

  /// GPU vertex buffer (StructuredBuffer<VoxelGpuVertex>) written by surface nets shaders.
  ezGALBufferHandle m_hGpuMeshVertices;

  /// GPU index buffer (StructuredBuffer<uint>) written by surface nets shaders.
  ezGALBufferHandle m_hGpuMeshIndices;

  /// Indirect draw arguments (4 uints, ByteAddressBuffer, DrawIndirect-flagged). Written GPU-side
  /// in the same submission as the mesh, so the consumed primitive count always matches the buffer
  /// contents — no readback latency window. The primitive count never appears CPU-side in the
  /// rendering path.
  ezGALBufferHandle m_hGpuMeshDrawArgs;

  ezUInt32 m_uiBaseMaterialIndex = 0;
};
