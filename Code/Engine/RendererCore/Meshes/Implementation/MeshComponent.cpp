#include <PCH.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_MESSAGE_TYPE(ezMeshComponent_SetMaterialMsg);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshComponent_SetMaterialMsg, 1, ezRTTIDefaultAllocator<ezMeshComponent_SetMaterialMsg>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Material", GetMaterialFile, SetMaterialFile)->AddAttributes(new ezAssetBrowserAttribute("Material")),
    EZ_MEMBER_PROPERTY("MaterialSlot", m_uiMaterialSlot),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

void ezMeshComponent_SetMaterialMsg::SetMaterialFile(const char* szFile)
{
  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    m_hMaterial = ezResourceManager::LoadResource<ezMaterialResource>(szFile);
  }
  else
  {
    m_hMaterial.Invalidate();
  }
}

const char* ezMeshComponent_SetMaterialMsg::GetMaterialFile() const
{
  if (!m_hMaterial.IsValid())
    return "";

  return m_hMaterial.GetResourceID();
}

void ezMeshComponent_SetMaterialMsg::Serialize(ezStreamWriter& stream) const
{
  // has to be stringyfied for transfer
  stream << GetMaterialFile();
  stream << m_uiMaterialSlot;
}

void ezMeshComponent_SetMaterialMsg::Deserialize(ezStreamReader& stream, ezUInt8 uiTypeVersion)
{
  ezStringBuilder file;
  stream >> file;
  SetMaterialFile(file);

  stream >> m_uiMaterialSlot;
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshRenderData, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_COMPONENT_TYPE(ezMeshComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Mesh", GetMeshFile, SetMeshFile)->AddAttributes(new ezAssetBrowserAttribute("Mesh")),
    EZ_ARRAY_ACCESSOR_PROPERTY("Materials", Materials_GetCount, Materials_GetValue, Materials_SetValue, Materials_Insert, Materials_Remove)->AddAttributes(new ezAssetBrowserAttribute("Material")),
  }
  EZ_END_PROPERTIES
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Rendering"),
  }
  EZ_END_ATTRIBUTES
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezExtractRenderDataMessage, OnExtractRenderData),
    EZ_MESSAGE_HANDLER(ezMeshComponent_SetMaterialMsg, OnSetMaterialMsg),
  }
  EZ_END_MESSAGEHANDLERS
}
EZ_END_COMPONENT_TYPE

ezMeshComponent::ezMeshComponent()
{
  m_RenderDataCategory = ezInvalidIndex;
}

ezMeshComponent::~ezMeshComponent()
{

}

ezResult ezMeshComponent::GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible)
{
  if (m_hMesh.IsValid())
  {
    ezResourceLock<ezMeshResource> pMesh(m_hMesh);
    bounds = pMesh->GetBounds();
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

void ezMeshComponent::SetMesh(const ezMeshResourceHandle& hMesh)
{
  m_hMesh = hMesh;

  TriggerLocalBoundsUpdate();
}

void ezMeshComponent::OnSetMaterialMsg(ezMeshComponent_SetMaterialMsg& msg)
{
  SetMaterial(msg.m_uiMaterialSlot, msg.m_hMaterial);
}

void ezMeshComponent::SetMaterial(ezUInt32 uiIndex, const ezMaterialResourceHandle& hMaterial)
{
  if (uiIndex >= m_Materials.GetCount())
    m_Materials.SetCount(uiIndex + 1);

  m_Materials[uiIndex] = hMaterial;
}

ezMaterialResourceHandle ezMeshComponent::GetMaterial(ezUInt32 uiIndex) const
{
  if (uiIndex >= m_Materials.GetCount())
    return ezMaterialResourceHandle();

  return m_Materials[uiIndex];
}

void ezMeshComponent::OnExtractRenderData(ezExtractRenderDataMessage& msg) const
{
  if (!m_hMesh.IsValid())
    return;

  const ezUInt32 uiMeshIDHash = m_hMesh.GetResourceIDHash();

  const ezUInt32 uiFlipWinding = GetOwner()->GetGlobalTransformSimd().ContainsNegativeScale() ? 1 : 0;
  const ezUInt32 uiUniformScale = GetOwner()->GetGlobalTransformSimd().ContainsUniformScale() ? 1 : 0;

  ezResourceLock<ezMeshResource> pMesh(m_hMesh);
  const ezDynamicArray<ezMeshResourceDescriptor::SubMesh>& parts = pMesh->GetSubMeshes();

  for (ezUInt32 uiPartIndex = 0; uiPartIndex < parts.GetCount(); ++uiPartIndex)
  {
    const ezUInt32 uiMaterialIndex = parts[uiPartIndex].m_uiMaterialIndex;
    ezMaterialResourceHandle hMaterial;

    // If we have a material override, use that otherwise use the default mesh material.
    if (GetMaterial(uiMaterialIndex).IsValid())
      hMaterial = m_Materials[uiMaterialIndex];
    else
      hMaterial = pMesh->GetMaterials()[uiMaterialIndex];

    const ezUInt32 uiMaterialIDHash = hMaterial.IsValid() ? hMaterial.GetResourceIDHash() : 0;

    // Generate batch id from mesh, material and part index.
    ezUInt32 data[] = { uiMeshIDHash, uiMaterialIDHash, uiPartIndex, uiFlipWinding };
    ezUInt32 uiBatchId = ezHashing::MurmurHash(data, sizeof(data));

    ezMeshRenderData* pRenderData = CreateRenderData(uiBatchId);
    {
      pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
      pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
      pRenderData->m_hMesh = m_hMesh;
      pRenderData->m_hMaterial = hMaterial;

      pRenderData->m_uiPartIndex = uiPartIndex;
      pRenderData->m_uiFlipWinding = uiFlipWinding;
      pRenderData->m_uiUniformScale = uiUniformScale;

      pRenderData->m_uiUniqueID = GetUniqueID() | (uiMaterialIndex << 24);
    }

    // Determine render data category.
    ezRenderData::Category category;
    if (msg.m_OverrideCategory != ezInvalidIndex)
    {
      category = msg.m_OverrideCategory;
    }
    else if (m_RenderDataCategory != ezInvalidIndex)
    {
      category = m_RenderDataCategory;
    }
    else
    {
      if (hMaterial.IsValid())
      {
        ezResourceLock<ezMaterialResource> pMaterial(hMaterial, ezResourceAcquireMode::AllowFallback);
        ezTempHashedString blendModeValue = pMaterial->GetPermutationValue("BLEND_MODE");
        if (blendModeValue == "BLEND_MODE_OPAQUE" || blendModeValue == "")
        {
          category = ezDefaultRenderDataCategories::LitOpaque;
        }
        else if (blendModeValue == "BLEND_MODE_MASKED")
        {
          category = ezDefaultRenderDataCategories::LitMasked;
        }
        else
        {
          category = ezDefaultRenderDataCategories::LitTransparent;
        }
      }
      else
      {
        category = ezDefaultRenderDataCategories::LitOpaque;
      }
    }

    // Sort by material and then by mesh
    ezUInt32 uiSortingKey = (uiMaterialIDHash << 16) | (uiMeshIDHash & 0xFFFE) | uiFlipWinding;
    msg.m_pExtractedRenderData->AddRenderData(pRenderData, category, uiSortingKey);
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

  return m_hMesh.GetResourceID();
}


void ezMeshComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  ezStreamWriter& s = stream.GetStream();

  // ignore components that have created meshes (?)

  s << m_hMesh;
  s << m_RenderDataCategory;

  s << m_Materials.GetCount();

  for (const auto& mat : m_Materials)
  {
    s << mat;
  }
}

void ezMeshComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  //const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  ezStreamReader& s = stream.GetStream();

  s >> m_hMesh;
  s >> m_RenderDataCategory;

  ezUInt32 uiMaterials = 0;
  s >> uiMaterials;

  m_Materials.SetCount(uiMaterials);

  for (auto& mat : m_Materials)
  {
    s >> mat;
  }
}


ezMeshRenderData* ezMeshComponent::CreateRenderData(ezUInt32 uiBatchId) const
{
  return ezCreateRenderDataForThisFrame<ezMeshRenderData>(GetOwner(), uiBatchId);
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

  return hMat.GetResourceID();
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

