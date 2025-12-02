#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererFoundation/Device/Device.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgSetMeshMaterial);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgSetMeshMaterial, 1, ezRTTIDefaultAllocator<ezMsgSetMeshMaterial>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_RESOURCE_MEMBER_PROPERTY("Material", m_hMaterial)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Material")),
    EZ_MEMBER_PROPERTY("MaterialSlot", m_uiMaterialSlot),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezMsgSetMeshMaterial::Serialize(ezStreamWriter& inout_stream) const
{
  // has to be stringified for transfer
  inout_stream << GetMaterialFile();
  inout_stream << m_uiMaterialSlot;
}

void ezMsgSetMeshMaterial::Deserialize(ezStreamReader& inout_stream, ezUInt8 uiTypeVersion)
{
  ezStringBuilder file;
  inout_stream >> file;
  SetMaterialFile(file);

  inout_stream >> m_uiMaterialSlot;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshRenderData, 1, ezRTTIDefaultAllocator<ezMeshRenderData>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
static_assert(sizeof(ezMeshRenderData) == 120);
#else
static_assert(sizeof(ezMeshRenderData) == 88);
#endif

void ezMeshRenderData::FillSortingKey()
{
  const ezUInt32 uiMeshIDHash = ezHashingUtils::StringHashTo32(m_hMesh.GetResourceIDHash());
  const ezUInt32 uiMaterialIDHash = m_hMaterial.IsValid() ? ezHashingUtils::StringHashTo32(m_hMaterial.GetResourceIDHash()) : 0;
  const ezUInt32 uiFlipWinding = m_Flags.IsSet(Flags::FlipWinding) ? 1 : 0;

  // Sort by material and then by mesh
  m_uiSortingKey = (uiMaterialIDHash << 16) | ((uiMeshIDHash + m_uiSubMeshIndex) & 0xFFFE) | uiFlipWinding;
}

bool ezMeshRenderData::CanBatch(const ezRenderData& other0) const
{
  const auto& other = ezStaticCast<const ezMeshRenderData&>(other0);

  return m_hCustomInstanceDataBuffer == other.m_hCustomInstanceDataBuffer &&
         m_hMesh == other.m_hMesh && m_uiSubMeshIndex == other.m_uiSubMeshIndex &&
         m_hMaterial == other.m_hMaterial &&
         CanBatchByBaseValues(other);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezMeshComponentBase, 4)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Rendering"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnMsgExtractRenderData),
    EZ_MESSAGE_HANDLER(ezMsgSetMeshMaterial, OnMsgSetMeshMaterial),
    EZ_MESSAGE_HANDLER(ezMsgSetColor, OnMsgSetColor),
    EZ_MESSAGE_HANDLER(ezMsgSetCustomData, OnMsgSetCustomData),
  } EZ_END_MESSAGEHANDLERS;
}
EZ_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

ezMeshComponentBase::ezMeshComponentBase() = default;
ezMeshComponentBase::~ezMeshComponentBase() = default;

void ezMeshComponentBase::OnDeactivated()
{
  DeleteInstanceData();

  SUPER::OnDeactivated();
}

void ezMeshComponentBase::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  ezStreamWriter& s = inout_stream.GetStream();

  // ignore components that have created meshes (?)

  s << m_hMesh;

  s << m_Materials.GetCount();
  for (const auto& mat : m_Materials)
  {
    s << mat;
  }

  s << m_Color;
  s << m_fSortingDepthOffset;
  s << m_vCustomData;
}

void ezMeshComponentBase::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  ezStreamReader& s = inout_stream.GetStream();

  s >> m_hMesh;

  if (uiVersion < 2)
  {
    ezUInt32 uiCategory = 0;
    s >> uiCategory;
  }

  ezUInt32 uiMaterials = 0;
  s >> uiMaterials;

  m_Materials.SetCount(uiMaterials);

  for (auto& mat : m_Materials)
  {
    s >> mat;
  }

  s >> m_Color;

  if (uiVersion >= 3)
  {
    s >> m_fSortingDepthOffset;
  }

  if (uiVersion >= 4)
  {
    s >> m_vCustomData;
  }
}

ezResult ezMeshComponentBase::GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg)
{
  if (m_hMesh.IsValid())
  {
    ezResourceLock<ezMeshResource> pMesh(m_hMesh, ezResourceAcquireMode::AllowLoadingFallback);
    ref_bounds = pMesh->GetBounds();
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

void ezMeshComponentBase::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (!m_hMesh.IsValid())
    return;

  const bool bDynamic = GetOwner()->IsDynamic();
  const ezTransform finalTransform = GetFinalGlobalTransform();
  auto hInstanceDataBuffer = msg.m_pRenderDataManager->GetOrCreateInstanceDataAndFill(*this, bDynamic, finalTransform, m_InstanceDataOffset, GetUniqueIdForRendering(), m_Color, m_vCustomData);

  ezResourceLock<ezMeshResource> pMesh(m_hMesh, ezResourceAcquireMode::AllowLoadingFallback);
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

    ezMeshRenderData* pRenderData = CreateRenderData(msg.m_pRenderDataManager);
    {
      // Already done in CreateRenderDataForThisFrame but only with the owner's transform. We need to use the final transform here.
      pRenderData->m_vGlobalPosition = finalTransform.m_vPosition;
      pRenderData->m_Flags.AddOrRemove(ezRenderData::Flags::FlipWinding, finalTransform.HasMirrorScaling());

      pRenderData->m_fSortingDepthOffset = m_fSortingDepthOffset;
      pRenderData->m_DataOffsets.m_uiCustomInstance = m_CustomInstanceDataOffset.m_uiOffset;
      pRenderData->m_hCustomInstanceDataBuffer = m_hCustomInstanceDataBuffer;

      pRenderData->SetFallbackGlobalBounds(GetOwner()->GetGlobalBounds());
      pRenderData->Fill(m_InstanceDataOffset, hInstanceDataBuffer, hMaterial, m_hMesh, uiMaterialIndex, uiPartIndex);
    }

    bool bDontCacheYet = false;
    ezRenderData::Category category = ezMaterialResource::GetRenderDataCategory(hMaterial, &bDontCacheYet);

    msg.AddRenderData(pRenderData, category, bDontCacheYet ? ezRenderData::Caching::Never : ezRenderData::Caching::IfStatic);
  }
}

void ezMeshComponentBase::DeleteInstanceData()
{
  if (ezRenderDataManager* pRenderDataManager = GetWorld()->GetModule<ezRenderDataManager>())
  {
    pRenderDataManager->DeleteInstanceData(m_InstanceDataOffset);
  }
  else
  {
    EZ_ASSERT_DEBUG(m_InstanceDataOffset.IsInvalidated(), "Implementation error");
  }
}

void ezMeshComponentBase::SetMesh(const ezMeshResourceHandle& hMesh)
{
  if (m_hMesh != hMesh)
  {
    m_hMesh = hMesh;

    TriggerLocalBoundsUpdate();
    InvalidateCachedRenderData();
  }
}

void ezMeshComponentBase::SetMaterial(ezUInt32 uiIndex, const ezMaterialResourceHandle& hMaterial)
{
  if (uiIndex >= 1024)
  {
    ezLog::Error("Invalid material slot index used to change mesh component material.");
    return;
  }

  m_Materials.EnsureCount(uiIndex + 1);

  if (m_Materials[uiIndex] != hMaterial)
  {
    m_Materials[uiIndex] = hMaterial;

    InvalidateCachedRenderData();
  }
}

ezMaterialResourceHandle ezMeshComponentBase::GetMaterial(ezUInt32 uiIndex) const
{
  if (uiIndex >= m_Materials.GetCount())
    return ezMaterialResourceHandle();

  return m_Materials[uiIndex];
}

void ezMeshComponentBase::SetColor(const ezColor& color)
{
  if (m_Color != color)
  {
    m_Color = color;

    InvalidateCachedRenderData();
  }
}

void ezMeshComponentBase::SetCustomData(const ezVec4& vData)
{
  m_vCustomData = vData;

  InvalidateCachedRenderData();
}

void ezMeshComponentBase::SetSortingDepthOffset(float fOffset)
{
  m_fSortingDepthOffset = fOffset;

  InvalidateCachedRenderData();
}

void ezMeshComponentBase::OnMsgSetMeshMaterial(ezMsgSetMeshMaterial& ref_msg)
{
  SetMaterial(ref_msg.m_uiMaterialSlot, ref_msg.m_hMaterial);
}

void ezMeshComponentBase::OnMsgSetColor(ezMsgSetColor& ref_msg)
{
  ezColor newColor = m_Color;
  ref_msg.ModifyColor(newColor);

  if (m_Color != newColor)
  {
    m_Color = newColor;

    InvalidateCachedRenderData();
  }
}

void ezMeshComponentBase::OnMsgSetCustomData(ezMsgSetCustomData& ref_msg)
{
  m_vCustomData.Set(ref_msg.m_fData0, ref_msg.m_fData1, ref_msg.m_fData2, ref_msg.m_fData3);
  InvalidateCachedRenderData();
}

void ezMeshComponentBase::SetCustomInstanceData(ezCustomInstanceDataOffset offset, ezGALDynamicBufferHandle hBuffer)
{
  if (m_CustomInstanceDataOffset.m_uiOffset != offset.m_uiOffset || m_hCustomInstanceDataBuffer != hBuffer)
  {
    m_CustomInstanceDataOffset = offset;
    m_hCustomInstanceDataBuffer = hBuffer;

    InvalidateCachedRenderData();
  }
}

ezTransform ezMeshComponentBase::GetFinalGlobalTransform() const
{
  return GetOwner()->GetGlobalTransform();
}

ezMeshRenderData* ezMeshComponentBase::CreateRenderData(const ezRenderDataManager* pRenderDataManager) const
{
  return pRenderDataManager->CreateRenderDataForThisFrame<ezMeshRenderData>(GetOwner());
}

ezUInt32 ezMeshComponentBase::Materials_GetCount() const
{
  return m_Materials.GetCount();
}

ezString ezMeshComponentBase::Materials_GetValue(ezUInt32 uiIndex) const
{
  return GetMaterial(uiIndex).GetResourceID();
}

void ezMeshComponentBase::Materials_SetValue(ezUInt32 uiIndex, ezString sValue)
{
  if (sValue.IsEmpty())
    SetMaterial(uiIndex, ezMaterialResourceHandle());
  else
  {
    auto hMat = ezResourceManager::LoadResource<ezMaterialResource>(sValue);
    SetMaterial(uiIndex, hMat);
  }
}

void ezMeshComponentBase::Materials_Insert(ezUInt32 uiIndex, ezString sValue)
{
  ezMaterialResourceHandle hMat;

  if (!sValue.IsEmpty())
    hMat = ezResourceManager::LoadResource<ezMaterialResource>(sValue);

  m_Materials.InsertAt(uiIndex, hMat);

  InvalidateCachedRenderData();
}

void ezMeshComponentBase::Materials_Remove(ezUInt32 uiIndex)
{
  m_Materials.RemoveAtAndCopy(uiIndex);

  InvalidateCachedRenderData();
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshComponentBase);
