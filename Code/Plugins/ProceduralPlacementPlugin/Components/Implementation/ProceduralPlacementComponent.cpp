#include <ProceduralPlacementPluginPCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Profiling/Profiling.h>
#include <GameEngine/Interfaces/PhysicsWorldModule.h>
#include <ProceduralPlacementPlugin/Components/Implementation/ActiveTile.h>
#include <ProceduralPlacementPlugin/Components/ProceduralPlacementComponent.h>
#include <ProceduralPlacementPlugin/Tasks/PlacementTask.h>
#include <ProceduralPlacementPlugin/Tasks/PrepareTask.h>
#include <ProceduralPlacementPlugin/Tasks/UpdateTilesTask.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

using namespace ezPPInternal;

ezCVarInt CVarMaxProcessingTiles("pp_MaxProcessingTiles", 8, ezCVarFlags::Default, "Maximum number of tiles in process");
ezCVarInt CVarMaxPlacedObjects("pp_MaxPlacedObjects", 128, ezCVarFlags::Default, "Maximum number of objects placed per frame");
ezCVarBool CVarVisTiles("pp_VisTiles", false, ezCVarFlags::Default, "Enables debug visualization of procedural placement tiles");

ezProceduralPlacementComponentManager::ezProceduralPlacementComponentManager(ezWorld* pWorld)
  : ezComponentManager<ezProceduralPlacementComponent, ezBlockStorageType::Compact>(pWorld)
{
}

ezProceduralPlacementComponentManager::~ezProceduralPlacementComponentManager() {}

void ezProceduralPlacementComponentManager::Initialize()
{
  {
    auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezProceduralPlacementComponentManager::UpdateTiles, this);
    desc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::PreAsync;
    desc.m_fPriority = 10000.0f;

    this->RegisterUpdateFunction(desc);
  }

  {
    auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezProceduralPlacementComponentManager::PreparePlace, this);
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

void ezProceduralPlacementComponentManager::UpdateTiles(const ezWorldModule::UpdateContext& context)
{
  // Update resource data
  bool bAnyObjectsRemoved = false;

  for (auto& hComponent : m_ComponentsToUpdate)
  {
    ezProceduralPlacementComponent* pComponent = nullptr;
    if (!TryGetComponent(hComponent, pComponent))
    {
      continue;
    }

    RemoveTilesForComponent(pComponent, &bAnyObjectsRemoved);

    ezResourceLock<ezProceduralPlacementResource> pResource(pComponent->m_hResource, ezResourceAcquireMode::NoFallback);
    auto layers = pResource->GetLayers();

    pComponent->m_Layers.Clear();
    for (ezUInt32 uiLayerIndex = 0; uiLayerIndex < layers.GetCount(); ++uiLayerIndex)
    {
      const auto& pLayer = layers[uiLayerIndex];
      if (pLayer->IsValid())
      {
        auto& activeLayer = pComponent->m_Layers.ExpandAndGetRef();
        activeLayer.m_pLayer = pLayer;
        activeLayer.m_pUpdateTilesTask = EZ_DEFAULT_NEW(UpdateTilesTask, pComponent, uiLayerIndex);
      }
    }
  }
  m_ComponentsToUpdate.Clear();

  // If we removed any objects during resource update do nothing else this frame so objects are actually deleted before we place new ones.
  if (bAnyObjectsRemoved)
  {
    return;
  }

  // Schedule tile update tasks
  m_UpdateTilesTaskGroupID = ezTaskSystem::CreateTaskGroup(ezTaskPriority::EarlyThisFrame);

  for (auto& visibleComponent : m_VisibleComponents)
  {
    ezProceduralPlacementComponent* pComponent = nullptr;
    if (!TryGetComponent(visibleComponent.m_hComponent, pComponent))
    {
      continue;
    }

    auto& activeLayers = pComponent->m_Layers;
    for (auto& activeLayer : activeLayers)
    {
      activeLayer.m_pUpdateTilesTask->AddCameraPosition(visibleComponent.m_vCameraPosition);

      if (activeLayer.m_pUpdateTilesTask->IsTaskFinished())
      {
        ezTaskSystem::AddTaskToGroup(m_UpdateTilesTaskGroupID, activeLayer.m_pUpdateTilesTask.Borrow());
      }
    }
  }

  ezTaskSystem::StartTaskGroup(m_UpdateTilesTaskGroupID);
}

void ezProceduralPlacementComponentManager::PreparePlace(const ezWorldModule::UpdateContext& context)
{
  // Find new active tiles and remove old ones
  {
    ezTaskSystem::WaitForGroup(m_UpdateTilesTaskGroupID);
    m_UpdateTilesTaskGroupID.Invalidate();

    for (auto& visibleComponent : m_VisibleComponents)
    {
      ezProceduralPlacementComponent* pComponent = nullptr;
      if (!TryGetComponent(visibleComponent.m_hComponent, pComponent))
      {
        continue;
      }

      auto& activeLayers = pComponent->m_Layers;
      for (auto& activeLayer : activeLayers)
      {
        m_NewTiles.PushBackRange(activeLayer.m_pUpdateTilesTask->GetNewTiles());

        auto oldTiles = activeLayer.m_pUpdateTilesTask->GetOldTiles();
        for (ezUInt64 uiOldTileKey : oldTiles)
        {
          ezProceduralPlacementComponent::ActiveLayer::TileIndexAndAge tileIndex;
          if (activeLayer.m_TileIndices.Remove(uiOldTileKey, &tileIndex))
          {
            DeallocateTile(tileIndex.m_uiIndex);
          }
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
        for (auto& visibleComponent : m_VisibleComponents)
        {
          float fDistance = (tilePos - visibleComponent.m_vCameraPosition.GetAsVec2()).GetLengthSquared();
          fMinDistance = ezMath::Min(fMinDistance, fDistance);
        }

        newTile.m_fDistanceToCamera = fMinDistance;
      }

      // Sort by distance, larger distances come first since new tiles are processed in reverse order.
      m_NewTiles.Sort([](auto& tileA, auto& tileB) { return tileA.m_fDistanceToCamera > tileB.m_fDistanceToCamera; });
    }

    ClearVisibleComponents();
  }

  // Allocate new tiles and placement tasks
  {
    EZ_PROFILE_SCOPE("Allocate new tiles");

    while (!m_NewTiles.IsEmpty() && GetNumAllocatedProcessingTasks() < (ezUInt32)CVarMaxProcessingTiles)
    {
      const TileDesc& newTile = m_NewTiles.PeekBack();

      ezProceduralPlacementComponent* pComponent = nullptr;
      if (TryGetComponent(newTile.m_hComponent, pComponent))
      {
        auto& pLayer = pComponent->m_Layers[newTile.m_uiLayerIndex].m_pLayer;
        ezUInt32 uiNewTileIndex = AllocateTile(newTile, pLayer);

        AllocateProcessingTask(uiNewTileIndex);
      }

      m_NewTiles.PopBack();
    }
  }

  const ezWorld* pWorld = GetWorld();

  // Update processing tasks
  if (GetWorldSimulationEnabled())
  {
    if (const ezPhysicsWorldModuleInterface* pPhysicsModule = pWorld->GetModule<ezPhysicsWorldModuleInterface>())
    {
      {
        EZ_PROFILE_SCOPE("Prepare processing tasks");

        ezTaskGroupID prepareTaskGroupID = ezTaskSystem::CreateTaskGroup(ezTaskPriority::EarlyThisFrame);

        for (auto& processingTask : m_ProcessingTasks)
        {
          if (!processingTask.IsValid() || processingTask.IsScheduled())
            continue;

          ezTaskSystem::AddTaskToGroup(prepareTaskGroupID, processingTask.m_pPrepareTask.Borrow());
        }

        ezTaskSystem::StartTaskGroup(prepareTaskGroupID);
        ezTaskSystem::WaitForGroup(prepareTaskGroupID);
      }

      {
        EZ_PROFILE_SCOPE("Kickoff placement tasks");

        for (auto& processingTask : m_ProcessingTasks)
        {
          if (!processingTask.IsValid() || processingTask.IsScheduled())
            continue;

          auto& activeTile = m_ActiveTiles[processingTask.m_uiTileIndex];
          activeTile.PrepareTask(pPhysicsModule, *processingTask.m_pPlacementTask);

          processingTask.m_PlacementTaskGroupID =
            ezTaskSystem::StartSingleTask(processingTask.m_pPlacementTask.Borrow(), ezTaskPriority::LongRunningHighPriority);
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

  ezUInt32 uiTotalNumPlacedObjects = 0;

  for (ezUInt32 i = 0; i < m_ProcessingTasks.GetCount(); ++i)
  {
    auto& task = m_ProcessingTasks[i];
    if (!task.IsValid() || !task.IsScheduled())
      continue;

    if (task.m_pPlacementTask->IsTaskFinished())
    {
      ezUInt32 uiPlacedObjects = 0;

      ezUInt32 uiTileIndex = task.m_uiTileIndex;
      auto& activeTile = m_ActiveTiles[uiTileIndex];

      auto& tileDesc = activeTile.GetDesc();
      ezProceduralPlacementComponent* pComponent = nullptr;
      if (TryGetComponent(tileDesc.m_hComponent, pComponent))
      {
        uiPlacedObjects = activeTile.PlaceObjects(*GetWorld(), task.m_pPlacementTask->GetOutputTransforms());
        if (uiPlacedObjects > 0)
        {
          auto& activeLayer = pComponent->m_Layers[tileDesc.m_uiLayerIndex];

          ezUInt64 uiTileKey = GetTileKey(tileDesc.m_iPosX, tileDesc.m_iPosY);
          auto& tile = activeLayer.m_TileIndices[uiTileKey];
          tile.m_uiIndex = uiTileIndex;
          tile.m_uiLastSeenFrame = ezRenderWorld::GetFrameCounter();
        }
      }

      if (uiPlacedObjects == 0)
      {
        // mark tile for re-use
        DeallocateTile(uiTileIndex);
      }

      // mark task for re-use
      DeallocateProcessingTask(i);

      uiTotalNumPlacedObjects += uiPlacedObjects;
    }

    if (uiTotalNumPlacedObjects >= (ezUInt32)CVarMaxPlacedObjects)
    {
      break;
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

  m_ComponentsToUpdate.PushBack(pComponent->GetHandle());
}

void ezProceduralPlacementComponentManager::RemoveComponent(ezProceduralPlacementComponent* pComponent)
{
  auto& hResource = pComponent->GetResource();
  if (!hResource.IsValid())
  {
    return;
  }

  RemoveTilesForComponent(pComponent);
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

ezUInt32 ezProceduralPlacementComponentManager::AllocateProcessingTask(ezUInt32 uiTileIndex)
{
  ezUInt32 uiNewTaskIndex = ezInvalidIndex;
  if (!m_FreeProcessingTasks.IsEmpty())
  {
    uiNewTaskIndex = m_FreeProcessingTasks.PeekBack();
    m_FreeProcessingTasks.PopBack();
  }
  else
  {
    uiNewTaskIndex = m_ProcessingTasks.GetCount();
    auto& newTask = m_ProcessingTasks.ExpandAndGetRef();

    ezStringBuilder sName;
    sName.Format("Prepare Task {}", uiNewTaskIndex);
    newTask.m_pPrepareTask = EZ_DEFAULT_NEW(PrepareTask, sName);

    sName.Format("Placement Task {}", uiNewTaskIndex);
    newTask.m_pPlacementTask = EZ_DEFAULT_NEW(PlacementTask, sName);
  }

  m_ProcessingTasks[uiNewTaskIndex].m_uiTileIndex = uiTileIndex;
  return uiNewTaskIndex;
}

void ezProceduralPlacementComponentManager::DeallocateProcessingTask(ezUInt32 uiTaskIndex)
{
  auto& task = m_ProcessingTasks[uiTaskIndex];
  if (task.IsScheduled())
  {
    ezTaskSystem::WaitForGroup(task.m_PlacementTaskGroupID);
  }

  task.m_pPlacementTask->Clear();
  task.Invalidate();

  m_FreeProcessingTasks.PushBack(uiTaskIndex);
}

ezUInt32 ezProceduralPlacementComponentManager::GetNumAllocatedProcessingTasks() const
{
  return m_ProcessingTasks.GetCount() - m_FreeProcessingTasks.GetCount();
}

void ezProceduralPlacementComponentManager::RemoveTilesForComponent(
  ezProceduralPlacementComponent* pComponent, bool* out_bAnyObjectsRemoved /*= nullptr*/)
{
  ezComponentHandle hComponent = pComponent->GetHandle();

  for (ezUInt32 uiNewTileIndex = 0; uiNewTileIndex < m_NewTiles.GetCount(); ++uiNewTileIndex)
  {
    if (m_NewTiles[uiNewTileIndex].m_hComponent == hComponent)
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
    if (tileDesc.m_hComponent == hComponent)
    {
      if (out_bAnyObjectsRemoved != nullptr && !m_ActiveTiles[uiTileIndex].GetPlacedObjects().IsEmpty())
      {
        *out_bAnyObjectsRemoved = true;
      }

      DeallocateTile(uiTileIndex);

      for (ezUInt32 i = 0; i < m_ProcessingTasks.GetCount(); ++i)
      {
        auto& taskInfo = m_ProcessingTasks[i];
        if (taskInfo.m_uiTileIndex == uiTileIndex)
        {
          DeallocateProcessingTask(i);
        }
      }
    }
  }
}

void ezProceduralPlacementComponentManager::OnResourceEvent(const ezResourceEvent& resourceEvent)
{
  if (resourceEvent.m_Type != ezResourceEvent::Type::ResourceContentUnloading)
    return;

  if (auto pResource = ezDynamicCast<const ezProceduralPlacementResource*>(resourceEvent.m_pResource))
  {
    ezProceduralPlacementResourceHandle hResource = pResource->GetResourceHandle();

    for (auto it = GetComponents(); it.IsValid(); it.Next())
    {
      if (it->m_hResource == hResource && !m_ComponentsToUpdate.Contains(it->GetHandle()))
      {
        m_ComponentsToUpdate.PushBack(it->GetHandle());
      }
    }
  }
}

void ezProceduralPlacementComponentManager::AddVisibleComponent(
  const ezComponentHandle& hComponent, const ezVec3& cameraPosition, const ezVec3& cameraDirection) const
{
  EZ_LOCK(m_VisibleComponentsMutex);

  for (auto& visibleComponent : m_VisibleComponents)
  {
    if (visibleComponent.m_hComponent == hComponent && visibleComponent.m_vCameraPosition == cameraPosition &&
        visibleComponent.m_vCameraDirection == cameraDirection)
    {
      return;
    }
  }

  auto& visibleComponent = m_VisibleComponents.ExpandAndGetRef();
  visibleComponent.m_hComponent = hComponent;
  visibleComponent.m_vCameraPosition = cameraPosition;
  visibleComponent.m_vCameraDirection = cameraDirection;
}

void ezProceduralPlacementComponentManager::ClearVisibleComponents()
{
  m_VisibleComponents.Clear();
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezProcGenBoxExtents, ezNoBase, 1, ezRTTIDefaultAllocator<ezProcGenBoxExtents>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Offset", m_vOffset),
    EZ_MEMBER_PROPERTY("Rotation", m_Rotation),
    EZ_MEMBER_PROPERTY("Extents", m_vExtents)->AddAttributes(new ezDefaultValueAttribute(ezVec3(10.0f)), new ezClampValueAttribute(ezVec3(0), ezVariant())),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
      new ezBoxManipulatorAttribute("Extents", "Offset", "Rotation"),
      new ezBoxVisualizerAttribute("Extents", "Offset", "Rotation", nullptr, ezColor::CornflowerBlue),
      new ezTransformManipulatorAttribute("Offset", "Rotation"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_COMPONENT_TYPE(ezProceduralPlacementComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
      EZ_ACCESSOR_PROPERTY("Resource", GetResourceFile, SetResourceFile)->AddAttributes(new ezAssetBrowserAttribute("Procedural Placement")),
      EZ_ARRAY_ACCESSOR_PROPERTY("BoxExtents", BoxExtents_GetCount, BoxExtents_GetValue, BoxExtents_SetValue, BoxExtents_Insert, BoxExtents_Remove),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
      EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnUpdateLocalBounds),
      EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
      new ezCategoryAttribute("ProceduralPlacement"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezProceduralPlacementComponent::ezProceduralPlacementComponent() = default;
ezProceduralPlacementComponent::~ezProceduralPlacementComponent() = default;
ezProceduralPlacementComponent& ezProceduralPlacementComponent::operator=(ezProceduralPlacementComponent&& other) = default;

void ezProceduralPlacementComponent::OnActivated()
{
  UpdateBoundsAndTiles();
}

void ezProceduralPlacementComponent::OnDeactivated()
{
  GetOwner()->UpdateLocalBounds();

  m_Bounds.Clear();
  m_Layers.Clear();

  auto pManager = static_cast<ezProceduralPlacementComponentManager*>(GetOwningManager());
  pManager->RemoveComponent(this);
}

void ezProceduralPlacementComponent::SetResourceFile(const char* szFile)
{
  ezProceduralPlacementResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezProceduralPlacementResource>(szFile);
    ezResourceManager::PreloadResource(hResource);
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
  auto pManager = static_cast<ezProceduralPlacementComponentManager*>(GetOwningManager());

  if (IsActiveAndInitialized())
  {
    pManager->RemoveComponent(this);
  }

  m_hResource = hResource;

  if (IsActiveAndInitialized())
  {
    pManager->AddComponent(this);
  }
}

void ezProceduralPlacementComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg)
{
  if (m_BoxExtents.IsEmpty())
    return;

  ezBoundingBoxSphere bounds;
  bounds.SetInvalid();

  for (auto& boxExtent : m_BoxExtents)
  {
    ezBoundingBoxSphere localBox = ezBoundingBox(-boxExtent.m_vExtents * 0.5f, boxExtent.m_vExtents * 0.5f);
    localBox.Transform(ezTransform(boxExtent.m_vOffset, boxExtent.m_Rotation).GetAsMat4());

    bounds.ExpandToInclude(localBox);
  }

  msg.AddBounds(bounds);
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
      auto pManager = static_cast<const ezProceduralPlacementComponentManager*>(GetOwningManager());
      pManager->AddVisibleComponent(GetHandle(), cameraPosition, cameraDirection);
    }
  }
}

void ezProceduralPlacementComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  ezStreamWriter& s = stream.GetStream();

  s << m_hResource;
  s.WriteArray(m_BoxExtents);
}

void ezProceduralPlacementComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

  s >> m_hResource;
  s.ReadArray(m_BoxExtents);
}

ezUInt32 ezProceduralPlacementComponent::BoxExtents_GetCount() const
{
  return m_BoxExtents.GetCount();
}

const ezProcGenBoxExtents& ezProceduralPlacementComponent::BoxExtents_GetValue(ezUInt32 uiIndex) const
{
  return m_BoxExtents[uiIndex];
}

void ezProceduralPlacementComponent::BoxExtents_SetValue(ezUInt32 uiIndex, const ezProcGenBoxExtents& value)
{
  m_BoxExtents.EnsureCount(uiIndex + 1);
  m_BoxExtents[uiIndex] = value;

  UpdateBoundsAndTiles();
}

void ezProceduralPlacementComponent::BoxExtents_Insert(ezUInt32 uiIndex, const ezProcGenBoxExtents& value)
{
  m_BoxExtents.Insert(value, uiIndex);

  UpdateBoundsAndTiles();
}

void ezProceduralPlacementComponent::BoxExtents_Remove(ezUInt32 uiIndex)
{
  m_BoxExtents.RemoveAtAndCopy(uiIndex);

  UpdateBoundsAndTiles();
}

void ezProceduralPlacementComponent::UpdateBoundsAndTiles()
{
  if (IsActiveAndInitialized())
  {
    auto pManager = static_cast<ezProceduralPlacementComponentManager*>(GetOwningManager());

    pManager->RemoveComponent(this);

    GetOwner()->UpdateLocalBounds();

    m_Bounds.Clear();
    m_Layers.Clear();

    ezSimdTransform ownerTransform = GetOwner()->GetGlobalTransformSimd();
    for (auto& boxExtent : m_BoxExtents)
    {
      ezSimdTransform localBoxTransform;
      localBoxTransform.m_Position = ezSimdConversion::ToVec3(boxExtent.m_vOffset);
      localBoxTransform.m_Rotation = ezSimdConversion::ToQuat(boxExtent.m_Rotation);
      localBoxTransform.m_Scale = ezSimdConversion::ToVec3(boxExtent.m_vExtents * 0.5f);

      ezSimdTransform finalBoxTransform;
      finalBoxTransform.SetGlobalTransform(ownerTransform, localBoxTransform);

      ezSimdBBox globalBox(ezSimdVec4f(-1.0f), ezSimdVec4f(1.0f));
      globalBox.Transform(finalBoxTransform.GetAsMat4());

      auto& bounds = m_Bounds.ExpandAndGetRef();
      bounds.m_GlobalBoundingBox = globalBox;
      bounds.m_GlobalToLocalBoxTransform = finalBoxTransform.GetInverse();
    }

    pManager->AddComponent(this);
  }
}

//////////////////////////////////////////////////////////////////////////

ezResult ezProcGenBoxExtents::Serialize(ezStreamWriter& stream) const
{
  stream << m_vOffset;
  stream << m_Rotation;
  stream << m_vExtents;

  return EZ_SUCCESS;
}

ezResult ezProcGenBoxExtents::Deserialize(ezStreamReader& stream)
{
  stream >> m_vOffset;
  stream >> m_Rotation;
  stream >> m_vExtents;

  return EZ_SUCCESS;
}
