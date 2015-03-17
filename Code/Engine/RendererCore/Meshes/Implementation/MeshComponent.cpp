#include <RendererCore/PCH.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <Core/ResourceManager/ResourceManager.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshRenderData, ezRenderData, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_COMPONENT_TYPE(ezMeshComponent, ezComponent, 1, ezMeshComponentManager);
  EZ_BEGIN_MESSAGEHANDLERS
    EZ_MESSAGE_HANDLER(ezExtractRenderDataMessage, OnExtractRenderData)
  EZ_END_MESSAGEHANDLERS
EZ_END_COMPONENT_TYPE();

ezMeshComponent::ezMeshComponent()
{
}

void ezMeshComponent::SetMesh(const ezMeshResourceHandle& hMesh)
{
  m_hMesh = hMesh;

  //ezResourceLock<ezMeshResource> pMesh(m_hMesh);
  //m_Materials.SetCount(pMesh->GetMaterials().GetCount());
}

ezResult ezMeshComponent::OnAttachedToObject()
{
  return EZ_SUCCESS;
}

ezResult ezMeshComponent::OnDetachedFromObject()
{
  return EZ_SUCCESS;
}

void ezMeshComponent::OnExtractRenderData(ezExtractRenderDataMessage& msg) const
{
  ezRenderPipeline* pRenderPipeline = msg.m_pRenderPipeline;

  ezResourceLock<ezMeshResource> pMesh(m_hMesh);
  const ezDynamicArray<ezMeshResourceDescriptor::SubMesh>& parts = pMesh->GetSubMeshes();

  for (ezUInt32 uiPartIndex = 0; uiPartIndex < parts.GetCount(); ++uiPartIndex)
  {
    ezMeshRenderData* pRenderData = pRenderPipeline->CreateRenderData<ezMeshRenderData>(ezDefaultPassTypes::Opaque, GetOwner());
    pRenderData->m_WorldTransform = GetOwner()->GetWorldTransform();
    pRenderData->m_hMesh = m_hMesh;

    const ezUInt32 uiMaterialIndex = parts[uiPartIndex].m_uiMaterialIndex;

    // if we have a material override, use that
    // otherwise use the default mesh material
    if (GetMaterial(parts[uiPartIndex].m_uiMaterialIndex).IsValid())
      pRenderData->m_hMaterial = m_Materials[uiMaterialIndex];
    else
      pRenderData->m_hMaterial = pMesh->GetMaterials()[uiMaterialIndex];

    pRenderData->m_uiPartIndex = uiPartIndex;
  }
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshComponent);

