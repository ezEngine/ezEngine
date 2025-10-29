#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <RmlUiPlugin/Implementation/RmlUiRenderData.h>
#include <RmlUiPlugin/Implementation/RmlUiMeshRenderer.h>

#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/Pipeline/ViewData.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/Resources/Texture.h>
#include <RendererCore/Pipeline/InstanceDataProvider.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/Implementation/MeshRendererUtils.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRmlUiMeshRenderData, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRmlUiMeshRenderer, 1, ezRTTIDefaultAllocator<ezRmlUiMeshRenderer>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

bool ezRmlUiMeshRenderData::CanBatch(const ezRenderData& other0) const
{
  const auto& other = ezStaticCast<const ezRmlUiMeshRenderData&>(other0);

  return m_hMesh == other.m_hMesh && m_uiSubMeshIndex == other.m_uiSubMeshIndex &&
    m_hTexture == other.m_hTexture && m_uiFlipWinding == other.m_uiFlipWinding;
}

void ezRmlUiMeshRenderData::FillSortingKey()
{
  m_uiFlipWinding = m_GlobalTransform.HasMirrorScaling() ? 1 : 0;
  m_uiUniformScale = m_GlobalTransform.ContainsUniformScale() ? 1 : 0;

  const ezUInt32 uiMeshIDHash = ezHashingUtils::StringHashTo32(m_hMesh.GetResourceIDHash());
  const ezUInt32 uiMaterialIDHash = m_hTexture.IsInvalidated() ? 0 : m_hTexture.GetInternalID().m_Data;

  // Sort by material and then by mesh
  m_uiSortingKey = (uiMaterialIDHash << 16) | ((uiMeshIDHash + m_uiSubMeshIndex) & 0xFFFE) | m_uiFlipWinding;
}

ezRmlUiMeshRenderer::ezRmlUiMeshRenderer()
{
  m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/RmlUiMesh.ezShader");
}

ezRmlUiMeshRenderer::~ezRmlUiMeshRenderer() = default;

void ezRmlUiMeshRenderer::GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& ref_types) const
{
  ref_types.PushBack(ezGetStaticRTTI<ezRmlUiMeshRenderData>());
}

void ezRmlUiMeshRenderer::GetSupportedRenderDataCategories(ezHybridArray<ezRenderData::Category, 8>& ref_categories) const
{
  ref_categories.PushBack(ezDefaultRenderDataCategories::SimpleOpaque);
}

void ezRmlUiMeshRenderer::RenderBatch(const ezRenderViewContext& renderViewContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const
{
  ezRenderContext* pContext = renderViewContext.m_pRenderContext;

  const ezRmlUiMeshRenderData* pRenderData = batch.GetFirstData<ezRmlUiMeshRenderData>();

  const ezUInt32 uiPartIndex = pRenderData->m_uiSubMeshIndex;

  const ezMeshResourceHandle& hMesh = pRenderData->m_hMesh;
  ezResourceLock<ezMeshResource> pMesh(hMesh, ezResourceAcquireMode::AllowLoadingFallback);

  // This can happen when the resource has been reloaded and now has fewer submeshes.
  const auto& subMeshes = pMesh->GetSubMeshes();
  if (subMeshes.GetCount() <= uiPartIndex)
  {
    return;
  }

  const ezMeshResourceDescriptor::SubMesh& meshPart = subMeshes[uiPartIndex];
  ezUInt32 uiPrimitiveCount = meshPart.m_uiPrimitiveCount;
  ezUInt32 uiFirstPrimitive = meshPart.m_uiFirstPrimitive;

  pContext->BindMeshBuffer(pMesh->GetMeshBuffer());
  
  pContext->BindShader(m_hShader);

  if (pRenderData->m_uiFlipWinding)
  {
    pContext->SetShaderPermutationVariable("FLIP_WINDING", "TRUE");
  }
  else
  {
    pContext->SetShaderPermutationVariable("FLIP_WINDING", "FALSE");
  }

  pContext->SetShaderPermutationVariable("SHADING_QUALITY", "SHADING_QUALITY_SIMPLIFIED");

  ezBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup();
  bindGroup.BindTexture("BaseTexture", pRenderData->m_hTexture);

  SetAdditionalData(renderViewContext, pRenderData);

  ezInstanceData* pInstanceData = pPass->GetPipeline()->GetFrameDataProvider<ezInstanceDataProvider>()->GetData(renderViewContext);
  pInstanceData->BindResources(pContext);

  ezUInt32 uiStartIndex = 0;
  while (uiStartIndex < batch.GetCount())
  {
    const ezUInt32 uiRemainingInstances = batch.GetCount() - uiStartIndex;

    ezUInt32 uiInstanceDataOffset = 0;
    ezArrayPtr<ezPerInstanceData> instanceData = pInstanceData->GetInstanceData(pContext, uiRemainingInstances, uiInstanceDataOffset);

    ezUInt32 uiFilteredCount = 0;
    FillPerInstanceData(instanceData, batch, uiStartIndex, uiFilteredCount);

    if (uiFilteredCount > 0) // Instance data might be empty if all render data was filtered.
    {
      pInstanceData->UpdateInstanceData(pContext, uiFilteredCount);

      if (pContext->DrawMeshBuffer(uiPrimitiveCount, uiFirstPrimitive, uiFilteredCount).Failed())
      {
        for (auto it = batch.GetIterator<ezRmlUiMeshRenderData>(uiStartIndex, instanceData.GetCount()); it.IsValid(); ++it)
        {
          pRenderData = it;

          // draw bounding box instead
          if (pRenderData->m_GlobalBounds.IsValid())
          {
            ezDebugRenderer::DrawLineBox(*renderViewContext.m_pViewDebugContext, pRenderData->m_GlobalBounds.GetBox(), ezColor::Magenta);
          }
        }
      }
    }

    uiStartIndex += instanceData.GetCount();
  }
}

void ezRmlUiMeshRenderer::SetAdditionalData(const ezRenderViewContext& renderViewContext, const ezRmlUiMeshRenderData* pRenderData) const
{
  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("VERTEX_SKINNING", "FALSE");
}

void ezRmlUiMeshRenderer::FillPerInstanceData(ezArrayPtr<ezPerInstanceData> instanceData, const ezRenderDataBatch& batch, ezUInt32 uiStartIndex, ezUInt32& out_uiFilteredCount) const
{
  ezUInt32 uiCount = ezMath::Min<ezUInt32>(instanceData.GetCount(), batch.GetCount() - uiStartIndex);
  ezUInt32 uiCurrentIndex = 0;

  for (auto it = batch.GetIterator<ezRmlUiMeshRenderData>(uiStartIndex, uiCount); it.IsValid(); ++it)
  {
    const ezRmlUiMeshRenderData* pRenderData = it;
    ezPerInstanceData& ref_perInstanceData = instanceData[uiCurrentIndex];

    ezMat4 objectToWorld = pRenderData->m_GlobalTransform.GetAsMat4();

    ref_perInstanceData.ObjectToWorld = objectToWorld;
    ref_perInstanceData.BoundingSphereRadius = pRenderData->m_GlobalBounds.m_fSphereRadius;
    ref_perInstanceData.GameObjectID = pRenderData->m_uiUniqueID;
    ref_perInstanceData.VertexColorAccessData = 0;

    ++uiCurrentIndex;
  }

  out_uiFilteredCount = uiCurrentIndex;
}

EZ_STATICLINK_FILE(RmlUiPlugin, RmlUiPlugin_Implementation_RmlUiMeshRenderer);
