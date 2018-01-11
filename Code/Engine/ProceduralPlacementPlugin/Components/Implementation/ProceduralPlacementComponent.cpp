#include <PCH.h>
#include <ProceduralPlacementPlugin/Components/ProceduralPlacementComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Pipeline/View.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <GameEngine/Components/PrefabReferenceComponent.h>
#include <GameEngine/Interfaces/PhysicsWorldModule.h>
#include <Foundation/Configuration/CVar.h>

namespace
{
  enum
  {
    MAX_TILE_INDEX = (1 << 20) - 1,
    TILE_INDEX_MASK = (1 << 21) - 1
  };

  EZ_ALWAYS_INLINE ezUInt64 GetTileKey(ezInt32 x, ezInt32 y, ezInt32 z)
  {
    ezUInt64 sx = (x + MAX_TILE_INDEX) & TILE_INDEX_MASK;
    ezUInt64 sy = (y + MAX_TILE_INDEX) & TILE_INDEX_MASK;
    ezUInt64 sz = (z + MAX_TILE_INDEX) & TILE_INDEX_MASK;

    return (sx << 42) | (sy << 21) | sz;
  }
}

ezCVarFloat CVarCullDistanceScale("pp_CullDistanceScale", 1.0f, ezCVarFlags::Default, "Global scale to control cull distance for all layers");
ezCVarInt CVarMaxProcessingTiles("pp_MaxProcessingTiles", 10, ezCVarFlags::Default, "Maximum number of tiles in process");
ezCVarBool CVarVisTiles("pp_VisTiles", false, ezCVarFlags::Default, "Enables debug visualization of procedural placement tiles");

ezProceduralPlacementComponentManager::ezProceduralPlacementComponentManager(ezWorld* pWorld)
  : ezComponentManager<ezProceduralPlacementComponent, ezBlockStorageType::Compact>(pWorld)
{

}

ezProceduralPlacementComponentManager::~ezProceduralPlacementComponentManager()
{

}

void ezProceduralPlacementComponentManager::Initialize()
{
  {
    auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezProceduralPlacementComponentManager::Update, this);
    desc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::Async;

    this->RegisterUpdateFunction(desc);
  }

  {
    auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezProceduralPlacementComponentManager::PlaceObjects, this);
    desc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::PostAsync;

    this->RegisterUpdateFunction(desc);
  }
}

void ezProceduralPlacementComponentManager::Deinitialize()
{

}

void ezProceduralPlacementComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
  // TODO: split this function into tasks

  // Find new active tiles
  {
    ezHybridArray<ezSimdTransform, 8, ezAlignedAllocatorWrapper> localBoundingBoxes;

    for (auto& visibleResource : m_VisibleResources)
    {
      auto& hResource = visibleResource.m_hResource;
      ezUInt32 uiResourceIdHash = hResource.GetResourceIDHash();

      ActiveResource* pActiveResource = nullptr;
      EZ_VERIFY(m_ActiveResources.TryGetValue(uiResourceIdHash, pActiveResource), "Implementation error");

      auto& activeLayers = pActiveResource->m_Layers;

      for (ezUInt32 uiLayerIndex = 0; uiLayerIndex < activeLayers.GetCount(); ++uiLayerIndex)
      {
        auto& activeLayer = activeLayers[uiLayerIndex];

        const float fTileSize = activeLayer.m_pLayer->GetTileSize();
        const float fCullDistance = activeLayer.m_pLayer->m_fCullDistance * CVarCullDistanceScale;
        ezSimdVec4f fHalfTileSize = ezSimdVec4f(fTileSize * 0.5f);

        ezVec3 cameraPos = visibleResource.m_vCameraPosition / fTileSize;
        float fPosX = ezMath::Round(cameraPos.x);
        float fPosY = ezMath::Round(cameraPos.y);
        ezInt32 iPosX = static_cast<ezInt32>(fPosX);
        ezInt32 iPosY = static_cast<ezInt32>(fPosY);
        float fRadius = ezMath::Ceil(fCullDistance / fTileSize);
        ezInt32 iRadius = static_cast<ezInt32>(fRadius);
        ezInt32 iRadiusSqr = iRadius * iRadius;

        float fY = (fPosY - fRadius + 0.5f) * fTileSize;
        ezInt32 iY = -iRadius;

        while (iY <= iRadius)
        {
          float fX = (fPosX - fRadius + 0.5f) * fTileSize;
          ezInt32 iX = -iRadius;

          while (iX <= iRadius)
          {
            if (iX*iX + iY*iY <= iRadiusSqr)
            {
              ezSimdVec4f testPos = ezSimdVec4f(fX, fY, 0.0f);
              ezSimdFloat minZ = 10000.0f;
              ezSimdFloat maxZ = -10000.0f;

              localBoundingBoxes.Clear();

              for (auto& bounds : pActiveResource->m_Bounds)
              {
                ezSimdBBox extendedBox = bounds.m_GlobalBoundingBox;
                extendedBox.Grow(fHalfTileSize);

                if (((testPos >= extendedBox.m_Min) && (testPos <= extendedBox.m_Max)).AllSet<2>())
                {
                  minZ = minZ.Min(bounds.m_GlobalBoundingBox.m_Min.z());
                  maxZ = maxZ.Max(bounds.m_GlobalBoundingBox.m_Max.z());

                  localBoundingBoxes.PushBack(bounds.m_LocalBoundingBox);
                }
              }

              if (!localBoundingBoxes.IsEmpty())
              {
                ezUInt64 uiTileKey = GetTileKey(iPosX + iX, iPosY + iY, 0);
                if (!activeLayer.m_TileIndices.Contains(uiTileKey))
                {
                  activeLayer.m_TileIndices.Insert(uiTileKey, ezInvalidIndex);

                  auto& newTile = m_NewTiles.ExpandAndGetRef();
                  newTile.m_uiResourceIdHash = uiResourceIdHash;
                  newTile.m_uiLayerIndex = uiLayerIndex;
                  newTile.m_iPosX = iPosX + iX;
                  newTile.m_iPosY = iPosY + iY;
                  newTile.m_fMinZ = minZ;
                  newTile.m_fMaxZ = maxZ;
                  newTile.m_LocalBoundingBoxes = localBoundingBoxes;
                }
              }
            }

            ++iX;
            fX += fTileSize;
          }

          ++iY;
          fY += fTileSize;
        }
      }
    }

    ClearVisibleResources();
  }

  // Allocate new tiles
  {
    while (!m_NewTiles.IsEmpty() && m_ProcessingTiles.GetCount() < (ezUInt32)CVarMaxProcessingTiles)
    {
      auto& newTile = m_NewTiles.PeekBack();

      ezUInt32 uiNewTileIndex = ezInvalidIndex;
      if (!m_FreeTiles.IsEmpty())
      {
        uiNewTileIndex = m_FreeTiles.PeekBack();
        m_FreeTiles.PopBack();
      }
      else
      {
        uiNewTileIndex = m_ActiveTiles.GetCount();
        m_ActiveTiles.ExpandAndGetRef();
      }

      const Layer* pLayer = m_ActiveResources[newTile.m_uiResourceIdHash].m_Layers[newTile.m_uiLayerIndex].m_pLayer;
      m_ActiveTiles[uiNewTileIndex].Initialize(newTile, pLayer);

      m_ProcessingTiles.PushBack(uiNewTileIndex);
      m_NewTiles.PopBack();
    }
  }

  // Update processing tiles
  if (ezPhysicsWorldModuleInterface* pPhysicsModule = GetWorld()->GetModuleOfBaseType<ezPhysicsWorldModuleInterface>())
  {
    for (auto& uiTileIndex : m_ProcessingTiles)
    {
      m_ActiveTiles[uiTileIndex].Update(pPhysicsModule);
    }
  }

  // Debug draw tiles
  if (CVarVisTiles)
  {
    for (auto& activeTile : m_ActiveTiles)
    {
      if (!activeTile.IsValid())
        continue;

      ezBoundingBox bbox = activeTile.GetBoundingBox();
      ezColor color = activeTile.GetDebugColor();

      ezDebugRenderer::DrawLineBox(GetWorld(), bbox, color);
    }
  }
}

void ezProceduralPlacementComponentManager::PlaceObjects(const ezWorldModule::UpdateContext& context)
{
  for (ezUInt32 i = 0; i < m_ProcessingTiles.GetCount(); ++i)
  {
    ezUInt32 uiTileIndex = m_ProcessingTiles[i];

    auto& activeTile = m_ActiveTiles[uiTileIndex];

    if (activeTile.IsFinished())
    {
      ezGameObjectDesc desc;
      auto& objectsToPlace = activeTile.GetLayer()->m_ObjectsToPlace;

      auto objectTransforms = activeTile.GetObjectTransforms();
      for (auto& objectTransform : objectTransforms)
      {
        desc.m_LocalPosition = ezSimdConversion::ToVec3(objectTransform.m_Transform.m_Position);
        desc.m_LocalRotation = ezSimdConversion::ToQuat(objectTransform.m_Transform.m_Rotation);
        desc.m_LocalScaling = ezSimdConversion::ToVec3(objectTransform.m_Transform.m_Scale);

        ezGameObject* pObject = nullptr;
        GetWorld()->CreateObject(desc, pObject);

        //pObject->GetTags().Set(tag);

        ezPrefabReferenceComponent* pPrefabReferenceComponent = nullptr;
        ezPrefabReferenceComponent::CreateComponent(pObject, pPrefabReferenceComponent);

        auto& objectToPlace = objectsToPlace[objectTransform.m_uiObjectIndex];
        pPrefabReferenceComponent->SetPrefabFile(objectToPlace);
      }

      if (!objectTransforms.IsEmpty())
      {
        auto& tileDesc = activeTile.GetDesc();

        auto& activeLayer = m_ActiveResources[tileDesc.m_uiResourceIdHash].m_Layers[tileDesc.m_uiLayerIndex];

        ezUInt64 uiTileKey = GetTileKey(tileDesc.m_iPosX, tileDesc.m_iPosY, 0);
        activeLayer.m_TileIndices[uiTileKey] = uiTileIndex;
      }

      m_ProcessingTiles.RemoveAtSwap(i);
      --i;
    }
  }
}

void ezProceduralPlacementComponentManager::AddComponent(ezProceduralPlacementComponent* pComponent)
{
  auto& hResource = pComponent->GetResource();
  ezUInt32 uiResourceIdHash = hResource.GetResourceIDHash();

  ActiveResource* pActiveResource = nullptr;
  if (!m_ActiveResources.TryGetValue(uiResourceIdHash, pActiveResource))
  {
    // Update data from resource
    pActiveResource = &m_ActiveResources[uiResourceIdHash];

    ezResourceLock<ezProceduralPlacementResource> pResource(hResource, ezResourceAcquireMode::NoFallback);
    auto layers = pResource->GetLayers();

    pActiveResource->m_Layers.Reserve(layers.GetCount());
    for (auto& layer : layers)
    {
      auto& activeLayer = pActiveResource->m_Layers.ExpandAndGetRef();
      activeLayer.m_pLayer = &layer;
    }
  }

  ezSimdTransform localBoundingBox = pComponent->GetOwner()->GetGlobalTransformSimd();
  localBoundingBox.m_Scale = localBoundingBox.m_Scale.CompMul(ezSimdConversion::ToVec3(pComponent->GetExtents() * 0.5f));
  localBoundingBox.Invert();

  auto& bounds = pActiveResource->m_Bounds.ExpandAndGetRef();
  bounds.m_GlobalBoundingBox = pComponent->GetOwner()->GetGlobalBoundsSimd().GetBox();
  bounds.m_LocalBoundingBox = localBoundingBox;
  bounds.m_hComponent = pComponent->GetHandle();
}

void ezProceduralPlacementComponentManager::RemoveComponent(ezProceduralPlacementComponent* pComponent)
{
  auto& hResource = pComponent->GetResource();
  ezUInt32 uiResourceIdHash = hResource.GetResourceIDHash();

  ActiveResource* pActiveResource = nullptr;
  if (m_ActiveResources.TryGetValue(uiResourceIdHash, pActiveResource))
  {
    ezComponentHandle hComponent = pComponent->GetHandle();

    for (ezUInt32 i = 0; i < pActiveResource->m_Bounds.GetCount(); ++i)
    {
      auto& bounds = pActiveResource->m_Bounds[i];
      if (bounds.m_hComponent == hComponent)
      {
        pActiveResource->m_Bounds.RemoveAtSwap(i);
        break;
      }
    }
  }

  //TODO: invalidate all corresponding tiles
}

void ezProceduralPlacementComponentManager::AddVisibleResource(const ezProceduralPlacementResourceHandle& hResource, const ezVec3& cameraPosition,
  const ezVec3& cameraDirection) const
{
  EZ_LOCK(m_VisibleResourcesMutex);

  for (auto& visibleResource : m_VisibleResources)
  {
    if (visibleResource.m_hResource == hResource &&
      visibleResource.m_vCameraPosition == cameraPosition &&
      visibleResource.m_vCameraDirection == cameraDirection)
    {
      return;
    }
  }

  auto& visibleResource = m_VisibleResources.ExpandAndGetRef();
  visibleResource.m_hResource = hResource;
  visibleResource.m_vCameraPosition = cameraPosition;
  visibleResource.m_vCameraDirection = cameraDirection;
}

void ezProceduralPlacementComponentManager::ClearVisibleResources()
{
  m_VisibleResources.Clear();
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_COMPONENT_TYPE(ezProceduralPlacementComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Resource", GetResourceFile, SetResourceFile)->AddAttributes(new ezAssetBrowserAttribute("ProceduralPlacement")),
    EZ_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new ezDefaultValueAttribute(ezVec3(10.0f)), new ezClampValueAttribute(ezVec3(0), ezVariant())),
  }
  EZ_END_PROPERTIES
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezUpdateLocalBoundsMessage, OnUpdateLocalBounds),
    EZ_MESSAGE_HANDLER(ezExtractRenderDataMessage, OnExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("ProceduralPlacement"),
    new ezBoxManipulatorAttribute("Extents"),
    new ezBoxVisualizerAttribute("Extents"),
  }
  EZ_END_ATTRIBUTES
}
EZ_END_COMPONENT_TYPE

ezProceduralPlacementComponent::ezProceduralPlacementComponent()
{
  m_vExtents.Set(10.0f);

  // temp
  m_hResource = ezResourceManager::GetExistingResource<ezProceduralPlacementResource>("TestPP");
  if (!m_hResource.IsValid())
  {
    m_hResource = ezResourceManager::CreateResource<ezProceduralPlacementResource>("TestPP", ezProceduralPlacementResourceDescriptor());
  }
}

ezProceduralPlacementComponent::~ezProceduralPlacementComponent()
{

}

void ezProceduralPlacementComponent::OnActivated()
{
  GetOwner()->UpdateLocalBounds();
  GetManager()->AddComponent(this);
}

void ezProceduralPlacementComponent::OnDeactivated()
{
  GetOwner()->UpdateLocalBounds();
  GetManager()->RemoveComponent(this);
}

void ezProceduralPlacementComponent::SetResourceFile(const char* szFile)
{
  ezProceduralPlacementResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezProceduralPlacementResource>(szFile, ezResourcePriority::High, ezProceduralPlacementResourceHandle());
    ezResourceManager::PreloadResource(hResource, ezTime::Seconds(0.0));
  }

  SetResource(hResource);
}

const char* ezProceduralPlacementComponent::GetResourceFile() const
{
  if (!m_hResource.IsValid())
    return "";

  return m_hResource.GetResourceID();
}

void ezProceduralPlacementComponent::SetResource(const ezProceduralPlacementResourceHandle& hResource)
{
  if (IsActiveAndInitialized())
  {
    GetManager()->RemoveComponent(this);
  }

  //m_hResource = hResource;

  if (IsActiveAndInitialized())
  {
    GetManager()->AddComponent(this);
  }
}

void ezProceduralPlacementComponent::SetExtents(const ezVec3& value)
{
  m_vExtents = value.CompMax(ezVec3::ZeroVector());

  if (IsActiveAndInitialized())
  {
    GetManager()->RemoveComponent(this);

    GetOwner()->UpdateLocalBounds();

    GetManager()->AddComponent(this);
  }
}

void ezProceduralPlacementComponent::OnUpdateLocalBounds(ezUpdateLocalBoundsMessage& msg)
{
  msg.AddBounds(ezBoundingBox(-m_vExtents * 0.5f, m_vExtents * 0.5f));
}

void ezProceduralPlacementComponent::OnExtractRenderData(ezExtractRenderDataMessage& msg) const
{
  // Don't extract render data for selection or in shadow views.
  if (msg.m_OverrideCategory != ezInvalidIndex)
    return;

  if (msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::MainView ||
    msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::EditorView)
  {
    const ezCamera* pCamera = msg.m_pView->GetCullingCamera();

    ezVec3 cameraPosition = pCamera->GetCenterPosition();
    ezVec3 cameraDirection = pCamera->GetCenterDirForwards();

    GetManager()->AddVisibleResource(m_hResource, cameraPosition, cameraDirection);
  }
}

void ezProceduralPlacementComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  ezStreamWriter& s = stream.GetStream();
}

void ezProceduralPlacementComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  //const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

}
