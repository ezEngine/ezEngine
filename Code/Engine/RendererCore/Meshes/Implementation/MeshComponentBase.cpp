#include <RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererCore/Messages/SetColorMessage.h>
#include <RendererFoundation/Device/Device.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgSetMeshMaterial);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgSetMeshMaterial, 1, ezRTTIDefaultAllocator<ezMsgSetMeshMaterial>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Material", GetMaterialFile, SetMaterialFile)->AddAttributes(new ezAssetBrowserAttribute("Material")),
    EZ_MEMBER_PROPERTY("MaterialSlot", m_uiMaterialSlot),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezAutoGenVisScriptMsgSender,
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezMsgSetMeshMaterial::SetMaterialFile(const char* szFile)
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

const char* ezMsgSetMeshMaterial::GetMaterialFile() const
{
  if (!m_hMaterial.IsValid())
    return "";

  return m_hMaterial.GetResourceID();
}

void ezMsgSetMeshMaterial::Serialize(ezStreamWriter& stream) const
{
  // has to be stringyfied for transfer
  stream << GetMaterialFile();
  stream << m_uiMaterialSlot;
}

void ezMsgSetMeshMaterial::Deserialize(ezStreamReader& stream, ezUInt8 uiTypeVersion)
{
  ezStringBuilder file;
  stream >> file;
  SetMaterialFile(file);

  stream >> m_uiMaterialSlot;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshRenderData, 1, ezRTTIDefaultAllocator<ezMeshRenderData>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezMeshRenderData::FillBatchIdAndSortingKey()
{
  FillBatchIdAndSortingKeyInternal(0);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezMeshComponentBase, 1)
  {
    EZ_BEGIN_ATTRIBUTES
    {
      new ezCategoryAttribute("Rendering"),
    } EZ_END_ATTRIBUTES;
    EZ_BEGIN_MESSAGEHANDLERS
    {
      EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnExtractRenderData),
      EZ_MESSAGE_HANDLER(ezMsgSetMeshMaterial, OnSetMaterial),
      EZ_MESSAGE_HANDLER(ezMsgSetColor, OnSetColor),
    } EZ_END_MESSAGEHANDLERS;
  }
EZ_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

ezMeshComponentBase::ezMeshComponentBase()
{
  m_RenderDataCategory = ezInvalidRenderDataCategory;
  m_Color = ezColor::White;
}

ezMeshComponentBase::~ezMeshComponentBase() = default;

void ezMeshComponentBase::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  ezStreamWriter& s = stream.GetStream();

  // ignore components that have created meshes (?)

  s << m_hMesh;

  ezUInt32 uiCategory = m_RenderDataCategory.m_uiValue;
  s << uiCategory;

  s << m_Materials.GetCount();

  for (const auto& mat : m_Materials)
  {
    s << mat;
  }

  s << m_Color;
}

void ezMeshComponentBase::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  ezStreamReader& s = stream.GetStream();

  s >> m_hMesh;

  ezUInt32 uiCategory = 0;
  s >> uiCategory;
  m_RenderDataCategory.m_uiValue = uiCategory;

  ezUInt32 uiMaterials = 0;
  s >> uiMaterials;

  m_Materials.SetCount(uiMaterials);

  for (auto& mat : m_Materials)
  {
    s >> mat;
  }

  s >> m_Color;
}

ezResult ezMeshComponentBase::GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible)
{
  if (m_hMesh.IsValid())
  {
    ezResourceLock<ezMeshResource> pMesh(m_hMesh, ezResourceAcquireMode::AllowFallback);
    bounds = pMesh->GetBounds();
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

void ezMeshComponentBase::OnExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (!m_hMesh.IsValid())
    return;

  ezResourceLock<ezMeshResource> pMesh(m_hMesh, ezResourceAcquireMode::AllowFallback);
  ezArrayPtr<const ezMeshResourceDescriptor::SubMesh> parts = pMesh->GetSubMeshes();

  for (ezUInt32 uiPartIndex = 0; uiPartIndex < parts.GetCount(); ++uiPartIndex)
  {
    const ezUInt32 uiMaterialIndex = parts[uiPartIndex].m_uiMaterialIndex;
    ezMaterialResourceHandle hMaterial;

    // If we have a material override, use that otherwise use the default mesh material.
    if (GetMaterial(uiMaterialIndex).IsValid())
      hMaterial = m_Materials[uiMaterialIndex];
    else
      hMaterial = pMesh->GetMaterials()[uiMaterialIndex];

    ezMeshRenderData* pRenderData = CreateRenderData();
    {
      pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
      pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
      pRenderData->m_hMesh = m_hMesh;
      pRenderData->m_hMaterial = hMaterial;
      pRenderData->m_Color = m_Color;
      pRenderData->m_uiSubMeshIndex = uiPartIndex;
      pRenderData->m_uiUniqueID = GetUniqueIdForRendering(uiMaterialIndex);

      pRenderData->FillBatchIdAndSortingKey();
    }

    // Determine render data category.
    ezRenderData::Category category = m_RenderDataCategory;
    if (category == ezInvalidRenderDataCategory)
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

    msg.AddRenderData(pRenderData, category, ezRenderData::Caching::IfStatic);
  }
}

void ezMeshComponentBase::SetMesh(const ezMeshResourceHandle& hMesh)
{
  m_hMesh = hMesh;

  TriggerLocalBoundsUpdate();
}

void ezMeshComponentBase::SetMaterial(ezUInt32 uiIndex, const ezMaterialResourceHandle& hMaterial)
{
  m_Materials.EnsureCount(uiIndex + 1);

  m_Materials[uiIndex] = hMaterial;
}

ezMaterialResourceHandle ezMeshComponentBase::GetMaterial(ezUInt32 uiIndex) const
{
  if (uiIndex >= m_Materials.GetCount())
    return ezMaterialResourceHandle();

  return m_Materials[uiIndex];
}

void ezMeshComponentBase::SetMeshFile(const char* szFile)
{
  ezMeshResourceHandle hMesh;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hMesh = ezResourceManager::LoadResource<ezMeshResource>(szFile);
  }

  SetMesh(hMesh);
}

const char* ezMeshComponentBase::GetMeshFile() const
{
  if (!m_hMesh.IsValid())
    return "";

  return m_hMesh.GetResourceID();
}

void ezMeshComponentBase::SetColor(const ezColor& color)
{
  m_Color = color;
}

const ezColor& ezMeshComponentBase::GetColor() const
{
  return m_Color;
}

void ezMeshComponentBase::OnSetMaterial(ezMsgSetMeshMaterial& msg)
{
  SetMaterial(msg.m_uiMaterialSlot, msg.m_hMaterial);
}

void ezMeshComponentBase::OnSetColor(ezMsgSetColor& msg)
{
  msg.ModifyColor(m_Color);
}

ezMeshRenderData* ezMeshComponentBase::CreateRenderData() const
{
  return ezCreateRenderDataForThisFrame<ezMeshRenderData>(GetOwner());
}

ezUInt32 ezMeshComponentBase::Materials_GetCount() const
{
  return m_Materials.GetCount();
}


const char* ezMeshComponentBase::Materials_GetValue(ezUInt32 uiIndex) const
{
  auto hMat = GetMaterial(uiIndex);

  if (!hMat.IsValid())
    return "";

  return hMat.GetResourceID();
}


void ezMeshComponentBase::Materials_SetValue(ezUInt32 uiIndex, const char* value)
{
  if (ezStringUtils::IsNullOrEmpty(value))
    SetMaterial(uiIndex, ezMaterialResourceHandle());
  else
  {
    auto hMat = ezResourceManager::LoadResource<ezMaterialResource>(value);
    SetMaterial(uiIndex, hMat);
  }
}


void ezMeshComponentBase::Materials_Insert(ezUInt32 uiIndex, const char* value)
{
  ezMaterialResourceHandle hMat;

  if (!ezStringUtils::IsNullOrEmpty(value))
    hMat = ezResourceManager::LoadResource<ezMaterialResource>(value);

  m_Materials.Insert(hMat, uiIndex);
}


void ezMeshComponentBase::Materials_Remove(ezUInt32 uiIndex)
{
  m_Materials.RemoveAtAndCopy(uiIndex);
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshComponentBase);
