#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/Gizmos/GizmoComponent.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoRenderer.h>
#include <EditorEngineProcessFramework/PickingRenderPass/PickingRenderPass.h>

#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererCore/../../../Data/Base/Shaders/Editor/GizmoConstants.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGizmoRenderer, 1, ezRTTIDefaultAllocator<ezGizmoRenderer>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

float ezGizmoRenderer::s_fGizmoScale = 1.0f;

ezGizmoRenderer::ezGizmoRenderer() = default;
ezGizmoRenderer::~ezGizmoRenderer() = default;

void ezGizmoRenderer::GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& inout_types) const
{
  inout_types.PushBack(ezGetStaticRTTI<ezGizmoRenderData>());
}

void ezGizmoRenderer::GetSupportedRenderDataCategories(ezHybridArray<ezRenderData::Category, 8>& inout_categories) const
{
  inout_categories.PushBack(ezDefaultRenderDataCategories::SimpleOpaque);
  inout_categories.PushBack(ezDefaultRenderDataCategories::SimpleForeground);
}

void ezGizmoRenderer::RenderBatch(const ezRenderViewContext& renderViewContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const
{
  bool bOnlyPickable = false;

  if (auto pPickingRenderPass = ezDynamicCast<const ezPickingRenderPass*>(pPass))
  {
    // gizmos only exist for 'selected' objects, so ignore all gizmo rendering, if we don't want to pick selected objects
    if (!pPickingRenderPass->m_bPickSelected)
      return;

    bOnlyPickable = true;
  }

  const ezGizmoRenderData* pRenderData = batch.GetFirstData<ezGizmoRenderData>();

  const ezMeshResourceHandle& hMesh = pRenderData->m_hMesh;
  const ezMaterialResourceHandle& hMaterial = pRenderData->m_hMaterial;
  ezUInt32 uiSubMeshIndex = pRenderData->m_uiSubMeshIndex;

  ezResourceLock<ezMeshResource> pMesh(hMesh, ezResourceAcquireMode::AllowLoadingFallback);

  // This can happen when the resource has been reloaded and now has fewer submeshes.
  const auto& subMeshes = pMesh->GetSubMeshes();
  if (subMeshes.GetCount() <= uiSubMeshIndex)
  {
    return;
  }

  const ezMeshResourceDescriptor::SubMesh& meshPart = subMeshes[uiSubMeshIndex];

  renderViewContext.m_pRenderContext->BindMeshBuffer(pMesh->GetMeshBuffer());
  renderViewContext.m_pRenderContext->BindMaterial(hMaterial);

  ezConstantBufferStorage<ezGizmoConstants>* pGizmoConstantBuffer;
  ezConstantBufferStorageHandle hGizmoConstantBuffer = ezRenderContext::CreateConstantBufferStorage(pGizmoConstantBuffer);
  EZ_SCOPE_EXIT(ezRenderContext::DeleteConstantBufferStorage(hGizmoConstantBuffer));

  renderViewContext.m_pRenderContext->BindConstantBuffer("ezGizmoConstants", hGizmoConstantBuffer);

  // since typically the fov is tied to the height, we orient the gizmo size on that
  const float fGizmoScale = s_fGizmoScale * (128.0f / (float)renderViewContext.m_pViewData->m_ViewPortRect.height);

  for (auto it = batch.GetIterator<ezGizmoRenderData>(); it.IsValid(); ++it)
  {
    pRenderData = it;

    if (bOnlyPickable && !pRenderData->m_bIsPickable)
      continue;

    EZ_ASSERT_DEV(pRenderData->m_hMesh == hMesh, "Invalid batching (mesh)");
    EZ_ASSERT_DEV(pRenderData->m_hMaterial == hMaterial, "Invalid batching (material)");
    EZ_ASSERT_DEV(pRenderData->m_uiSubMeshIndex == uiSubMeshIndex, "Invalid batching (part)");

    ezGizmoConstants& cb = pGizmoConstantBuffer->GetDataForWriting();
    ezMat4 m = pRenderData->m_GlobalTransform.GetAsMat4();
    cb.ObjectToWorldMatrix = m;
    m.Invert(0.001f).IgnoreResult(); // this can fail, if scale is 0 (which happens), doesn't matter in those cases
    cb.WorldToObjectMatrix = m;
    cb.GizmoColor = pRenderData->m_GizmoColor;
    cb.GizmoScale = fGizmoScale;
    cb.GameObjectID = pRenderData->m_uiUniqueID;

    if (renderViewContext.m_pRenderContext->DrawMeshBuffer(meshPart.m_uiPrimitiveCount, meshPart.m_uiFirstPrimitive).Failed())
    {
      // draw bounding box instead
      if (pRenderData->m_GlobalBounds.IsValid())
      {
        ezDebugRenderer::DrawLineBox(*renderViewContext.m_pViewDebugContext, pRenderData->m_GlobalBounds.GetBox(), ezColor::Magenta);
      }
    }
  }
}
