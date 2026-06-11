#include <TerrainPlugin/TerrainPluginPCH.h>

#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/RenderContext/BindGroupBuilder.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderGraph/RenderGraph.h>
#include <RendererCore/RenderGraph/RenderGraphPassBuilder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/DynamicBuffer.h>
#include <Shaders/Terrain/Rendering/HeightfieldRenderConstants.h>
#include <Shaders/Terrain/Rendering/VoxelMeshRenderConstants.h>
#include <TerrainPlugin/Rendering/TerrainRenderData.h>
#include <TerrainPlugin/Rendering/TerrainRenderer.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTerrainHeightfieldRenderData, 1, ezRTTIDefaultAllocator<ezTerrainHeightfieldRenderData>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTerrainHeightfieldRenderer, 1, ezRTTIDefaultAllocator<ezTerrainHeightfieldRenderer>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezTerrainHeightfieldRenderer::ezTerrainHeightfieldRenderer() = default;
ezTerrainHeightfieldRenderer::~ezTerrainHeightfieldRenderer() = default;

void ezTerrainHeightfieldRenderer::GetSupportedRenderDataTypes(ezDynamicArray<const ezRTTI*>& out_types) const
{
  out_types.PushBack(ezGetStaticRTTI<ezTerrainHeightfieldRenderData>());
}

void ezTerrainHeightfieldRenderer::RenderBatch(const ezRenderViewContext& renderViewContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const
{
  ezRenderContext* pContext = renderViewContext.m_pRenderContext;

  for (auto it = batch.GetIterator<ezTerrainHeightfieldRenderData>(0, batch.GetDataCount()); it.IsValid(); ++it)
  {
    const ezTerrainHeightfieldRenderData* pRenderData = it;

    if (pRenderData->m_hHeightBuffer.IsInvalidated() || pRenderData->m_hCellMaterialBuffer.IsInvalidated() || !pRenderData->m_hMaterial.IsValid())
      continue;

    // Shader and textures come from the material asset assigned to the component.
    pContext->BindMaterial(pRenderData->m_hMaterial);

    // Bind height and normal buffers as SRVs — the shader reads them by vertex index
    ezBindGroupBuilder& bindGroup = pContext->GetBindGroup(EZ_GAL_BIND_GROUP_DRAW_CALL);
    bindGroup.BindBuffer("TerrainHeights", pRenderData->m_hHeightBuffer);
    bindGroup.BindBuffer("TerrainNormals", pRenderData->m_hNormalBuffer);
    bindGroup.BindBuffer("TerrainCellMaterials", pRenderData->m_hCellMaterialBuffer);
    bindGroup.BindBuffer("TerrainWeights", pRenderData->m_hVertexWeightBuffer);

    // Bind instance data buffer so the pixel shader can read GameObjectID for picking.
    ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
    if (auto* pInstanceDataBuffer = pDevice->GetDynamicBuffer(pRenderData->m_hInstanceDataBuffer))
    {
      bindGroup.BindBuffer("perInstanceData", pInstanceDataBuffer->GetBufferForRendering());
    }

    // +1 for vertex count (cells+1 vertices per side), +8 for 4-vertex border on each side.
    const ezUInt32 uiStoredSize = pRenderData->m_uiCellsPerSide + 9;

    HeightfieldRenderConstants constants;
    constants.GridSpacing = pRenderData->m_fGridSpacing;
    constants.FirstVertexIdx = 4 * uiStoredSize + 4; // skip 4 border rows and 4 border cols
    constants.VertexIdxPitch = uiStoredSize;
    constants.CellsPerSide = pRenderData->m_uiCellsPerSide;
    constants.InstanceDataOffset = pRenderData->m_DataOffsets.m_uiInstance;
    constants.FallbackMaterialSlot = pRenderData->m_uiDefaultMaterialIndex;
    pContext->SetPushConstants("HeightfieldRenderConstants", constants);

    // No vertex buffer — SV_VertexID drives everything.
    const ezUInt32 uiPrimitiveCount = pRenderData->m_uiCellsPerSide * pRenderData->m_uiCellsPerSide * 2;
    pContext->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, uiPrimitiveCount);

    pContext->DrawMeshBuffer().IgnoreResult();
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTerrainVoxelRenderData, 1, ezRTTIDefaultAllocator<ezTerrainVoxelRenderData>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTerrainVoxelRenderer, 1, ezRTTIDefaultAllocator<ezTerrainVoxelRenderer>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezTerrainVoxelRenderer::ezTerrainVoxelRenderer() = default;
ezTerrainVoxelRenderer::~ezTerrainVoxelRenderer() = default;

void ezTerrainVoxelRenderer::GetSupportedRenderDataTypes(ezDynamicArray<const ezRTTI*>& out_types) const
{
  out_types.PushBack(ezGetStaticRTTI<ezTerrainVoxelRenderData>());
}

void ezTerrainVoxelRenderer::RenderBatch(const ezRenderViewContext& renderViewContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const
{
  ezRenderContext* pContext = renderViewContext.m_pRenderContext;

  const bool bAsync = pContext->GetAllowAsyncShaderLoading();
  pContext->SetAllowAsyncShaderLoading(false);
  EZ_SCOPE_EXIT(pContext->SetAllowAsyncShaderLoading(bAsync));

  for (auto it = batch.GetIterator<ezTerrainVoxelRenderData>(0, batch.GetDataCount()); it.IsValid(); ++it)
  {
    const ezTerrainVoxelRenderData* pRenderData = it;

    if (pRenderData->m_hGpuMeshVertices.IsInvalidated() || pRenderData->m_hGpuMeshIndices.IsInvalidated() || pRenderData->m_hGpuMeshDrawArgs.IsInvalidated() || !pRenderData->m_hMaterial.IsValid())
      continue;

    ezResourceLock<ezMaterialResource> pMaterial(pRenderData->m_hMaterial, ezResourceAcquireMode::AllowLoadingFallback_NeverFail);
    if (pMaterial.GetAcquireResult() != ezResourceAcquireResult::Final)
      continue;

    // Material provides the shader (VoxelMeshMaterial.ezShader) and any texture bindings.
    pContext->BindMaterial(pRenderData->m_hMaterial);

    // Bind GPU vertex + index buffers as SRVs.
    // The VS reads: index = VoxelIndices[SV_VertexID], then vertex = VoxelVertices[index].
    ezBindGroupBuilder& bindGroup = pContext->GetBindGroup(EZ_GAL_BIND_GROUP_DRAW_CALL);
    bindGroup.BindBuffer("VoxelVertices", pRenderData->m_hGpuMeshVertices);
    bindGroup.BindBuffer("VoxelIndices", pRenderData->m_hGpuMeshIndices);

    // Bind instance data buffer so the shader can read the world transform and GameObjectID.
    ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
    if (auto* pInstanceDataBuffer = pDevice->GetDynamicBuffer(pRenderData->m_hInstanceDataBuffer))
    {
      bindGroup.BindBuffer("perInstanceData", pInstanceDataBuffer->GetBufferForRendering());
    }

    VoxelMeshRenderConstants constants;
    constants.InstanceDataOffset = pRenderData->m_DataOffsets.m_uiInstance;
    constants.BaseMaterialIndex = pRenderData->m_uiBaseMaterialIndex;
    pContext->SetPushConstants("VoxelMeshRenderConstants", constants);

    // Non-indexed draw: SV_VertexID drives index + vertex lookup.
    // Vertex count is read from the GPU-side indirect args buffer (filled in the same submission as the mesh).
    pContext->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, 1);
    if (pContext->ApplyContextStates().Succeeded())
    {
      pContext->GetCommandEncoder()->DrawInstancedIndirect(pRenderData->m_hGpuMeshDrawArgs, 0).IgnoreResult();
    }
  }
}

EZ_STATICLINK_FILE(TerrainPlugin, TerrainPlugin_Rendering_TerrainRenderer);
