#include <RendererCore/PCH.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Meshes/MeshRenderer.h>
#include <RendererCore/RendererCore.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/ConstantBuffers/ConstantBufferResource.h>
#include <Core/ResourceManager/ResourceManager.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshRenderer, ezRenderer, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

struct PerObjectCB
{
  ezMat4 world;
  ezMat4 mvp;
};

void ezMeshRenderer::GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types)
{
  types.PushBack(ezGetStaticRTTI<ezMeshRenderData>());
}

ezUInt32 ezMeshRenderer::Render(const ezRenderContext& renderContext, ezRenderPipelinePass* pPass, const ezArrayPtr<const ezRenderData*>& renderData)
{
  ezGALContext* pContext = renderContext.m_pGALContext;

  if (!m_hObjectTransformCB.IsValid())
  {
    /// \todo Not sure ezMeshRenderer should create the PerObject CB, probably this should be centralized somewhere else

    // this always accesses the same constant buffer (same GUID), but every ezMeshRenderer holds its own reference to it
    m_hObjectTransformCB = ezResourceManager::GetExistingResource<ezConstantBufferResource>("{34204E49-441A-49D6-AB09-9E8DE38BC803}");

    if (!m_hObjectTransformCB.IsValid())
    {
      // only create this constant buffer, when it does not yet exist
      ezConstantBufferResourceDescriptor<PerObjectCB> desc;
      m_hObjectTransformCB = ezResourceManager::CreateResource<ezConstantBufferResource>("{34204E49-441A-49D6-AB09-9E8DE38BC803}", desc);
    }
  }

  ezRendererCore::BindConstantBuffer(pContext, "PerObject", m_hObjectTransformCB);

  const ezMat4& ViewProj = renderContext.m_pView->GetViewProjectionMatrix();
  ezMaterialResourceHandle hLastMaterial;
  
  ezUInt32 uiDataRendered = 0;
  while (uiDataRendered < renderData.GetCount() && renderData[uiDataRendered]->IsInstanceOf<ezMeshRenderData>())
  { 
    const ezMeshRenderData* pRenderData = static_cast<const ezMeshRenderData*>(renderData[uiDataRendered]);

    PerObjectCB* cb = ezRendererCore::BeginModifyConstantBuffer<PerObjectCB>(m_hObjectTransformCB, pContext);
      cb->world = pRenderData->m_WorldTransform.GetAsMat4();
      cb->mvp = ViewProj * cb->world;
    ezRendererCore::EndModifyConstantBuffer(pContext);

    ezResourceLock<ezMeshResource> pMesh(pRenderData->m_hMesh);
    const ezMeshResourceDescriptor::SubMesh& meshPart = pMesh->GetSubMeshes()[pRenderData->m_uiPartIndex];

    if (pRenderData->m_hMaterial != hLastMaterial)
    {
      ezRendererCore::SetMaterialState(pContext, pRenderData->m_hMaterial);
      hLastMaterial = pRenderData->m_hMaterial;
    }

    ezRendererCore::DrawMeshBuffer(pContext, pMesh->GetMeshBuffer(), meshPart.m_uiPrimitiveCount, meshPart.m_uiFirstPrimitive);
    
    ++uiDataRendered;
  }

  return uiDataRendered;
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshRenderer);

