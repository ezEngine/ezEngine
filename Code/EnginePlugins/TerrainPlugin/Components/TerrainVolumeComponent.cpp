#include <TerrainPlugin/TerrainPluginPCH.h>

#include <Core/Messages/TransformChangedMessage.h>
#include <Core/Physics/SurfaceResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Types/TagRegistry.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <TerrainPlugin/Components/TerrainVolumeComponent.h>
#include <TerrainPlugin/Rendering/TerrainRenderData.h>
#include <TerrainPlugin/TerrainSystem.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezTerrainVolumeComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_ACCESSOR_PROPERTY("Resolution", ezTerrainResolution, GetResolution, SetResolution),
    EZ_ACCESSOR_PROPERTY("Size", GetSize, SetSize)->AddAttributes(new ezClampValueAttribute(1.0f, 1024.0f), new ezDefaultValueAttribute(64.0f)),
    EZ_RESOURCE_ACCESSOR_PROPERTY("Material", GetMaterial, SetMaterial)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Material", "Terrain-Voxel")),
    EZ_ACCESSOR_PROPERTY("BaseMaterialIndex", GetBaseMaterialIndex, SetBaseMaterialIndex)->AddAttributes(new ezClampValueAttribute(0, 15)),
    EZ_ACCESSOR_PROPERTY("FillHeight", GetFillHeight, SetFillHeight)->AddAttributes(new ezDefaultValueAttribute(-0.01f), new ezClampValueAttribute(-0.01f, 100.0f), new ezMinValueTextAttribute("Off")),
    EZ_ACCESSOR_PROPERTY("EnableCollider", GetEnableCollider, SetEnableCollider)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_ACCESSOR_PROPERTY("CleanupIterations", GetCleanupIterations, SetCleanupIterations)->AddAttributes(new ezDefaultValueAttribute(2), new ezClampValueAttribute(0, 4)),
    EZ_SET_ACCESSOR_PROPERTY("TerrainTags", GetTags, Reflection_SetTag, Reflection_RemoveTag)->AddAttributes(new ezTagSetWidgetAttribute("Terrain")),
    EZ_ARRAY_ACCESSOR_PROPERTY("Surfaces", Surfaces_GetCount, Surfaces_GetValue, Surfaces_SetValue, Surfaces_Insert, Surfaces_Remove)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Surface", ezDependencyFlags::Package)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgTransformChanged, OnMsgTransformChanged),
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnMsgExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_FUNCTION_PROPERTY(OnObjectCreated),
  }
  EZ_END_FUNCTIONS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Terrain"),
    new ezBoxVisualizerAttribute("Size", 1.0f, ezColorScheme::LightUI(ezColorScheme::Green), nullptr, ezVisualizerAnchor::NegX | ezVisualizerAnchor::NegY | ezVisualizerAnchor::NegZ),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezTerrainVolumeComponent::ezTerrainVolumeComponent() = default;
ezTerrainVolumeComponent::~ezTerrainVolumeComponent() = default;

void ezTerrainVolumeComponent::SetResolution(ezEnum<ezTerrainResolution> resolution)
{
  if (m_Resolution == resolution)
    return;

  m_Resolution = resolution;

  if (m_uiVoxelIndex != ezInvalidIndex)
  {
    if (auto* pSystem = GetWorld()->GetModule<ezTerrainSystem>())
    {
      pSystem->RemoveVoxelTerrain(m_uiVoxelIndex);
      const float fVoxelSize = m_fSize / static_cast<float>(m_Resolution.GetValue());
      m_uiVoxelIndex = pSystem->CreateVoxelTerrain(m_Resolution.GetValue(), fVoxelSize);
      auto& vol = pSystem->ModifyVoxelTerrain(m_uiVoxelIndex);
      vol.m_GlobalTransform = GetOwner()->GetGlobalTransform();
      vol.m_bInitialSolid = (m_fFillHeight >= 0.0f);
      vol.m_fFillHeight = m_fFillHeight;
      vol.m_uiCleanupIterations = m_uiCleanupIterations;
      vol.m_Tags = m_Tags;
    }
  }

  TriggerLocalBoundsUpdate();
}

void ezTerrainVolumeComponent::SetSize(float f)
{
  if (m_fSize == f)
    return;

  m_fSize = f;

  if (m_uiVoxelIndex != ezInvalidIndex)
  {
    if (auto* pSystem = GetWorld()->GetModule<ezTerrainSystem>())
    {
      pSystem->ModifyVoxelTerrain(m_uiVoxelIndex).m_fVoxelSize = m_fSize / static_cast<float>(m_Resolution.GetValue());
    }
  }

  TriggerLocalBoundsUpdate();
}

void ezTerrainVolumeComponent::SetMaterial(const ezMaterialResourceHandle& hMaterial)
{
  m_hMaterial = hMaterial;
  InvalidateCachedRenderData();
}

void ezTerrainVolumeComponent::SetBaseMaterialIndex(ezUInt8 i)
{
  m_uiBaseMaterialIndex = i;
  InvalidateCachedRenderData();
}

void ezTerrainVolumeComponent::SetFillHeight(float f)
{
  if (m_fFillHeight == f)
    return;

  m_fFillHeight = f;

  if (m_uiVoxelIndex != ezInvalidIndex)
  {
    if (auto* pSystem = GetWorld()->GetModule<ezTerrainSystem>())
    {
      auto& vol = pSystem->ModifyVoxelTerrain(m_uiVoxelIndex);
      vol.m_fFillHeight = f;
      vol.m_bInitialSolid = (f >= 0.0f);
    }
  }
}

void ezTerrainVolumeComponent::SetEnableCollider(bool bEnable)
{
  m_bEnableCollider = bEnable;
}

void ezTerrainVolumeComponent::SetCleanupIterations(ezUInt8 n)
{
  n = ezMath::Clamp(n, (ezUInt8)0, (ezUInt8)4);

  if (m_uiCleanupIterations == n)
    return;

  m_uiCleanupIterations = n;

  if (m_uiVoxelIndex != ezInvalidIndex)
  {
    if (auto* pSystem = GetWorld()->GetModule<ezTerrainSystem>())
      pSystem->ModifyVoxelTerrain(m_uiVoxelIndex).m_uiCleanupIterations = m_uiCleanupIterations;
  }
}

void ezTerrainVolumeComponent::Reflection_SetTag(const char* szTagName)
{
  if (ezStringUtils::IsNullOrEmpty(szTagName))
    return;

  const ezTag& tag = ezTagRegistry::GetGlobalRegistry().RegisterTag(szTagName);
  if (m_Tags.IsSet(tag))
    return;

  m_Tags.Set(tag);

  if (m_uiVoxelIndex != ezInvalidIndex)
  {
    if (auto* pSystem = GetWorld()->GetModule<ezTerrainSystem>())
      pSystem->ModifyVoxelTerrain(m_uiVoxelIndex).m_Tags = m_Tags;
  }
}

void ezTerrainVolumeComponent::Reflection_RemoveTag(const char* szTagName)
{
  if (ezStringUtils::IsNullOrEmpty(szTagName))
    return;

  if (const ezTag* pTag = ezTagRegistry::GetGlobalRegistry().GetTagByName(ezTempHashedString(szTagName)))
  {
    if (!m_Tags.IsSet(*pTag))
      return;

    m_Tags.Remove(*pTag);

    if (m_uiVoxelIndex != ezInvalidIndex)
    {
      if (auto* pSystem = GetWorld()->GetModule<ezTerrainSystem>())
        pSystem->ModifyVoxelTerrain(m_uiVoxelIndex).m_Tags = m_Tags;
    }
  }
}

ezUInt32 ezTerrainVolumeComponent::Surfaces_GetCount() const
{
  return m_Surfaces.GetCount();
}

ezString ezTerrainVolumeComponent::Surfaces_GetValue(ezUInt32 uiIndex) const
{
  if (uiIndex >= m_Surfaces.GetCount() || !m_Surfaces[uiIndex].IsValid())
    return {};

  return m_Surfaces[uiIndex].GetResourceID();
}

void ezTerrainVolumeComponent::Surfaces_SetValue(ezUInt32 uiIndex, ezString sValue)
{
  m_Surfaces.EnsureCount(uiIndex + 1);

  if (!sValue.IsEmpty())
    m_Surfaces[uiIndex] = ezResourceManager::LoadResource<ezSurfaceResource>(sValue);
  else
    m_Surfaces[uiIndex].Invalidate();
}

void ezTerrainVolumeComponent::Surfaces_Insert(ezUInt32 uiIndex, ezString sValue)
{
  ezSurfaceResourceHandle hSurface;

  if (!sValue.IsEmpty())
    hSurface = ezResourceManager::LoadResource<ezSurfaceResource>(sValue);

  m_Surfaces.InsertAt(uiIndex, hSurface);
}

void ezTerrainVolumeComponent::Surfaces_Remove(ezUInt32 uiIndex)
{
  m_Surfaces.RemoveAtAndCopy(uiIndex);
}

void ezTerrainVolumeComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
  s << m_hMaterial;
  s << m_Resolution.GetValue();
  s << m_fSize;
  s << m_uiBaseMaterialIndex;
  s << m_fFillHeight;
  s << m_bEnableCollider;
  s << m_uiCleanupIterations;
  m_Tags.Save(s);
  s << m_uiStableId;

  const ezUInt32 uiNumSurfaces = m_Surfaces.GetCount();
  s << uiNumSurfaces;
  for (const auto& hSurface : m_Surfaces)
    s << hSurface;
}

void ezTerrainVolumeComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();
  s >> m_hMaterial;
  ezUInt16 uiRes = 0;
  s >> uiRes;
  m_Resolution = static_cast<ezTerrainResolution::Enum>(uiRes);
  s >> m_fSize;
  s >> m_uiBaseMaterialIndex;
  s >> m_fFillHeight;
  s >> m_bEnableCollider;
  s >> m_uiCleanupIterations;
  m_Tags.Load(s, ezTagRegistry::GetGlobalRegistry());
  s >> m_uiStableId;

  ezUInt32 uiNumSurfaces = 0;
  s >> uiNumSurfaces;
  m_Surfaces.SetCount(uiNumSurfaces);
  for (auto& hSurface : m_Surfaces)
    s >> hSurface;
}

void ezTerrainVolumeComponent::OnActivated()
{
  SUPER::OnActivated();

  GetOwner()->EnableStaticTransformChangesNotifications();

  auto* pSystem = GetWorld()->GetOrCreateModule<ezTerrainSystem>();
  const float fVoxelSize = m_fSize / static_cast<float>(m_Resolution.GetValue());
  m_uiVoxelIndex = pSystem->CreateVoxelTerrain(m_Resolution.GetValue(), fVoxelSize);
  auto& vol = pSystem->ModifyVoxelTerrain(m_uiVoxelIndex);
  vol.m_GlobalTransform = GetOwner()->GetGlobalTransform();
  vol.m_bInitialSolid = (m_fFillHeight >= 0.0f);
  vol.m_fFillHeight = m_fFillHeight;
  vol.m_uiCleanupIterations = m_uiCleanupIterations;
  vol.m_Tags = m_Tags;

  TriggerLocalBoundsUpdate();
}

void ezTerrainVolumeComponent::OnDeactivated()
{
  if (auto* pSystem = GetWorld()->GetModule<ezTerrainSystem>())
  {
    pSystem->RemoveVoxelTerrain(m_uiVoxelIndex);
  }

  if (auto* pRDM = GetWorld()->GetModule<ezRenderDataManager>())
  {
    pRDM->DeleteInstanceData(m_InstanceDataOffset);
  }

  m_uiVoxelIndex = ezInvalidIndex;

  SUPER::OnDeactivated();
}

void ezTerrainVolumeComponent::OnObjectCreated(const ezAbstractObjectNode& node)
{
  m_uiStableId = ezHashingUtils::xxHash64(&node.GetGuid(), sizeof(ezUuid));
}

ezResult ezTerrainVolumeComponent::GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg)
{
  ref_bounds = ezBoundingBoxSphere::MakeFromBox(ezBoundingBox::MakeFromMinMax(ezVec3::MakeZero(), ezVec3(m_fSize)));
  return EZ_SUCCESS;
}

void ezTerrainVolumeComponent::OnMsgTransformChanged(ezMsgTransformChanged& msg)
{
  if (m_uiVoxelIndex != ezInvalidIndex)
  {
    if (auto* pSystem = GetWorld()->GetModule<ezTerrainSystem>())
    {
      auto& vol = pSystem->ModifyVoxelTerrain(m_uiVoxelIndex);
      vol.m_GlobalTransform = msg.m_NewGlobalTransform;
    }
  }
}

void ezTerrainVolumeComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (!m_hMaterial.IsValid() || m_uiVoxelIndex == ezInvalidIndex)
    return;

  const auto* pSystem = GetWorld()->GetModule<ezTerrainSystem>();
  if (pSystem == nullptr)
    return;

  const ezGALBufferHandle hVerts = pSystem->GetVoxelVolumeGpuMeshVertexBuffer(m_uiVoxelIndex);
  const ezGALBufferHandle hIdxs = pSystem->GetVoxelVolumeGpuMeshIndexBuffer(m_uiVoxelIndex);
  if (hVerts.IsInvalidated() || hIdxs.IsInvalidated())
    return;

  const bool bDynamic = GetOwner()->IsDynamic();
  auto hInstanceDataBuffer = msg.m_pRenderDataManager->GetOrCreateInstanceDataAndFill(
    *this, bDynamic, GetOwner()->GetGlobalTransform(), m_InstanceDataOffset,
    GetUniqueIdForRendering(), ezColor::White, ezVec4(0, 1, 0, 1));

  ezTerrainVoxelRenderData* pRenderData = msg.m_pRenderDataManager->CreateRenderDataForThisFrame<ezTerrainVoxelRenderData>(GetOwner());
  pRenderData->m_uiNumInstances = 1;
  pRenderData->m_DataOffsets.m_uiInstance = m_InstanceDataOffset.m_uiOffset;
  pRenderData->m_hInstanceDataBuffer = hInstanceDataBuffer;
  pRenderData->m_hMaterial = m_hMaterial;
  pRenderData->m_hGpuMeshVertices = hVerts;
  pRenderData->m_hGpuMeshIndices = hIdxs;
  pRenderData->m_hGpuMeshDrawArgs = pSystem->GetVoxelVolumeGpuMeshDrawArgsBuffer(m_uiVoxelIndex);
  pRenderData->m_uiBaseMaterialIndex = m_uiBaseMaterialIndex;
  pRenderData->m_uiSortingKey = 0;

  msg.AddRenderData(pRenderData, ezDefaultRenderDataCategories::LitOpaque, ezRenderData::Caching::IfStatic);

  // The vertex shader reads the voxel mesh buffers as SRVs and emulates indexed drawing, so declare
  // them as render-graph dependencies for the categories that render this volume.
  const ezGALBufferHandle hDrawArgs = pSystem->GetVoxelVolumeGpuMeshDrawArgsBuffer(m_uiVoxelIndex);
  if (!hVerts.IsInvalidated())
    msg.AddDependency(hVerts, ezDefaultRenderDataCategories::LitOpaque, ezGALResourceState::ShaderResource, ezGALShaderStageFlags::VertexShader);
  if (!hIdxs.IsInvalidated())
    msg.AddDependency(hIdxs, ezDefaultRenderDataCategories::LitOpaque, ezGALResourceState::ShaderResource, ezGALShaderStageFlags::VertexShader);
  if (!hDrawArgs.IsInvalidated())
    msg.AddDependency(hDrawArgs, ezDefaultRenderDataCategories::LitOpaque, ezGALResourceState::DrawIndirect);
}

ezUInt64 ezTerrainVolumeComponent::ComputeColliderContentHash(ezUInt64 uiBrushOverlapHash) const
{
  const ezUInt8 uiVersion = 1;

  ezHashStreamWriter64 hashWriter;
  hashWriter << uiVersion;
  hashWriter << uiBrushOverlapHash;
  hashWriter << m_Resolution;
  hashWriter << m_fSize;
  hashWriter << m_fFillHeight;

  for (ezUInt32 s = 0; s < m_Surfaces.GetCount(); ++s)
    hashWriter << m_Surfaces[s];

  return hashWriter.GetHashValue();
}
