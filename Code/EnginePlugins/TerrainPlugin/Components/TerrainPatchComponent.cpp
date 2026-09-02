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

#include <Foundation/Configuration/CVar.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <RendererCore/Pipeline/View.h>

ezCVarFloat cvar_TerrainLodQuality("Terrain.LodQuality", 1.0f, ezCVarFlags::Default, "Global multiplier for every terrain patch's LodCellPixelSize. > 1 keeps more detail (patches switch LOD later), < 1 coarsens sooner.");

/// View height that LodCellPixelSize is measured against. Fixed rather than the actual viewport, so
/// that a patch picks the same LOD (and therefore the same triangle count) on any display resolution.
static constexpr float g_fLodReferenceViewHeight = 1080.0f;

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezTerrainPatchComponent, 3, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Size", GetSize, SetSize)->AddAttributes(new ezClampValueAttribute(16.0f, 256.0), new ezDefaultValueAttribute(128.0f)),
    EZ_ENUM_ACCESSOR_PROPERTY("Resolution", ezTerrainResolution, GetResolution, SetResolution),
    EZ_RESOURCE_ACCESSOR_PROPERTY("Material", GetMaterial, SetMaterial)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Material", "Terrain-Heightfield"), new ezRequiredAttribute()),
    EZ_ACCESSOR_PROPERTY("BaseMaterialIndex", GetBaseMaterialIndex, SetBaseMaterialIndex)->AddAttributes(new ezClampValueAttribute(0, 31)),
    EZ_RESOURCE_ACCESSOR_PROPERTY("HeightImage", GetHeightImage, SetHeightImage)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Data_2D")),
    EZ_ACCESSOR_PROPERTY("HeightImageOffset", GetHeightImageOffset, SetHeightImageOffset),
    EZ_ACCESSOR_PROPERTY("HeightImageSize", GetHeightImageSize, SetHeightImageSize)->AddAttributes(new ezDefaultValueAttribute(ezVec2(1.0f))),
    EZ_ACCESSOR_PROPERTY("HeightImageScale", GetHeightImageScale, SetHeightImageScale)->AddAttributes(new ezDefaultValueAttribute(32.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ENUM_ACCESSOR_PROPERTY("Collider", ezTerrainPatchColliderMode, GetCollider, SetCollider),
    EZ_ACCESSOR_PROPERTY("LodCellPixelSize", GetLodCellPixelSize, SetLodCellPixelSize)->AddAttributes(new ezDefaultValueAttribute(16.0f), new ezClampValueAttribute(0.0f, ezVariant()), new ezMinValueTextAttribute("LOD Disabled")),
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

constexpr float g_fSkirtDepth = 2.0f;

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

  // Version 3
  s << m_fLodCellPixelSize;
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

  if (uiVersion == 2)
  {
    float fUnusedLodDistanceScale = 0.0f;
    s >> fUnusedLodDistanceScale;
  }
  else if (uiVersion >= 3)
  {
    s >> m_fLodCellPixelSize;
  }
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
  ezVec3 vMin = ezVec3::MakeZero();
  ezVec3 vMax(m_fSize, m_fSize, m_fSize);

  // The skirt extends the 4-vertex border ring outward and pulls those vertices down by SkirtDepth.
  // LodCellPixelSize == 0 ("LOD Disabled") turns the skirt off, so the bounds stay tight in that case.
  if (m_fLodCellPixelSize > 0.0f)
  {
    const float fSkirtWorld = 4.0f * (m_fSize / static_cast<float>(m_Resolution.GetValue()));
    vMin.x -= fSkirtWorld;
    vMin.y -= fSkirtWorld;
    vMax.x += fSkirtWorld;
    vMax.y += fSkirtWorld;
    vMin.z -= g_fSkirtDepth;
  }

  ref_bounds = ezBoundingBoxSphere::MakeFromBox(ezBoundingBox::MakeFromMinMax(vMin, vMax));
  return EZ_SUCCESS;
}

static float CalculateGridCellScreenCoverage(const ezVec3& vWorldPos, float fGridSpacing, const ezCamera& camera)
{
  const ezBoundingSphere sphere = ezBoundingSphere::MakeFromCenterAndRadius(vWorldPos, fGridSpacing * 0.5f);

  if (camera.IsPerspective())
  {
    return ezGraphicsUtils::CalculateSphereScreenCoverage(sphere, camera.GetCenterPosition(), camera.GetFovY(1.0f));
  }
  else
  {
    return ezGraphicsUtils::CalculateSphereScreenCoverage(sphere.m_fRadius, camera.GetDimensionY(1.0f));
  }
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
  const float fGridSpacing = m_fSize / static_cast<float>(m_Resolution.GetValue());
  pRenderData->m_uiCellsPerSide = (ezUInt32)m_Resolution.GetValue();
  pRenderData->m_fGridSpacing = fGridSpacing;
  pRenderData->m_uiDefaultMaterialIndex = m_uiBaseMaterialIndex;

  // LodCellPixelSize == 0 is the "LOD Disabled" special value: always render at full resolution
  // and skip the skirt (there is no coarser neighbor LOD it would need to hide seams against).
  const bool bLodEnabled = m_fLodCellPixelSize > 0.0f;

  ezUInt8 uiLod = 0;
  float fLodFade = 0.0f;

  if (bLodEnabled && msg.m_pView != nullptr)
  {
    // Measure the cell size at the point of the patch closest to the camera, not at its center.
    const ezCamera* pLodCamera = msg.m_pView->GetLodCamera();
    const ezVec3 vCamPos = pLodCamera->GetCenterPosition();

    // Clamp against the patch's own XY footprint in local space. The global bounds are not used here:
    // they are padded by the skirt and span the full height range, which would pull the sample point
    // away from the actual surface.
    const ezTransform globalTransform = GetOwner()->GetGlobalTransform();
    const ezVec3 vLocalCamPos = globalTransform.GetInverse() * vCamPos;
    const ezVec3 vLocalNearest(
      ezMath::Clamp(vLocalCamPos.x, 0.0f, m_fSize),
      ezMath::Clamp(vLocalCamPos.y, 0.0f, m_fSize),
      0.0f);

    const ezVec3 vLodPos = globalTransform * vLocalNearest;
    const float fCoverage0 = CalculateGridCellScreenCoverage(vLodPos, fGridSpacing, *pLodCamera);
    // Coverage is a fraction of view height, so scaling by the reference height turns it into the
    // on-screen height of one grid cell in pixels. Each LOD doubles the cell size, hence log2.
    const float fCellPixels = fCoverage0 * g_fLodReferenceViewHeight;
    const float fTargetPixels = ezMath::Max(m_fLodCellPixelSize * cvar_TerrainLodQuality, 0.00001f);

    const float fContinuousLod = ezMath::Clamp(ezMath::Log2(fTargetPixels / ezMath::Max(fCellPixels, 0.00001f)), 0.0f, 2.0f);

    uiLod = (ezUInt8)ezMath::Trunc(fContinuousLod);
    const float fFrac = fContinuousLod - uiLod;

    const float kFadeBand = 0.3f;
    fLodFade = ezMath::Saturate((fFrac - (1.0f - kFadeBand)) / kFadeBand);
  }

  pRenderData->m_uiLod = uiLod;
  pRenderData->m_fLodFade = fLodFade;
  pRenderData->m_bRenderSkirt = bLodEnabled;
  pRenderData->m_fSkirtDepth = g_fSkirtDepth;
  pRenderData->m_uiSortingKey = 0;
  pRenderData->m_uiNumInstances = 1;
  pRenderData->m_DataOffsets.m_uiInstance = m_InstanceDataOffset.m_uiOffset;
  pRenderData->m_hInstanceDataBuffer = hInstanceDataBuffer;

  // LOD level and fade are recomputed from the current camera distance every frame, so the render
  // data must not be cached across frames (caching would freeze it at whatever LOD was extracted once).
  msg.AddRenderData(pRenderData, ezDefaultRenderDataCategories::LitOpaque, ezRenderData::Caching::Never);

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

void ezTerrainPatchComponent::SetLodCellPixelSize(float fPixels)
{
  fPixels = ezMath::Max(fPixels, 0.0f);
  if (m_fLodCellPixelSize != fPixels)
  {
    const bool bSkirtChanged = (m_fLodCellPixelSize > 0.0f) != (fPixels > 0.0f);

    m_fLodCellPixelSize = fPixels;
    InvalidateCachedRenderData();

    if (bSkirtChanged)
      TriggerLocalBoundsUpdate();
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
