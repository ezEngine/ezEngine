#include <RendererCore/PCH.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Meshes/MeshRenderer.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/ConstantBuffers/ConstantBufferResource.h>
#include <Core/ResourceManager/ResourceManager.h>

#include <RendererCore/../../../Shared/Data/Shaders/Common/ObjectConstants.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshRenderer, ezRenderer, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

void ezMeshRenderer::GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types)
{
  types.PushBack(ezGetStaticRTTI<ezMeshRenderData>());
}

ezUInt32 ezMeshRenderer::Render(const ezRenderViewContext& renderViewContext, ezRenderPipelinePass* pPass, const ezArrayPtr<const ezRenderData* const>& renderData)
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

  renderViewContext.m_pRenderContext->BindConstantBuffer("ObjectConstants", m_hObjectTransformCB);

  const ezMat4& ViewMatrix = renderViewContext.m_pViewData->m_ViewMatrix;
  const ezMat4& ViewProjMatrix = renderViewContext.m_pViewData->m_ViewProjectionMatrix;
  ezMaterialResourceHandle hLastMaterial;

  ezUInt32 uiDataRendered = 0;
  while (uiDataRendered < renderData.GetCount() && renderData[uiDataRendered]->IsInstanceOf<ezMeshRenderData>())
  {
    const ezMeshRenderData* pRenderData = static_cast<const ezMeshRenderData*>(renderData[uiDataRendered]);

    ObjectConstants* cb = renderViewContext.m_pRenderContext->BeginModifyConstantBuffer<ObjectConstants>(m_hObjectTransformCB);
    cb->ObjectToWorldMatrix = pRenderData->m_GlobalTransform.GetAsMat4();
    cb->ObjectToCameraMatrix = ViewMatrix * cb->ObjectToWorldMatrix;
    cb->ObjectToScreenMatrix = ViewProjMatrix * cb->ObjectToWorldMatrix;
    cb->GameObjectID = pRenderData->m_uiEditorPickingID;
    cb->PartIndex = pRenderData->m_uiPartIndex;

    renderViewContext.m_pRenderContext->EndModifyConstantBuffer();

    ezResourceLock<ezMeshResource> pMesh(pRenderData->m_hMesh);
    const ezMeshResourceDescriptor::SubMesh& meshPart = pMesh->GetSubMeshes()[pRenderData->m_uiPartIndex];

    if (pRenderData->m_hMaterial != hLastMaterial)
    {
      renderViewContext.m_pRenderContext->SetMaterialState(pRenderData->m_hMaterial);
      hLastMaterial = pRenderData->m_hMaterial;
    }

    renderViewContext.m_pRenderContext->SetMaterialParameter("MeshColor", pRenderData->m_MeshColor);

    renderViewContext.m_pRenderContext->BindMeshBuffer(pMesh->GetMeshBuffer());

    renderViewContext.m_pRenderContext->DrawMeshBuffer(meshPart.m_uiPrimitiveCount, meshPart.m_uiFirstPrimitive);

    ++uiDataRendered;
  }

  return uiDataRendered;
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshRenderer);

