#pragma once

#include <RendererCore/Pipeline/Renderer.h>
#include <TerrainPlugin/TerrainPluginDLL.h>

class ezRenderDataBatch;

/// Renders terrain patches submitted as ezTerrainHeightfieldRenderData.
///
/// Each patch is drawn as a procedural grid using SV_VertexID. No vertex buffer is needed;
/// the vertex shader reads height data from a structured buffer (SRV) bound per patch.
/// The shader and textures come from the material assigned to the patch component.
class EZ_TERRAINPLUGIN_DLL ezTerrainHeightfieldRenderer : public ezRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTerrainHeightfieldRenderer, ezRenderer);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezTerrainHeightfieldRenderer);

public:
  ezTerrainHeightfieldRenderer();
  ~ezTerrainHeightfieldRenderer();

  virtual void GetSupportedRenderDataTypes(ezDynamicArray<const ezRTTI*>& out_types) const override;
  virtual void RenderBatch(const ezRenderViewContext& renderViewContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const override;
};

/// Renders voxel mesh volumes directly from the GPU buffers produced by surface nets shaders.
///
/// No CPU vertex/index upload is required. The vertex shader reads VoxelGpuVertex structs from a
/// StructuredBuffer SRV and uses the index SRV to emulate indexed drawing, both indexed by SV_VertexID.
class EZ_TERRAINPLUGIN_DLL ezTerrainVoxelRenderer : public ezRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTerrainVoxelRenderer, ezRenderer);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezTerrainVoxelRenderer);

public:
  ezTerrainVoxelRenderer();
  ~ezTerrainVoxelRenderer();

  void GetSupportedRenderDataTypes(ezDynamicArray<const ezRTTI*>& out_types) const override;
  void RenderBatch(const ezRenderViewContext& renderViewContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const override;
};
