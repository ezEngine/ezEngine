#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Meshes/InstancedMeshComponent.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezMeshInstanceData, ezNoBase, 1, ezRTTIDefaultAllocator<ezMeshInstanceData>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("LocalPosition", GetLocalPosition, SetLocalPosition)->AddAttributes(new ezSuffixAttribute(" m")),
    EZ_ACCESSOR_PROPERTY("LocalRotation", GetLocalRotation, SetLocalRotation),
    EZ_ACCESSOR_PROPERTY("LocalScaling", GetLocalScaling, SetLocalScaling)->AddAttributes(new ezDefaultValueAttribute(ezVec3(1.0f, 1.0f, 1.0f))),

    EZ_MEMBER_PROPERTY("Color", m_color)
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezTransformManipulatorAttribute("LocalPosition", "LocalRotation", "LocalScaling"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_STATIC_REFLECTED_TYPE
// clang-format on

void ezMeshInstanceData::SetLocalPosition(ezVec3 vPosition)
{
  m_transform.m_vPosition = vPosition;
}
ezVec3 ezMeshInstanceData::GetLocalPosition() const
{
  return m_transform.m_vPosition;
}

void ezMeshInstanceData::SetLocalRotation(ezQuat qRotation)
{
  m_transform.m_qRotation = qRotation;
}

ezQuat ezMeshInstanceData::GetLocalRotation() const
{
  return m_transform.m_qRotation;
}

void ezMeshInstanceData::SetLocalScaling(ezVec3 vScaling)
{
  m_transform.m_vScale = vScaling;
}

ezVec3 ezMeshInstanceData::GetLocalScaling() const
{
  return m_transform.m_vScale;
}

static constexpr ezTypeVersion s_MeshInstanceDataVersion = 1;
ezResult ezMeshInstanceData::Serialize(ezStreamWriter& ref_writer) const
{
  ref_writer.WriteVersion(s_MeshInstanceDataVersion);

  ref_writer << m_transform;
  ref_writer << m_color;

  return EZ_SUCCESS;
}

ezResult ezMeshInstanceData::Deserialize(ezStreamReader& ref_reader)
{
  /*auto version = */ ref_reader.ReadVersion(s_MeshInstanceDataVersion);

  ref_reader >> m_transform;
  ref_reader >> m_color;

  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezInstancedMeshComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_RESOURCE_ACCESSOR_PROPERTY("Mesh", GetMesh, SetMesh)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Mesh_Static")),
    EZ_ACCESSOR_PROPERTY("MainColor", GetColor, SetColor)->AddAttributes(new ezExposeColorAlphaAttribute()),
    EZ_ACCESSOR_PROPERTY("CustomData", GetCustomData, SetCustomData)->AddAttributes(new ezDefaultValueAttribute(ezVec4(0, 1, 0, 1))),
    EZ_ARRAY_ACCESSOR_PROPERTY("Materials", Materials_GetCount, Materials_GetValue, Materials_SetValue, Materials_Insert, Materials_Remove)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Material")),

    EZ_ARRAY_ACCESSOR_PROPERTY("InstanceData", Instances_GetCount, Instances_GetValue, Instances_SetValue, Instances_Insert, Instances_Remove),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractGeometry, OnMsgExtractGeometry),
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnMsgExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezInstancedMeshComponent::ezInstancedMeshComponent() = default;
ezInstancedMeshComponent::~ezInstancedMeshComponent() = default;

void ezInstancedMeshComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  inout_stream.GetStream().WriteArray(m_RawInstancedData).IgnoreResult();
}

void ezInstancedMeshComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);

  inout_stream.GetStream().ReadArray(m_RawInstancedData).IgnoreResult();
}

void ezInstancedMeshComponent::OnMsgExtractGeometry(ezMsgExtractGeometry& ref_msg) {}

ezResult ezInstancedMeshComponent::GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg)
{
  if (!m_hMesh.IsValid() || m_RawInstancedData.IsEmpty())
    return EZ_FAILURE;

  ezResourceLock<ezMeshResource> pMesh(m_hMesh, ezResourceAcquireMode::AllowLoadingFallback);
  ezBoundingBoxSphere singleBounds = pMesh->GetBounds();
  m_fBoundingSphereRadius = singleBounds.m_fSphereRadius;

  for (const auto& instance : m_RawInstancedData)
  {
    auto instanceBounds = singleBounds;
    instanceBounds.Transform(instance.m_transform.GetAsMat4());

    ref_bounds.ExpandToInclude(instanceBounds);
  }

  return EZ_SUCCESS;
}

void ezInstancedMeshComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (!m_hMesh.IsValid() || m_RawInstancedData.IsEmpty())
    return;

  const bool bDynamic = GetOwner()->IsDynamic();
  ezGALDynamicBufferHandle hInstanceDataBuffer;
  auto instanceData = msg.m_pRenderDataManager->GetOrCreateInstanceData(this, bDynamic, hInstanceDataBuffer, m_InstanceDataOffset, m_RawInstancedData.GetCount());

  const ezTransform ownerTransform = GetOwner()->GetGlobalTransform();
  const ezUInt32 uiUniqueID = GetUniqueIdForRendering();
  const ezUInt32 uiRandomSeed = GetOwner()->GetStableRandomSeed();
  for (ezUInt32 i = 0; i < m_RawInstancedData.GetCount(); ++i)
  {
    auto& meshInstance = m_RawInstancedData[i];
    const ezTransform globalTransform = ownerTransform * meshInstance.m_transform;
    const ezColor color = m_Color * meshInstance.m_color;

    ezRenderDataManager::FillPerInstanceData(instanceData[i], nullptr, globalTransform, uiUniqueID, color, m_vCustomData, m_fBoundingSphereRadius, uiRandomSeed + i);
  }

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
    pRenderData->SetFallbackGlobalBounds(GetOwner()->GetGlobalBounds());
    pRenderData->Fill(m_InstanceDataOffset, hInstanceDataBuffer, hMaterial, m_hMesh, uiMaterialIndex, uiPartIndex, m_RawInstancedData.GetCount());

    bool bDontCacheYet = false;
    ezRenderData::Category category = ezMaterialResource::GetRenderDataCategory(hMaterial, &bDontCacheYet);

    msg.AddRenderData(pRenderData, category, bDontCacheYet ? ezRenderData::Caching::Never : ezRenderData::Caching::IfStatic);
  }
}

ezUInt32 ezInstancedMeshComponent::Instances_GetCount() const
{
  return m_RawInstancedData.GetCount();
}

ezMeshInstanceData ezInstancedMeshComponent::Instances_GetValue(ezUInt32 uiIndex) const
{
  return m_RawInstancedData[uiIndex];
}

void ezInstancedMeshComponent::Instances_SetValue(ezUInt32 uiIndex, ezMeshInstanceData value)
{
  m_RawInstancedData[uiIndex] = value;

  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

void ezInstancedMeshComponent::Instances_Insert(ezUInt32 uiIndex, ezMeshInstanceData value)
{
  m_RawInstancedData.InsertAt(uiIndex, value);

  TriggerLocalBoundsUpdate();
  DeleteInstanceData();
  InvalidateCachedRenderData();
}

void ezInstancedMeshComponent::Instances_Remove(ezUInt32 uiIndex)
{
  m_RawInstancedData.RemoveAtAndCopy(uiIndex);

  TriggerLocalBoundsUpdate();
  DeleteInstanceData();
  InvalidateCachedRenderData();
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_InstancedMeshComponent);
