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

EZ_STATICLINK_FILE(TerrainPlugin, TerrainPlugin_Rendering_TerrainRenderer);
