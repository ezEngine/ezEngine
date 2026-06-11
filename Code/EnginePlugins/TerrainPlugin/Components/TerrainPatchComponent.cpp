#include <TerrainPlugin/TerrainPluginPCH.h>

#include <Core/Messages/TransformChangedMessage.h>
#include <Core/Physics/SurfaceResource.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Types/TagRegistry.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <TerrainPlugin/Components/TerrainPatchComponent.h>
#include <TerrainPlugin/Rendering/TerrainRenderData.h>
#include <TerrainPlugin/TerrainSystem.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezTerrainPatchComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Size", GetSize, SetSize)->AddAttributes(new ezClampValueAttribute(16.0f, 256.0), new ezDefaultValueAttribute(128.0f)),
    EZ_ENUM_ACCESSOR_PROPERTY("Resolution", ezTerrainResolution, GetResolution, SetResolution),
    EZ_RESOURCE_ACCESSOR_PROPERTY("Material", GetMaterial, SetMaterial)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Material", "Terrain-Heightfield")),
    EZ_ACCESSOR_PROPERTY("BaseMaterialIndex", GetBaseMaterialIndex, SetBaseMaterialIndex)->AddAttributes(new ezClampValueAttribute(0, 31)),
    EZ_RESOURCE_ACCESSOR_PROPERTY("HeightImage", GetHeightImage, SetHeightImage)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Data_2D")),
    EZ_ACCESSOR_PROPERTY("HeightImageOffset", GetHeightImageOffset, SetHeightImageOffset),
    EZ_ACCESSOR_PROPERTY("HeightImageSize", GetHeightImageSize, SetHeightImageSize)->AddAttributes(new ezDefaultValueAttribute(ezVec2(1.0f))),
    EZ_ACCESSOR_PROPERTY("HeightImageScale", GetHeightImageScale, SetHeightImageScale)->AddAttributes(new ezDefaultValueAttribute(32.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ENUM_ACCESSOR_PROPERTY("Collider", ezTerrainPatchColliderMode, GetCollider, SetCollider),
    EZ_ARRAY_ACCESSOR_PROPERTY("Surfaces", Surfaces_GetCount, Surfaces_GetValue, Surfaces_SetValue, Surfaces_Insert, Surfaces_Remove)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Surface", ezDependencyFlags::Package)),
    EZ_SET_ACCESSOR_PROPERTY("TerrainTags", GetTags, Reflection_SetTag, Reflection_RemoveTag)->AddAttributes(new ezTagSetWidgetAttribute("Terrain")),
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

ezTerrainPatchComponent::ezTerrainPatchComponent() = default;
ezTerrainPatchComponent::~ezTerrainPatchComponent() = default;

void ezTerrainPatchComponent::SetResolution(ezEnum<ezTerrainResolution> resolution)
{
  if (m_Resolution != resolution)
  {
    if (m_uiHeightfieldIndex != ezInvalidIndex)
    {
      if (auto* pSystem = GetWorld()->GetModule<ezTerrainSystem>())
      {
        pSystem->RemoveHeightfieldTerrain(m_uiHeightfieldIndex);

        m_uiHeightfieldIndex = pSystem->CreateHeightfieldTerrain((ezUInt32)resolution.GetValue());

        auto& data = pSystem->ModifyHeightfieldTerrain(m_uiHeightfieldIndex);
        data.m_uiDefaultMaterialIndex = m_uiBaseMaterialIndex;
        data.m_fGridSpacing = m_fSize / static_cast<float>(resolution.GetValue());
        data.m_GlobalTransform = GetOwner()->GetGlobalTransform();
      }
    }

    m_Resolution = resolution;
    TriggerLocalBoundsUpdate();
    InvalidateCachedRenderData();
  }
}

void ezTerrainPatchComponent::SetSize(float fSize)
{
  if (m_fSize != fSize)
  {
    m_fSize = fSize;

    if (m_uiHeightfieldIndex != ezInvalidIndex)
    {
      if (auto* pSystem = GetWorld()->GetModule<ezTerrainSystem>())
      {
        auto& data = pSystem->ModifyHeightfieldTerrain(m_uiHeightfieldIndex);
        data.m_fGridSpacing = m_fSize / static_cast<float>(m_Resolution.GetValue());
      }
    }

    TriggerLocalBoundsUpdate();
    InvalidateCachedRenderData();
  }
}

void ezTerrainPatchComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  s << m_hMaterial;
  s << m_Resolution;
  s << m_fSize;
  s << m_ColliderMode;
  s << m_uiBaseMaterialIndex;

  const ezUInt32 uiNumSurfaces = m_Surfaces.GetCount();
  s << uiNumSurfaces;
  for (const auto& hSurface : m_Surfaces)
    s << hSurface;
  m_Tags.Save(s);

  s << m_hHeightImage;
  s << m_vImageOffset;
  s << m_vImageSize;
  s << m_fHeightScale;
  s << m_uiStableId;
}

void ezTerrainPatchComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();
  s >> m_hMaterial;
  s >> m_Resolution;
  s >> m_fSize;
  s >> m_ColliderMode;
  s >> m_uiBaseMaterialIndex;

  ezUInt32 uiNumSurfaces = 0;
  s >> uiNumSurfaces;
  m_Surfaces.SetCount(uiNumSurfaces);

  for (auto& hSurface : m_Surfaces)
    s >> hSurface;
  m_Tags.Load(s, ezTagRegistry::GetGlobalRegistry());

  s >> m_hHeightImage;
  s >> m_vImageOffset;
  s >> m_vImageSize;
  s >> m_fHeightScale;
  s >> m_uiStableId;
}

void ezTerrainPatchComponent::SetMaterial(const ezMaterialResourceHandle& hMaterial)
{
  if (m_hMaterial != hMaterial)
  {
    m_hMaterial = hMaterial;
    InvalidateCachedRenderData();
  }
}

void ezTerrainPatchComponent::SetHeightImage(const ezImageDataResourceHandle& hImage)
{
  if (m_hHeightImage != hImage)
  {
    m_hHeightImage = hImage;

    if (m_uiHeightfieldIndex != ezInvalidIndex)
    {
      if (auto* pSystem = GetWorld()->GetOrCreateModule<ezTerrainSystem>())
        pSystem->ModifyHeightfieldTerrain(m_uiHeightfieldIndex).m_hHeightImage = hImage;
    }
  }
}

void ezTerrainPatchComponent::SetHeightImageOffset(ezVec2 vOffset)
{
  if (m_vImageOffset != vOffset)
  {
    m_vImageOffset = vOffset;

    if (m_uiHeightfieldIndex != ezInvalidIndex)
    {
      if (auto* pSystem = GetWorld()->GetOrCreateModule<ezTerrainSystem>())
        pSystem->ModifyHeightfieldTerrain(m_uiHeightfieldIndex).m_vImageOffset = vOffset;
    }
  }
}

void ezTerrainPatchComponent::SetHeightImageSize(ezVec2 vSize)
{
  if (m_vImageSize != vSize)
  {
    m_vImageSize = vSize;

    if (m_uiHeightfieldIndex != ezInvalidIndex)
    {
      if (auto* pSystem = GetWorld()->GetOrCreateModule<ezTerrainSystem>())
        pSystem->ModifyHeightfieldTerrain(m_uiHeightfieldIndex).m_vImageSize = vSize;
    }
  }
}

void ezTerrainPatchComponent::SetHeightImageScale(float fScale)
{
  if (m_fHeightScale != fScale)
  {
    m_fHeightScale = fScale;

    if (m_uiHeightfieldIndex != ezInvalidIndex)
    {
      if (auto* pSystem = GetWorld()->GetOrCreateModule<ezTerrainSystem>())
        pSystem->ModifyHeightfieldTerrain(m_uiHeightfieldIndex).m_fHeightScale = fScale;
    }
  }
}

void ezTerrainPatchComponent::Reflection_SetTag(const char* szTagName)
{
  if (ezStringUtils::IsNullOrEmpty(szTagName))
    return;
  const ezTag& tag = ezTagRegistry::GetGlobalRegistry().RegisterTag(szTagName);
  if (m_Tags.IsSet(tag))
    return;
  m_Tags.Set(tag);
  if (m_uiHeightfieldIndex != ezInvalidIndex)
  {
    if (auto* pSystem = GetWorld()->GetModule<ezTerrainSystem>())
      pSystem->ModifyHeightfieldTerrain(m_uiHeightfieldIndex).m_Tags = m_Tags;
  }
}

void ezTerrainPatchComponent::Reflection_RemoveTag(const char* szTagName)
{
  if (ezStringUtils::IsNullOrEmpty(szTagName))
    return;
  if (const ezTag* pTag = ezTagRegistry::GetGlobalRegistry().GetTagByName(ezTempHashedString(szTagName)))
  {
    if (!m_Tags.IsSet(*pTag))
      return;
    m_Tags.Remove(*pTag);
    if (m_uiHeightfieldIndex != ezInvalidIndex)
    {
      if (auto* pSystem = GetWorld()->GetModule<ezTerrainSystem>())
        pSystem->ModifyHeightfieldTerrain(m_uiHeightfieldIndex).m_Tags = m_Tags;
    }
  }
}

void ezTerrainPatchComponent::OnActivated()
{
  SUPER::OnActivated();

  GetOwner()->EnableStaticTransformChangesNotifications();

  auto* pSystem = GetWorld()->GetOrCreateModule<ezTerrainSystem>();

  m_uiHeightfieldIndex = pSystem->CreateHeightfieldTerrain((ezUInt32)m_Resolution.GetValue());

  auto& data = pSystem->ModifyHeightfieldTerrain(m_uiHeightfieldIndex);
  data.m_uiDefaultMaterialIndex = m_uiBaseMaterialIndex;
  data.m_fGridSpacing = m_fSize / static_cast<float>(m_Resolution.GetValue());
  data.m_GlobalTransform = GetOwner()->GetGlobalTransform();
  data.m_Tags = m_Tags;
  data.m_hHeightImage = m_hHeightImage;
  data.m_vImageOffset = m_vImageOffset;
  data.m_vImageSize = m_vImageSize;
  data.m_fHeightScale = m_fHeightScale;

  TriggerLocalBoundsUpdate();
}

void ezTerrainPatchComponent::OnDeactivated()
{
  if (auto* pSystem = GetWorld()->GetModule<ezTerrainSystem>())
  {
    pSystem->RemoveHeightfieldTerrain(m_uiHeightfieldIndex);
  }

  if (auto* pRDM = GetWorld()->GetModule<ezRenderDataManager>())
  {
    pRDM->DeleteInstanceData(m_InstanceDataOffset);
  }

  SUPER::OnDeactivated();
}

ezResult ezTerrainPatchComponent::GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg)
{
  const ezVec3 vMin = ezVec3::MakeZero();
  const ezVec3 vMax(m_fSize, m_fSize, m_fSize);

  ref_bounds = ezBoundingBoxSphere::MakeFromBox(ezBoundingBox::MakeFromMinMax(vMin, vMax));
  return EZ_SUCCESS;
}

void ezTerrainPatchComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (m_uiHeightfieldIndex == ezInvalidIndex)
    return;

  const auto* pSystem = GetWorld()->GetModule<ezTerrainSystem>();
  if (pSystem == nullptr)
    return;

  const ezGALBufferHandle hHeightBuffer = pSystem->GetHeightfieldHeightBuffer(m_uiHeightfieldIndex);
  if (hHeightBuffer.IsInvalidated())
    return;

  const ezGALBufferHandle hNormalBuffer = pSystem->GetHeightfieldNormalBuffer(m_uiHeightfieldIndex);
  const ezGALBufferHandle hCellMaterialBuffer = pSystem->GetHeightfieldCellMaterialBuffer(m_uiHeightfieldIndex);
  const ezGALBufferHandle hVertexWeightBuffer = pSystem->GetHeightfieldMaterialVertexWeightBuffer(m_uiHeightfieldIndex);

  // Create instance data so the pixel shader can read GameObjectID for editor picking.
  auto hInstanceDataBuffer = msg.m_pRenderDataManager->GetOrCreateInstanceDataAndFill(*this, true, GetOwner()->GetGlobalTransform(), m_InstanceDataOffset, GetUniqueIdForRendering());

  ezTerrainHeightfieldRenderData* pRenderData = msg.m_pRenderDataManager->CreateRenderDataForThisFrame<ezTerrainHeightfieldRenderData>(GetOwner());
  pRenderData->m_hMaterial = m_hMaterial;
  pRenderData->m_hHeightBuffer = hHeightBuffer;
  pRenderData->m_hNormalBuffer = hNormalBuffer;
  pRenderData->m_hCellMaterialBuffer = hCellMaterialBuffer;
  pRenderData->m_hVertexWeightBuffer = hVertexWeightBuffer;
  pRenderData->m_uiCellsPerSide = (ezUInt32)m_Resolution.GetValue();
  pRenderData->m_fGridSpacing = m_fSize / static_cast<float>(m_Resolution.GetValue());
  pRenderData->m_uiDefaultMaterialIndex = m_uiBaseMaterialIndex;
  pRenderData->m_uiSortingKey = 0;
  pRenderData->m_uiNumInstances = 1;
  pRenderData->m_DataOffsets.m_uiInstance = m_InstanceDataOffset.m_uiOffset;
  pRenderData->m_hInstanceDataBuffer = hInstanceDataBuffer;

  msg.AddRenderData(pRenderData, ezDefaultRenderDataCategories::LitOpaque, ezRenderData::Caching::IfStatic);

  // The vertex shader samples these persistent GPU buffers, so declare them as render-graph
  // dependencies for the categories that render this patch.
  if (!hHeightBuffer.IsInvalidated())
    msg.AddDependency(hHeightBuffer, ezDefaultRenderDataCategories::LitOpaque, ezGALResourceState::ShaderResource, ezGALShaderStageFlags::VertexShader);
  if (!hNormalBuffer.IsInvalidated())
    msg.AddDependency(hNormalBuffer, ezDefaultRenderDataCategories::LitOpaque, ezGALResourceState::ShaderResource, ezGALShaderStageFlags::VertexShader);
  if (!hCellMaterialBuffer.IsInvalidated())
    msg.AddDependency(hCellMaterialBuffer, ezDefaultRenderDataCategories::LitOpaque, ezGALResourceState::ShaderResource, ezGALShaderStageFlags::VertexShader);
  if (!hVertexWeightBuffer.IsInvalidated())
    msg.AddDependency(hVertexWeightBuffer, ezDefaultRenderDataCategories::LitOpaque, ezGALResourceState::ShaderResource, ezGALShaderStageFlags::VertexShader);
}

void ezTerrainPatchComponent::OnMsgTransformChanged(ezMsgTransformChanged& msg)
{
  if (m_uiHeightfieldIndex != ezInvalidIndex)
  {
    auto* pSystem = GetWorld()->GetOrCreateModule<ezTerrainSystem>();
    auto& data = pSystem->ModifyHeightfieldTerrain(m_uiHeightfieldIndex);
    data.m_GlobalTransform = msg.m_NewGlobalTransform;
  }
}

void ezTerrainPatchComponent::SetCollider(ezEnum<ezTerrainPatchColliderMode> mode)
{
  m_ColliderMode = mode;
}

void ezTerrainPatchComponent::OnObjectCreated(const ezAbstractObjectNode& node)
{
  m_uiStableId = ezHashingUtils::xxHash64(&node.GetGuid(), sizeof(ezUuid));
}


void ezTerrainPatchComponent::SetBaseMaterialIndex(ezUInt8 uiIndex)
{
  if (m_uiBaseMaterialIndex == uiIndex)
    return;

  m_uiBaseMaterialIndex = uiIndex;

  if (m_uiHeightfieldIndex != ezInvalidIndex)
  {
    auto* pSystem = GetWorld()->GetOrCreateModule<ezTerrainSystem>();
    pSystem->ModifyHeightfieldTerrain(m_uiHeightfieldIndex).m_uiDefaultMaterialIndex = uiIndex;
  }
}

ezUInt32 ezTerrainPatchComponent::Surfaces_GetCount() const
{
  return m_Surfaces.GetCount();
}

ezString ezTerrainPatchComponent::Surfaces_GetValue(ezUInt32 uiIndex) const
{
  if (uiIndex >= m_Surfaces.GetCount() || !m_Surfaces[uiIndex].IsValid())
    return {};
  return m_Surfaces[uiIndex].GetResourceID();
}

ezUInt64 ezTerrainPatchComponent::ComputeColliderContentHash(ezUInt64 uiBrushOverlapHash) const
{
  const ezUInt8 uiVersion = 1;

  ezHashStreamWriter64 hashWriter;
  hashWriter << uiVersion;
  hashWriter << uiBrushOverlapHash;
  hashWriter << m_ColliderMode.GetValue();
  hashWriter << m_fSize;

  hashWriter << m_hHeightImage;
  hashWriter << m_vImageOffset;
  hashWriter << m_vImageSize;
  hashWriter << m_fHeightScale;

  for (ezUInt32 i = 0; i < m_Surfaces.GetCount(); ++i)
    hashWriter << m_Surfaces[i];

  return hashWriter.GetHashValue();
}

void ezTerrainPatchComponent::Surfaces_SetValue(ezUInt32 uiIndex, ezString sValue)
{
  m_Surfaces.EnsureCount(uiIndex + 1);
  if (!sValue.IsEmpty())
    m_Surfaces[uiIndex] = ezResourceManager::LoadResource<ezSurfaceResource>(sValue);
  else
    m_Surfaces[uiIndex].Invalidate();
}

void ezTerrainPatchComponent::Surfaces_Insert(ezUInt32 uiIndex, ezString sValue)
{
  ezSurfaceResourceHandle hSurface;
  if (!sValue.IsEmpty())
    hSurface = ezResourceManager::LoadResource<ezSurfaceResource>(sValue);
  m_Surfaces.InsertAt(uiIndex, hSurface);
}

void ezTerrainPatchComponent::Surfaces_Remove(ezUInt32 uiIndex)
{
  m_Surfaces.RemoveAtAndCopy(uiIndex);
}

//////////////////////////////////////////////////////////////////////////

ezTerrainPatchComponentManager::ezTerrainPatchComponentManager(ezWorld* pWorld)
  : SUPER(pWorld)
{
  ezResourceManager::GetResourceEvents().AddEventHandler(ezMakeDelegate(&ezTerrainPatchComponentManager::ResourceEventHandler, this));
}

ezTerrainPatchComponentManager::~ezTerrainPatchComponentManager()
{
  ezResourceManager::GetResourceEvents().RemoveEventHandler(ezMakeDelegate(&ezTerrainPatchComponentManager::ResourceEventHandler, this));
}

void ezTerrainPatchComponentManager::ResourceEventHandler(const ezResourceEvent& e)
{
  if (e.m_Type == ezResourceEvent::Type::ResourceContentUpdated && e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<ezImageDataResource>())
  {
    ezImageDataResourceHandle hResource((ezImageDataResource*)e.m_pResource);
    for (auto it = GetComponents(); it.IsValid(); it.Next())
    {
      if (it->m_hHeightImage == hResource)
      {
        it->m_bHeightImageDirty = true;
      }
    }
  }
}

void ezTerrainPatchComponentManager::Initialize()
{
  SUPER::Initialize();

  auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezTerrainPatchComponentManager::Update, this);
  desc.m_Phase = ezWorldUpdatePhase::PostAsync;
  desc.m_bOnlyUpdateWhenSimulating = false;
  RegisterUpdateFunction(desc);
}

void ezTerrainPatchComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
  for (auto it = m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    ezTerrainPatchComponent* pComp = it;
    if (pComp->IsActiveAndInitialized())
    {
      if (pComp->m_bHeightImageDirty && pComp->m_uiHeightfieldIndex != ezInvalidIndex)
      {
        if (auto* pSystem = GetWorld()->GetModule<ezTerrainSystem>())
        {
          pSystem->ModifyHeightfieldTerrain(pComp->m_uiHeightfieldIndex); // marks patch dirty for rebake
          pComp->m_bHeightImageDirty = false;
        }
      }
    }
  }
}
