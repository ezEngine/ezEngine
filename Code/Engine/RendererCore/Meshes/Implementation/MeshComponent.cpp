#include <RendererCore/PCH.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/View.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshRenderData, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_COMPONENT_TYPE(ezMeshComponent, 1);
  EZ_BEGIN_PROPERTIES
    EZ_ACCESSOR_PROPERTY("Mesh", GetMeshFile, SetMeshFile)->AddAttributes(new ezAssetBrowserAttribute("Mesh")),
    EZ_MEMBER_PROPERTY("Mesh Color", m_MeshColor),
    EZ_ARRAY_ACCESSOR_PROPERTY("Materials", Materials_GetCount, Materials_GetValue, Materials_SetValue, Materials_Insert, Materials_Remove)->AddAttributes(new ezAssetBrowserAttribute("Material")),
  EZ_END_PROPERTIES
  EZ_BEGIN_ATTRIBUTES
    new ezCategoryAttribute("Graphics"),
  EZ_END_ATTRIBUTES
  EZ_BEGIN_MESSAGEHANDLERS
    EZ_MESSAGE_HANDLER(ezUpdateLocalBoundsMessage, OnUpdateLocalBounds),
    EZ_MESSAGE_HANDLER(ezExtractRenderDataMessage, OnExtractRenderData),
  EZ_END_MESSAGEHANDLERS
EZ_END_COMPONENT_TYPE();

ezMeshComponent::ezMeshComponent()
{
  m_RenderPass = ezInvalidIndex;
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

void ezMeshComponent::OnAfterAttachedToObject()
{
  if (IsActive() && m_hMesh.IsValid())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void ezMeshComponent::OnBeforeDetachedFromObject()
{
  if (IsActive() && m_hMesh.IsValid())
  {
    // temporay set to inactive so we don't receive the msg
    SetActive(false);
    GetOwner()->UpdateLocalBounds();
    SetActive(true);
  }
}

void ezMeshComponent::OnUpdateLocalBounds(ezUpdateLocalBoundsMessage& msg) const
{
  if (!m_hMesh.IsValid())
    return;

  ezResourceLock<ezMeshResource> pMesh(m_hMesh);
  if (pMesh->GetBounds().IsValid())
  {
    msg.m_ResultingLocalBounds.ExpandToInclude(pMesh->GetBounds());
  }
}

void ezMeshComponent::OnExtractRenderData(ezExtractRenderDataMessage& msg) const
{
  if (!m_hMesh.IsValid())
    return;

  ezRenderPipeline* pRenderPipeline = msg.m_pView->GetRenderPipeline();

  ezResourceLock<ezMeshResource> pMesh(m_hMesh);
  const ezDynamicArray<ezMeshResourceDescriptor::SubMesh>& parts = pMesh->GetSubMeshes();

  for (ezUInt32 uiPartIndex = 0; uiPartIndex < parts.GetCount(); ++uiPartIndex)
  {
    ezRenderPassType renderPass = ezDefaultPassTypes::Opaque;
    if (m_RenderPass != ezInvalidIndex)
    {
      renderPass = m_RenderPass;
    } 
    if (msg.m_OverrideRenderPass != ezInvalidIndex)
    {
      renderPass = msg.m_OverrideRenderPass;
    }

    const ezUInt32 uiMaterialIndex = parts[uiPartIndex].m_uiMaterialIndex;

    ezMeshRenderData* pRenderData = pRenderPipeline->CreateRenderData<ezMeshRenderData>(renderPass, GetOwner());
    pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
    pRenderData->m_hMesh = m_hMesh;
    pRenderData->m_uiEditorPickingID = m_uiEditorPickingID | (uiMaterialIndex << 24);
    pRenderData->m_MeshColor = m_MeshColor;
    // if we have a material override, use that
    // otherwise use the default mesh material
    if (GetMaterial(uiMaterialIndex).IsValid())
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


void ezMeshComponent::SerializeComponent(ezWorldWriter& stream) const
{
  ezStreamWriter& s = stream.GetStream();

  // ignore components that have created meshes (?)

  s << GetMeshFile();
  s << m_MeshColor;
  s << m_RenderPass;

  s << m_Materials.GetCount();

  for (const auto& mat : m_Materials)
  {
    if ( mat.IsValid() )
    {
      ezResourceLock<ezMaterialResource> pMat(mat);
      s << pMat->GetResourceID();
    }
    else
    {
      s << "";
    }
  }
}

void ezMeshComponent::DeserializeComponent(ezWorldReader& stream)
{
  ezStreamReader& s = stream.GetStream();

  ezStringBuilder sTemp;

  s >> sTemp;
  SetMeshFile(sTemp);

  s >> m_MeshColor;
  s >> m_RenderPass;

  ezUInt32 uiMaterials = 0;
  s >> uiMaterials;
  
  m_Materials.SetCount(uiMaterials);

  for (auto& mat : m_Materials)
  {
    s >> sTemp;

    if (!sTemp.IsEmpty())
      mat = ezResourceManager::LoadResource<ezMaterialResource>(sTemp);
  }
}

ezUInt32 ezMeshComponent::Materials_GetCount() const
{
  return m_Materials.GetCount();
}


const char* ezMeshComponent::Materials_GetValue(ezUInt32 uiIndex) const
{
  auto hMat = GetMaterial(uiIndex);

  if (!hMat.IsValid())
    return "";

  ezResourceLock<ezMaterialResource> pMat(hMat, ezResourceAcquireMode::PointerOnly);
  return pMat->GetResourceID();
}


void ezMeshComponent::Materials_SetValue(ezUInt32 uiIndex, const char* value)
{
  if (ezStringUtils::IsNullOrEmpty(value))
    SetMaterial(uiIndex, ezMaterialResourceHandle());
  else
  {
    auto hMat = ezResourceManager::LoadResource<ezMaterialResource>(value);
    SetMaterial(uiIndex, hMat);
  }
}


void ezMeshComponent::Materials_Insert(ezUInt32 uiIndex, const char* value)
{
  ezMaterialResourceHandle hMat;

  if (!ezStringUtils::IsNullOrEmpty(value))
    hMat = ezResourceManager::LoadResource<ezMaterialResource>(value);

  m_Materials.Insert(hMat, uiIndex);
}


void ezMeshComponent::Materials_Remove(ezUInt32 uiIndex)
{
  m_Materials.RemoveAt(uiIndex);
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshComponent);

