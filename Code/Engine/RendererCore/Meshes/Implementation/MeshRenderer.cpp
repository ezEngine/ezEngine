#include <RendererCore/PCH.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Meshes/MeshRenderer.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/Pipeline/ViewData.h>
#include <RendererCore/ConstantBuffers/ConstantBufferResource.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/ObjectConstants.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshRenderer, 1, ezRTTIDefaultAllocator<ezMeshRenderer>);
EZ_END_DYNAMIC_REFLECTED_TYPE

void ezMeshRenderer::GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types)
{
  types.PushBack(ezGetStaticRTTI<ezMeshRenderData>());
}

void ezMeshRenderer::RenderBatch(const ezRenderViewContext& renderViewContext, ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch)
{
  if (!m_hObjectTransformCB.IsValid())
  {
    /// \todo Not sure ezMeshRenderer should create the ObjectConstants CB, probably this should be centralized somewhere else

    // this always accesses the same constant buffer (same GUID), but every ezMeshRenderer holds its own reference to it
    m_hObjectTransformCB = ezResourceManager::GetExistingResource<ezConstantBufferResource>("{34204E49-441A-49D6-AB09-9E8DE38BC803}");

    if (!m_hObjectTransformCB.IsValid())
    {
      // only create this constant buffer, when it does not yet exist
      ezConstantBufferResourceDescriptor<ObjectConstants> desc;
      m_hObjectTransformCB = ezResourceManager::CreateResource<ezConstantBufferResource>("{34204E49-441A-49D6-AB09-9E8DE38BC803}", desc, "ezMeshRenderer CB");
    }
  }

  const ezMeshRenderData* pRenderData = batch.GetData<ezMeshRenderData>(0);

  const ezMeshResourceHandle& hMesh = pRenderData->m_hMesh;
  const ezMaterialResourceHandle& hMaterial = pRenderData->m_hMaterial;
  ezUInt32 uiPartIndex = pRenderData->m_uiPartIndex;

  ezResourceLock<ezMeshResource> pMesh(hMesh);

  // This can happen when the resource has been reloaded and now has fewer submeshes.
  const auto& subMeshes = pMesh->GetSubMeshes();
  if (subMeshes.GetCount() <= uiPartIndex)
  {
    return;
  }

  const ezMeshResourceDescriptor::SubMesh& meshPart = subMeshes[uiPartIndex];

  renderViewContext.m_pRenderContext->BindMeshBuffer(pMesh->GetMeshBuffer());
  renderViewContext.m_pRenderContext->SetMaterialState(hMaterial);  

  renderViewContext.m_pRenderContext->BindConstantBuffer("ObjectConstants", m_hObjectTransformCB);

  const ezMat4& ViewMatrix = renderViewContext.m_pViewData->m_ViewMatrix;
  const ezMat4& ViewProjMatrix = renderViewContext.m_pViewData->m_ViewProjectionMatrix;

  // TODO: use instancing
  for (auto it = batch.GetIterator<ezMeshRenderData>(); it.IsValid(); ++it)
  {
    pRenderData = it;

    EZ_ASSERT_DEV(pRenderData->m_hMesh == hMesh, "Invalid batching (mesh)");
    EZ_ASSERT_DEV(pRenderData->m_hMaterial == hMaterial, "Invalid batching (material)");
    EZ_ASSERT_DEV(pRenderData->m_uiPartIndex == uiPartIndex, "Invalid batching (part)");

    ObjectConstants* cb = renderViewContext.m_pRenderContext->BeginModifyConstantBuffer<ObjectConstants>(m_hObjectTransformCB);
    cb->ObjectToWorldMatrix = pRenderData->m_GlobalTransform.GetAsMat4();
    cb->ObjectToCameraMatrix = ViewMatrix * cb->ObjectToWorldMatrix;
    cb->ObjectToScreenMatrix = ViewProjMatrix * cb->ObjectToWorldMatrix;
    cb->GameObjectID = pRenderData->m_uiEditorPickingID;
    cb->PartIndex = pRenderData->m_uiPartIndex;

    renderViewContext.m_pRenderContext->EndModifyConstantBuffer();
    
    renderViewContext.m_pRenderContext->SetMaterialParameter("MeshColor", pRenderData->m_MeshColor);    

    renderViewContext.m_pRenderContext->DrawMeshBuffer(meshPart.m_uiPrimitiveCount, meshPart.m_uiFirstPrimitive);
  }
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshRenderer);

