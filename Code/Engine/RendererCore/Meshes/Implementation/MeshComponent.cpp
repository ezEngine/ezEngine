#include <RendererCore/PCH.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Pipeline/View.h>
#include <Core/ResourceManager/ResourceManager.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshRenderData, ezRenderData, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_COMPONENT_TYPE(ezMeshComponent, ezComponent, 1, ezMeshComponentManager);
  EZ_BEGIN_PROPERTIES
    EZ_ACCESSOR_PROPERTY("MeshFile", GetMeshFile, SetMeshFile),
    EZ_MEMBER_PROPERTY("Mesh Color", m_MeshColor),
  EZ_END_PROPERTIES
  EZ_BEGIN_MESSAGEHANDLERS
    EZ_MESSAGE_HANDLER(ezUpdateLocalBoundsMessage, OnUpdateLocalBounds),
    EZ_MESSAGE_HANDLER(ezExtractRenderDataMessage, OnExtractRenderData),
  EZ_END_MESSAGEHANDLERS
EZ_END_COMPONENT_TYPE();

ezMeshComponent::ezMeshComponent()
{
  m_iRenderPass = -1;
  m_MeshColor = ezColor::White;
}

void ezMeshComponent::SetMesh(const ezMeshResourceHandle& hMesh)
{
  m_hMesh = hMesh;

  if (IsActive() && GetOwner() != nullptr)
  {
    GetOwner()->UpdateLocalBounds();
  }
}

ezResult ezMeshComponent::OnAttachedToObject()
{
  if (IsActive() && m_hMesh.IsValid())
  {
    GetOwner()->UpdateLocalBounds();
  }

  return EZ_SUCCESS;
}

ezResult ezMeshComponent::OnDetachedFromObject()
{
  if (IsActive() && m_hMesh.IsValid())
  {
    GetOwner()->UpdateLocalBounds();
  }

  return EZ_SUCCESS;
}

void ezMeshComponent::OnUpdateLocalBounds(ezUpdateLocalBoundsMessage& msg) const
{
  if (!IsActive() || !m_hMesh.IsValid())
    return;

  ezResourceLock<ezMeshResource> pMesh(m_hMesh);
  msg.m_ResultingLocalBounds.ExpandToInclude(pMesh->GetBounds());
}

void ezMeshComponent::OnExtractRenderData(ezExtractRenderDataMessage& msg) const
{
  if (!IsActive() || !m_hMesh.IsValid())
    return;

  if (ezStringUtils::FindSubString_NoCase(GetOwner()->GetName(), "nopick") != nullptr)
  {
    ezTag tagIgnorePicking;
    ezTagRegistry::GetGlobalRegistry().RegisterTag("IgnorePicking", &tagIgnorePicking);
    m_Tags.Set(tagIgnorePicking);
  }

  ezRenderPipeline* pRenderPipeline = msg.m_pView->GetRenderPipeline();

  ezResourceLock<ezMeshResource> pMesh(m_hMesh);
  const ezDynamicArray<ezMeshResourceDescriptor::SubMesh>& parts = pMesh->GetSubMeshes();

  for (ezUInt32 uiPartIndex = 0; uiPartIndex < parts.GetCount(); ++uiPartIndex)
  {
    ezMeshRenderData* pRenderData = pRenderPipeline->CreateRenderData<ezMeshRenderData>(m_iRenderPass == -1 ? ezDefaultPassTypes::Opaque : m_iRenderPass, GetOwner());
    pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
    pRenderData->m_hMesh = m_hMesh;
    pRenderData->m_uiEditorPickingID = m_uiEditorPickingID;
    pRenderData->m_MeshColor = m_MeshColor;
    pRenderData->m_Tags = m_Tags;

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

void ezMeshComponent::SetMeshFile(const char* szFile)
{
  ezMeshResourceHandle hMesh;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hMesh = ezResourceManager::LoadResource<ezMeshResource>(szFile);
  }

  SetMesh(hMesh);
}

const char* ezMeshComponent::GetMeshFile() const
{
  if (!m_hMesh.IsValid())
    return "";

  ezResourceLock<ezMeshResource> pMesh(m_hMesh);
  return pMesh->GetResourceID();
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshComponent);

