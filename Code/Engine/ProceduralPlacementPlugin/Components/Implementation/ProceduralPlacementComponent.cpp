#include <PCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Profiling/Profiling.h>
#include <GameEngine/Interfaces/PhysicsWorldModule.h>
#include <ProceduralPlacementPlugin/Components/Implementation/ActiveTile.h>
#include <ProceduralPlacementPlugin/Components/ProceduralPlacementComponent.h>
#include <ProceduralPlacementPlugin/Tasks/PlacementTask.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Pipeline/View.h>

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

#define EmptyTileIndex ezInvalidIndex
}

using namespace ezPPInternal;

ezCVarFloat CVarCullDistanceScale("pp_CullDistanceScale", 1.0f, ezCVarFlags::Default,
                                  "Global scale to control cull distance for all layers");
ezCVarInt CVarMaxProcessingTiles("pp_MaxProcessingTiles", 10, ezCVarFlags::Default, "Maximum number of tiles in process");
ezCVarBool CVarVisTiles("pp_VisTiles", false, ezCVarFlags::Default, "Enables debug visualization of procedural placement tiles");

ezProceduralPlacementComponentManager::ezProceduralPlacementComponentManager(ezWorld* pWorld)
    : ezComponentManager<ezProceduralPlacementComponent, ezBlockStorageType::Compact>(pWorld)
{
}

ezProceduralPlacementComponentManager::~ezProceduralPlacementComponentManager() {}

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

  ezResourceManager::s_ResourceEvents.AddEventHandler(ezMakeDelegate(&ezProceduralPlacementComponentManager::OnResourceEvent, this));
}

void ezProceduralPlacementComponentManager::Deinitialize()
{
  ezResourceManager::s_ResourceEvents.RemoveEventHandler(ezMakeDelegate(&ezProceduralPlacementComponentManager::OnResourceEvent, this));

  for (auto& activeTile : m_ActiveTiles)
  {
    activeTile.Deinitialize(*GetWorld());
  }
  m_ActiveTiles.Clear();
}

void ezProceduralPlacementComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
  // Update resource data
  bool bAnyObjectsRemoved = false;

  for (auto& hResource : m_ResourcesToUpdate)
  {
    ezUInt32 uiResourceIdHash = hResource.GetResourceIDHash();
    RemoveTilesForResource(uiResourceIdHash, &bAnyObjectsRemoved);

    ActiveResource& activeResource = m_ActiveResources[uiResourceIdHash];

    ezResourceLock<ezProceduralPlacementResource> pResource(hResource, ezResourceAcquireMode::NoFallback);
    auto layers = pResource->GetLayers();

    activeResource.m_Layers.Clear();
    for (auto& pLayer : layers)
    {
      if (pLayer->IsValid())
      {
        activeResource.m_Layers.ExpandAndGetRef().m_pLayer = pLayer;
      }
    }
  }
  m_ResourcesToUpdate.Clear();

  // If we removed any objects during resource update do nothing else this frame so objects are actually deleted before we place new ones.
  if (bAnyObjectsRemoved)
  {
    return;
  }

  // TODO: split this function into tasks

  // Find new active tiles
  {
    EZ_PROFILE_SCOPE("Find new tiles");

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
        const float fPatternSize = activeLayer.m_pLayer->m_pPattern->m_fSize;
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
            if (iX * iX + iY * iY <= iRadiusSqr)
            {
              ezUInt64 uiTileKey = GetTileKey(iPosX + iX, iPosY + iY, 0);
              if (!activeLayer.m_TileIndices.Contains(uiTileKey))
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
                  activeLayer.m_TileIndices.Insert(uiTileKey, EmptyTileIndex);

                  auto& newTile = m_NewTiles.ExpandAndGetRef();
                  newTile.m_uiResourceIdHash = uiResourceIdHash;
                  newTile.m_uiLayerIndex = uiLayerIndex;
                  newTile.m_iPosX = iPosX + iX;
                  newTile.m_iPosY = iPosY + iY;
                  newTile.m_fMinZ = minZ;
                  newTile.m_fMaxZ = maxZ;
                  newTile.m_fPatternSize = fPatternSize;
                  newTile.m_fDistanceToCamera = -1.0f;
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

    // Sort new tiles
    {
      EZ_PROFILE_SCOPE("Sort new tiles");

      for (auto& newTile : m_NewTiles)
      {
        ezVec2 tilePos = ezVec2((float)newTile.m_iPosX, (float)newTile.m_iPosY) * newTile.m_fPatternSize;

        float fMinDistance = ezMath::BasicType<float>::MaxValue();
        for (auto& visibleResource : m_VisibleResources)
        {
          float fDistance = (tilePos - visibleResource.m_vCameraPosition.GetAsVec2()).GetLengthSquared();
          fMinDistance = ezMath::Min(fMinDistance, fDistance);
        }

        newTile.m_fDistanceToCamera = fMinDistance;
      }

      // Sort by distance, larger distances come first since new tiles are processed in reverse order.
      m_NewTiles.Sort([](auto& tileA, auto& tileB) { return tileA.m_fDistanceToCamera > tileB.m_fDistanceToCamera; });
    }

    ClearVisibleResources();
  }

  // Allocate new tiles and placement tasks
  {
    EZ_PROFILE_SCOPE("Allocate new tiles");

    while (!m_NewTiles.IsEmpty() && GetNumAllocatedPlacementTasks() < (ezUInt32)CVarMaxProcessingTiles)
    {
      const TileDesc& newTile = m_NewTiles.PeekBack();
      auto& pLayer = m_ActiveResources[newTile.m_uiResourceIdHash].m_Layers[newTile.m_uiLayerIndex].m_pLayer;
      ezUInt32 uiNewTileIndex = AllocateTile(newTile, pLayer);

      AllocatePlacementTask(uiNewTileIndex);
      m_NewTiles.PopBack();
    }
  }

  const ezWorld* pWorld = GetWorld();

  // Update processing tiles
  if (GetWorldSimulationEnabled())
  {
    if (const ezPhysicsWorldModuleInterface* pPhysicsModule = pWorld->GetModule<ezPhysicsWorldModuleInterface>())
    {
      EZ_PROFILE_SCOPE("Update processing tiles");

      for (ezUInt32 i = 0; i < m_PlacementTaskInfos.GetCount(); ++i)
      {
        auto& taskInfo = m_PlacementTaskInfos[i];
        if (!taskInfo.IsValid() || taskInfo.IsScheduled())
          continue;

        m_ActiveTiles[taskInfo.m_uiTileIndex].PrepareTask(pPhysicsModule, *taskInfo.m_pTask);

        if (!taskInfo.m_pTask->GetInputPoints().IsEmpty())
        {
          taskInfo.m_taskGroupID = ezTaskSystem::StartSingleTask(taskInfo.m_pTask.Borrow(), ezTaskPriority::LongRunningHighPriority);
        }
        else
        {
          // mark tile & task for re-use
          DeallocateTile(taskInfo.m_uiTileIndex);
          DeallocatePlacementTask(i);
        }
      }
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

      ezDebugRenderer::DrawLineBox(pWorld, bbox, color);
    }
  }
}

void ezProceduralPlacementComponentManager::PlaceObjects(const ezWorldModule::UpdateContext& context)
{
  EZ_PROFILE_SCOPE("Place objects");

  for (ezUInt32 i = 0; i < m_PlacementTaskInfos.GetCount(); ++i)
  {
    auto& taskInfo = m_PlacementTaskInfos[i];
    if (!taskInfo.IsValid() || !taskInfo.IsScheduled())
      continue;

    if (taskInfo.m_pTask->IsTaskFinished())
    {
      ezUInt32 uiTileIndex = taskInfo.m_uiTileIndex;
      auto& activeTile = m_ActiveTiles[uiTileIndex];
      if (activeTile.PlaceObjects(*GetWorld(), *taskInfo.m_pTask) > 0)
      {
        auto& tileDesc = activeTile.GetDesc();

        auto& activeLayer = m_ActiveResources[tileDesc.m_uiResourceIdHash].m_Layers[tileDesc.m_uiLayerIndex];

        ezUInt64 uiTileKey = GetTileKey(tileDesc.m_iPosX, tileDesc.m_iPosY, 0);
        activeLayer.m_TileIndices[uiTileKey] = uiTileIndex;
      }
      else
      {
        // mark tile for re-use
        DeallocateTile(uiTileIndex);
      }

      // mark task for re-use
      DeallocatePlacementTask(i);
    }
  }
}

void ezProceduralPlacementComponentManager::AddComponent(ezProceduralPlacementComponent* pComponent)
{
  auto& hResource = pComponent->GetResource();
  if (!hResource.IsValid())
  {
    return;
  }

  ezUInt32 uiResourceIdHash = hResource.GetResourceIDHash();
  if (!m_ActiveResources.Contains(uiResourceIdHash))
  {
    m_ResourcesToUpdate.PushBack(hResource);
  }

  ezSimdTransform localBoundingBox = pComponent->GetOwner()->GetGlobalTransformSimd();
  localBoundingBox.m_Scale = localBoundingBox.m_Scale.CompMul(ezSimdConversion::ToVec3(pComponent->GetExtents() * 0.5f));
  localBoundingBox.Invert();

  ActiveResource& activeResource = m_ActiveResources[uiResourceIdHash];

  auto& bounds = activeResource.m_Bounds.ExpandAndGetRef();
  bounds.m_GlobalBoundingBox = pComponent->GetOwner()->GetGlobalBoundsSimd().GetBox();
  bounds.m_LocalBoundingBox = localBoundingBox;
  bounds.m_hComponent = pComponent->GetHandle();
}

void ezProceduralPlacementComponentManager::RemoveComponent(ezProceduralPlacementComponent* pComponent)
{
  auto& hResource = pComponent->GetResource();
  if (!hResource.IsValid())
  {
    return;
  }

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
        pActiveResource->m_Bounds.RemoveAtAndSwap(i);
        break;
      }
    }
  }

  RemoveTilesForResource(uiResourceIdHash);
}

ezUInt32 ezProceduralPlacementComponentManager::AllocateTile(const TileDesc& desc, ezSharedPtr<const Layer>& pLayer)
{
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

  m_ActiveTiles[uiNewTileIndex].Initialize(desc, pLayer);
  return uiNewTileIndex;
}

void ezProceduralPlacementComponentManager::DeallocateTile(ezUInt32 uiTileIndex)
{
  m_ActiveTiles[uiTileIndex].Deinitialize(*GetWorld());
  m_FreeTiles.PushBack(uiTileIndex);
}

ezUInt32 ezProceduralPlacementComponentManager::AllocatePlacementTask(ezUInt32 uiTileIndex)
{
  ezUInt32 uiNewTaskIndex = ezInvalidIndex;
  if (!m_FreePlacementTasks.IsEmpty())
  {
    uiNewTaskIndex = m_FreePlacementTasks.PeekBack();
    m_FreePlacementTasks.PopBack();
  }
  else
  {
    uiNewTaskIndex = m_PlacementTaskInfos.GetCount();
    auto& newTask = m_PlacementTaskInfos.ExpandAndGetRef();

    newTask.m_pTask = EZ_DEFAULT_NEW(PlacementTask);
  }

  m_PlacementTaskInfos[uiNewTaskIndex].m_uiTileIndex = uiTileIndex;
  return uiNewTaskIndex;
}

void ezProceduralPlacementComponentManager::DeallocatePlacementTask(ezUInt32 uiPlacementTaskIndex)
{
  auto& task = m_PlacementTaskInfos[uiPlacementTaskIndex];
  if (task.IsScheduled())
  {
    ezTaskSystem::WaitForGroup(task.m_taskGroupID);
  }

  task.m_pTask->Clear();
  task.Invalidate();

  m_FreePlacementTasks.PushBack(uiPlacementTaskIndex);
}

ezUInt32 ezProceduralPlacementComponentManager::GetNumAllocatedPlacementTasks() const
{
  return m_PlacementTaskInfos.GetCount() - m_FreePlacementTasks.GetCount();
}

void ezProceduralPlacementComponentManager::RemoveTilesForResource(ezUInt32 uiResourceIdHash, bool* out_bAnyObjectsRemoved)
{
  ActiveResource* pActiveResource = nullptr;
  if (!m_ActiveResources.TryGetValue(uiResourceIdHash, pActiveResource))
    return;

  for (auto& layer : pActiveResource->m_Layers)
  {
    layer.m_TileIndices.Clear();
  }

  for (ezUInt32 uiNewTileIndex = 0; uiNewTileIndex < m_NewTiles.GetCount(); ++uiNewTileIndex)
  {
    if (m_NewTiles[uiNewTileIndex].m_uiResourceIdHash == uiResourceIdHash)
    {
      m_NewTiles.RemoveAtAndSwap(uiNewTileIndex);
      --uiNewTileIndex;
    }
  }

  for (ezUInt32 uiTileIndex = 0; uiTileIndex < m_ActiveTiles.GetCount(); ++uiTileIndex)
  {
    auto& activeTile = m_ActiveTiles[uiTileIndex];
    if (!activeTile.IsValid())
      continue;

    auto& tileDesc = activeTile.GetDesc();
    if (tileDesc.m_uiResourceIdHash == uiResourceIdHash)
    {
      if (out_bAnyObjectsRemoved != nullptr && !m_ActiveTiles[uiTileIndex].GetPlacedObjects().IsEmpty())
      {
        *out_bAnyObjectsRemoved = true;
      }

      DeallocateTile(uiTileIndex);

      for (ezUInt32 i = 0; i < m_PlacementTaskInfos.GetCount(); ++i)
      {
        auto& taskInfo = m_PlacementTaskInfos[i];
        if (taskInfo.m_uiTileIndex == uiTileIndex)
        {
          DeallocatePlacementTask(i);
        }
      }
    }
  }
}

void ezProceduralPlacementComponentManager::OnResourceEvent(const ezResourceEvent& resourceEvent)
{
  if (resourceEvent.m_EventType != ezResourceEventType::ResourceContentUnloading)
    return;

  if (auto pResource = ezDynamicCast<const ezProceduralPlacementResource*>(resourceEvent.m_pResource))
  {
    ezProceduralPlacementResourceHandle hResource = pResource->GetHandle();

    if (!m_ResourcesToUpdate.Contains(hResource))
    {
      m_ResourcesToUpdate.PushBack(hResource);
    }
  }
}

void ezProceduralPlacementComponentManager::AddVisibleResource(const ezProceduralPlacementResourceHandle& hResource,
                                                               const ezVec3& cameraPosition, const ezVec3& cameraDirection) const
{
  EZ_LOCK(m_VisibleResourcesMutex);

  for (auto& visibleResource : m_VisibleResources)
  {
    if (visibleResource.m_hResource == hResource && visibleResource.m_vCameraPosition == cameraPosition &&
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
  EZ_BEGIN_PROPERTIES{
      EZ_ACCESSOR_PROPERTY("Resource", GetResourceFile, SetResourceFile)
          ->AddAttributes(new ezAssetBrowserAttribute("Procedural Placement")),
      EZ_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)
          ->AddAttributes(new ezDefaultValueAttribute(ezVec3(10.0f)), new ezClampValueAttribute(ezVec3(0), ezVariant())),
  } EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS{
      EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnUpdateLocalBounds),
      EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnExtractRenderData),
  } EZ_END_MESSAGEHANDLERS; EZ_BEGIN_ATTRIBUTES{
      new ezCategoryAttribute("ProceduralPlacement"),
      new ezBoxManipulatorAttribute("Extents"),
      new ezBoxVisualizerAttribute("Extents"),
  } EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE

ezProceduralPlacementComponent::ezProceduralPlacementComponent()
{
  m_vExtents.Set(10.0f);
}

ezProceduralPlacementComponent::~ezProceduralPlacementComponent() {}

void ezProceduralPlacementComponent::OnActivated()
{
  GetOwner()->UpdateLocalBounds();
  GetWorld()->GetComponentManager<ezProceduralPlacementComponentManager>()->AddComponent(this);
}

void ezProceduralPlacementComponent::OnDeactivated()
{
  GetOwner()->UpdateLocalBounds();
  GetWorld()->GetComponentManager<ezProceduralPlacementComponentManager>()->RemoveComponent(this);
}

void ezProceduralPlacementComponent::SetResourceFile(const char* szFile)
{
  ezProceduralPlacementResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezProceduralPlacementResource>(szFile, ezResourcePriority::High,
                                                                               ezProceduralPlacementResourceHandle());
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
    GetWorld()->GetComponentManager<ezProceduralPlacementComponentManager>()->RemoveComponent(this);
  }

  m_hResource = hResource;

  if (IsActiveAndInitialized())
  {
    GetWorld()->GetComponentManager<ezProceduralPlacementComponentManager>()->AddComponent(this);
  }
}

void ezProceduralPlacementComponent::SetExtents(const ezVec3& value)
{
  m_vExtents = value.CompMax(ezVec3::ZeroVector());

  if (IsActiveAndInitialized())
  {
    GetWorld()->GetComponentManager<ezProceduralPlacementComponentManager>()->RemoveComponent(this);

    GetOwner()->UpdateLocalBounds();

    GetWorld()->GetComponentManager<ezProceduralPlacementComponentManager>()->AddComponent(this);
  }
}

void ezProceduralPlacementComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg)
{
  msg.AddBounds(ezBoundingBox(-m_vExtents * 0.5f, m_vExtents * 0.5f));
}

void ezProceduralPlacementComponent::OnExtractRenderData(ezMsgExtractRenderData& msg) const
{
  // Don't extract render data for selection or in shadow views.
  if (msg.m_OverrideCategory != ezInvalidRenderDataCategory)
    return;

  if (msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::MainView ||
      msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::EditorView)
  {
    const ezCamera* pCamera = msg.m_pView->GetCullingCamera();

    ezVec3 cameraPosition = pCamera->GetCenterPosition();
    ezVec3 cameraDirection = pCamera->GetCenterDirForwards();

    if (m_hResource.IsValid())
    {
      GetWorld()->GetComponentManager<ezProceduralPlacementComponentManager>()->AddVisibleResource(m_hResource, cameraPosition,
                                                                                                   cameraDirection);
    }
  }
}

void ezProceduralPlacementComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  ezStreamWriter& s = stream.GetStream();

  s << m_hResource;
  s << m_vExtents;
}

void ezProceduralPlacementComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

  s >> m_hResource;
  s >> m_vExtents;
}
