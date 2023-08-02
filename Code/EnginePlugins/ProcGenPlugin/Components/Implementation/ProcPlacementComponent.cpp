#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Profiling/Profiling.h>
#include <ProcGenPlugin/Components/Implementation/PlacementTile.h>
#include <ProcGenPlugin/Components/ProcPlacementComponent.h>
#include <ProcGenPlugin/Tasks/FindPlacementTilesTask.h>
#include <ProcGenPlugin/Tasks/PlacementData.h>
#include <ProcGenPlugin/Tasks/PlacementTask.h>
#include <ProcGenPlugin/Tasks/PreparePlacementTask.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

using namespace ezProcGenInternal;

ezCVarInt cvar_ProcGenProcessingMaxTiles("ProcGen.Processing.MaxTiles", 8, ezCVarFlags::Default, "Maximum number of tiles in process");
ezCVarInt cvar_ProcGenProcessingMaxNewObjectsPerFrame("ProcGen.Processing.MaxNewObjectsPerFrame", 128, ezCVarFlags::Default, "Maximum number of objects placed per frame");
ezCVarBool cvar_ProcGenVisTiles("ProcGen.VisTiles", false, ezCVarFlags::Default, "Enables debug visualization of procedural placement tiles");

ezProcPlacementComponentManager::ezProcPlacementComponentManager(ezWorld* pWorld)
  : ezComponentManager<ezProcPlacementComponent, ezBlockStorageType::Compact>(pWorld)
{
}

ezProcPlacementComponentManager::~ezProcPlacementComponentManager() = default;

void ezProcPlacementComponentManager::Initialize()
{
  SUPER::Initialize();

  {
    auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezProcPlacementComponentManager::FindTiles, this);
    desc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::PreAsync;
    desc.m_fPriority = 10000.0f;

    this->RegisterUpdateFunction(desc);
  }

  {
    auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezProcPlacementComponentManager::PreparePlace, this);
    desc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::Async;

    this->RegisterUpdateFunction(desc);
  }

  {
    auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezProcPlacementComponentManager::PlaceObjects, this);
    desc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::PostAsync;

    this->RegisterUpdateFunction(desc);
  }

  ezResourceManager::GetResourceEvents().AddEventHandler(ezMakeDelegate(&ezProcPlacementComponentManager::OnResourceEvent, this));
}

void ezProcPlacementComponentManager::Deinitialize()
{
  ezResourceManager::GetResourceEvents().RemoveEventHandler(ezMakeDelegate(&ezProcPlacementComponentManager::OnResourceEvent, this));

  for (auto& activeTile : m_ActiveTiles)
  {
    activeTile.Deinitialize(*GetWorld());
  }
  m_ActiveTiles.Clear();

  SUPER::Deinitialize();
}

void ezProcPlacementComponentManager::FindTiles(const ezWorldModule::UpdateContext& context)
{
  // Update resource data
  bool bAnyObjectsRemoved = false;

  for (auto& hComponent : m_ComponentsToUpdate)
  {
    ezProcPlacementComponent* pComponent = nullptr;
    if (!TryGetComponent(hComponent, pComponent))
    {
      continue;
    }

    RemoveTilesForComponent(pComponent, &bAnyObjectsRemoved);

    ezResourceLock<ezProcGenGraphResource> pResource(pComponent->m_hResource, ezResourceAcquireMode::BlockTillLoaded);
    auto outputs = pResource->GetPlacementOutputs();

    pComponent->m_OutputContexts.Clear();
    for (ezUInt32 uiIndex = 0; uiIndex < outputs.GetCount(); ++uiIndex)
    {
      const auto& pOutput = outputs[uiIndex];
      if (pOutput->IsValid())
      {
        auto& outputContext = pComponent->m_OutputContexts.ExpandAndGetRef();
        outputContext.m_pOutput = pOutput;
        outputContext.m_pUpdateTilesTask = EZ_DEFAULT_NEW(FindPlacementTilesTask, pComponent, uiIndex);
      }
    }
  }
  m_ComponentsToUpdate.Clear();

  // If we removed any objects during resource update do nothing else this frame so objects are actually deleted before we place new ones.
  if (bAnyObjectsRemoved)
  {
    return;
  }

  // Schedule find tiles tasks
  m_UpdateTilesTaskGroupID = ezTaskSystem::CreateTaskGroup(ezTaskPriority::EarlyThisFrame);

  for (auto& visibleComponent : m_VisibleComponents)
  {
    ezProcPlacementComponent* pComponent = nullptr;
    if (!TryGetComponent(visibleComponent.m_hComponent, pComponent))
    {
      continue;
    }

    auto& outputContexts = pComponent->m_OutputContexts;
    for (auto& outputContext : outputContexts)
    {
      outputContext.m_pUpdateTilesTask->AddCameraPosition(visibleComponent.m_vCameraPosition);

      if (outputContext.m_pUpdateTilesTask->IsTaskFinished())
      {
        ezTaskSystem::AddTaskToGroup(m_UpdateTilesTaskGroupID, outputContext.m_pUpdateTilesTask);
      }
    }
  }

  ezTaskSystem::StartTaskGroup(m_UpdateTilesTaskGroupID);
}

void ezProcPlacementComponentManager::PreparePlace(const ezWorldModule::UpdateContext& context)
{
  // Find new active tiles and remove old ones
  {
    EZ_PROFILE_SCOPE("Add new/remove old tiles");

    ezTaskSystem::WaitForGroup(m_UpdateTilesTaskGroupID);
    m_UpdateTilesTaskGroupID.Invalidate();

    for (auto& visibleComponent : m_VisibleComponents)
    {
      ezProcPlacementComponent* pComponent = nullptr;
      if (!TryGetComponent(visibleComponent.m_hComponent, pComponent))
      {
        continue;
      }

      auto& outputContexts = pComponent->m_OutputContexts;
      for (auto& outputContext : outputContexts)
      {
        auto oldTiles = outputContext.m_pUpdateTilesTask->GetOldTiles();
        for (ezUInt64 uiOldTileKey : oldTiles)
        {
          ezProcPlacementComponent::OutputContext::TileIndexAndAge tileIndex;
          if (outputContext.m_TileIndices.Remove(uiOldTileKey, &tileIndex))
          {
            if (tileIndex.m_uiIndex != NewTileIndex)
            {
              DeallocateTile(tileIndex.m_uiIndex);
            }
          }

          // Also remove from new tiles list
          for (ezUInt32 i = 0; i < m_NewTiles.GetCount(); ++i)
          {
            auto& newTile = m_NewTiles[i];
            ezUInt64 uiTileKey = GetTileKey(newTile.m_iPosX, newTile.m_iPosY);
            if (uiTileKey == uiOldTileKey)
            {
              m_NewTiles.RemoveAtAndSwap(i);
              break;
            }
          }
        }

        m_NewTiles.PushBackRange(outputContext.m_pUpdateTilesTask->GetNewTiles());
      }
    }

    // Sort new tiles
    {
      EZ_PROFILE_SCOPE("Sort new tiles");

      // Update distance to camera
      for (auto& newTile : m_NewTiles)
      {
        ezVec2 tilePos = ezVec2((float)newTile.m_iPosX, (float)newTile.m_iPosY);
        newTile.m_fDistanceToCamera = ezMath::MaxValue<float>();

        for (auto& visibleComponent : m_VisibleComponents)
        {
          ezVec2 cameraPos = visibleComponent.m_vCameraPosition.GetAsVec2() / newTile.m_fTileSize;

          float fDistance = (tilePos - cameraPos).GetLengthSquared();
          newTile.m_fDistanceToCamera = ezMath::Min(newTile.m_fDistanceToCamera, fDistance);
        }
      }

      // Sort by distance, larger distances come first since new tiles are processed in reverse order.
      m_NewTiles.Sort([](auto& ref_tileA, auto& ref_tileB) { return ref_tileA.m_fDistanceToCamera > ref_tileB.m_fDistanceToCamera; });
    }

    ClearVisibleComponents();
  }

  // Debug draw tiles
  if (cvar_ProcGenVisTiles)
  {
    ezStringBuilder sb;
    sb.Format("Procedural Placement Stats:\nNum Tiles to process: {}", m_NewTiles.GetCount());

    ezColor textColor = ezColorScheme::LightUI(ezColorScheme::Grape);
    ezDebugRenderer::DrawInfoText(GetWorld(), ezDebugRenderer::ScreenPlacement::TopLeft, "ProcPlaceStats", sb, textColor);

    for (ezUInt32 i = 0; i < m_NewTiles.GetCount(); ++i)
    {
      DebugDrawTile(m_NewTiles[i], textColor, m_NewTiles.GetCount() - i - 1);
    }

    for (auto& activeTile : m_ActiveTiles)
    {
      if (!activeTile.IsValid())
        continue;

      DebugDrawTile(activeTile.GetDesc(), activeTile.GetDebugColor());
    }
  }

  // Allocate new tiles and placement tasks
  {
    EZ_PROFILE_SCOPE("Allocate new tiles");

    while (!m_NewTiles.IsEmpty() && GetNumAllocatedProcessingTasks() < (ezUInt32)cvar_ProcGenProcessingMaxTiles)
    {
      const PlacementTileDesc& newTile = m_NewTiles.PeekBack();

      ezProcPlacementComponent* pComponent = nullptr;
      if (TryGetComponent(newTile.m_hComponent, pComponent))
      {
        auto& pOutput = pComponent->m_OutputContexts[newTile.m_uiOutputIndex].m_pOutput;
        ezUInt32 uiNewTileIndex = AllocateTile(newTile, pOutput);

        AllocateProcessingTask(uiNewTileIndex);
      }

      m_NewTiles.PopBack();
    }
  }

  const ezWorld* pWorld = GetWorld();

  // Update processing tasks
  if (GetWorldSimulationEnabled())
  {
    {
      EZ_PROFILE_SCOPE("Prepare processing tasks");

      ezTaskGroupID prepareTaskGroupID = ezTaskSystem::CreateTaskGroup(ezTaskPriority::EarlyThisFrame);

      for (auto& processingTask : m_ProcessingTasks)
      {
        if (!processingTask.IsValid() || processingTask.IsScheduled())
          continue;

        auto& activeTile = m_ActiveTiles[processingTask.m_uiTileIndex];
        activeTile.PreparePlacementData(pWorld, pWorld->GetModuleReadOnly<ezPhysicsWorldModuleInterface>(), *processingTask.m_pData);

        ezTaskSystem::AddTaskToGroup(prepareTaskGroupID, processingTask.m_pPrepareTask);
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

        processingTask.m_uiScheduledFrame = ezRenderWorld::GetFrameCounter();
        processingTask.m_PlacementTaskGroupID = ezTaskSystem::StartSingleTask(processingTask.m_pPlacementTask, ezTaskPriority::LongRunningHighPriority);
      }
    }
  }
}

void ezProcPlacementComponentManager::PlaceObjects(const ezWorldModule::UpdateContext& context)
{
  m_SortedProcessingTasks.Clear();
  for (ezUInt32 i = 0; i < m_ProcessingTasks.GetCount(); ++i)
  {
    auto& sortedTask = m_SortedProcessingTasks.ExpandAndGetRef();
    sortedTask.m_uiScheduledFrame = m_ProcessingTasks[i].m_uiScheduledFrame;
    sortedTask.m_uiTaskIndex = i;
  }

  m_SortedProcessingTasks.Sort([](auto& ref_taskA, auto& ref_taskB) { return ref_taskA.m_uiScheduledFrame < ref_taskB.m_uiScheduledFrame; });

  ezUInt32 uiTotalNumPlacedObjects = 0;

  for (auto& sortedTask : m_SortedProcessingTasks)
  {
    auto& task = m_ProcessingTasks[sortedTask.m_uiTaskIndex];
    if (!task.IsValid() || !task.IsScheduled())
      continue;

    if (task.m_pPlacementTask->IsTaskFinished())
    {
      ezUInt32 uiPlacedObjects = 0;

      ezUInt32 uiTileIndex = task.m_uiTileIndex;
      auto& activeTile = m_ActiveTiles[uiTileIndex];

      auto& tileDesc = activeTile.GetDesc();
      ezProcPlacementComponent* pComponent = nullptr;
      if (TryGetComponent(tileDesc.m_hComponent, pComponent))
      {
        auto& outputContext = pComponent->m_OutputContexts[tileDesc.m_uiOutputIndex];

        ezUInt64 uiTileKey = GetTileKey(tileDesc.m_iPosX, tileDesc.m_iPosY);
        if (auto pTile = outputContext.m_TileIndices.GetValue(uiTileKey))
        {
          uiPlacedObjects = activeTile.PlaceObjects(*GetWorld(), task.m_pPlacementTask->GetOutputTransforms());

          pTile->m_uiIndex = uiPlacedObjects > 0 ? uiTileIndex : EmptyTileIndex;
          pTile->m_uiLastSeenFrame = ezRenderWorld::GetFrameCounter();
        }
      }

      if (uiPlacedObjects == 0)
      {
        // mark tile for re-use
        DeallocateTile(uiTileIndex);
      }

      // mark task for re-use
      DeallocateProcessingTask(sortedTask.m_uiTaskIndex);

      uiTotalNumPlacedObjects += uiPlacedObjects;
    }

    if (uiTotalNumPlacedObjects >= (ezUInt32)cvar_ProcGenProcessingMaxNewObjectsPerFrame)
    {
      break;
    }
  }
}

void ezProcPlacementComponentManager::DebugDrawTile(const ezProcGenInternal::PlacementTileDesc& desc, const ezColor& color, ezUInt32 uiQueueIndex)
{
  const ezProcPlacementComponent* pComponent = nullptr;
  if (!TryGetComponent(desc.m_hComponent, pComponent))
    return;

  ezBoundingBox bbox = desc.GetBoundingBox();
  ezDebugRenderer::DrawLineBox(GetWorld(), bbox, color);

  ezUInt64 uiAge = -1;
  auto& outputContext = pComponent->m_OutputContexts[desc.m_uiOutputIndex];
  if (auto pTile = outputContext.m_TileIndices.GetValue(GetTileKey(desc.m_iPosX, desc.m_iPosY)))
  {
    uiAge = ezRenderWorld::GetFrameCounter() - pTile->m_uiLastSeenFrame;
  }

  ezStringBuilder sb;
  if (uiQueueIndex != ezInvalidIndex)
  {
    sb.Format("Queue Index: {}\n", uiQueueIndex);
  }
  sb.AppendFormat("Age: {}\nDistance: {}", uiAge, desc.m_fDistanceToCamera);
  ezDebugRenderer::Draw3DText(GetWorld(), sb, bbox.GetCenter(), color);
}

void ezProcPlacementComponentManager::AddComponent(ezProcPlacementComponent* pComponent)
{
  auto& hResource = pComponent->GetResource();
  if (!hResource.IsValid())
  {
    return;
  }

  m_ComponentsToUpdate.PushBack(pComponent->GetHandle());
}

void ezProcPlacementComponentManager::RemoveComponent(ezProcPlacementComponent* pComponent)
{
  auto& hResource = pComponent->GetResource();
  if (!hResource.IsValid())
  {
    return;
  }

  RemoveTilesForComponent(pComponent);
}

ezUInt32 ezProcPlacementComponentManager::AllocateTile(const PlacementTileDesc& desc, ezSharedPtr<const PlacementOutput>& pOutput)
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

  m_ActiveTiles[uiNewTileIndex].Initialize(desc, pOutput);
  return uiNewTileIndex;
}

void ezProcPlacementComponentManager::DeallocateTile(ezUInt32 uiTileIndex)
{
  m_ActiveTiles[uiTileIndex].Deinitialize(*GetWorld());
  m_FreeTiles.PushBack(uiTileIndex);
}

ezUInt32 ezProcPlacementComponentManager::AllocateProcessingTask(ezUInt32 uiTileIndex)
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

    newTask.m_pData = EZ_DEFAULT_NEW(PlacementData);

    ezStringBuilder sName;
    sName.Format("Prepare Task {}", uiNewTaskIndex);
    newTask.m_pPrepareTask = EZ_DEFAULT_NEW(PreparePlacementTask, newTask.m_pData.Borrow(), sName);

    sName.Format("Placement Task {}", uiNewTaskIndex);
    newTask.m_pPlacementTask = EZ_DEFAULT_NEW(PlacementTask, newTask.m_pData.Borrow(), sName);
  }

  m_ProcessingTasks[uiNewTaskIndex].m_uiTileIndex = uiTileIndex;
  return uiNewTaskIndex;
}

void ezProcPlacementComponentManager::DeallocateProcessingTask(ezUInt32 uiTaskIndex)
{
  auto& task = m_ProcessingTasks[uiTaskIndex];
  if (task.IsScheduled())
  {
    ezTaskSystem::WaitForGroup(task.m_PlacementTaskGroupID);
  }

  task.m_pData->Clear();
  task.m_pPrepareTask->Clear();
  task.m_pPlacementTask->Clear();
  task.Invalidate();

  m_FreeProcessingTasks.PushBack(uiTaskIndex);
}

ezUInt32 ezProcPlacementComponentManager::GetNumAllocatedProcessingTasks() const
{
  return m_ProcessingTasks.GetCount() - m_FreeProcessingTasks.GetCount();
}

void ezProcPlacementComponentManager::RemoveTilesForComponent(ezProcPlacementComponent* pComponent, bool* out_bAnyObjectsRemoved /*= nullptr*/)
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

void ezProcPlacementComponentManager::OnResourceEvent(const ezResourceEvent& resourceEvent)
{
  if (resourceEvent.m_Type != ezResourceEvent::Type::ResourceContentUnloading)
    return;

  if (auto pResource = ezDynamicCast<const ezProcGenGraphResource*>(resourceEvent.m_pResource))
  {
    ezProcGenGraphResourceHandle hResource = pResource->GetResourceHandle();

    for (auto it = GetComponents(); it.IsValid(); it.Next())
    {
      if (it->m_hResource == hResource && !m_ComponentsToUpdate.Contains(it->GetHandle()))
      {
        m_ComponentsToUpdate.PushBack(it->GetHandle());
      }
    }
  }
}

void ezProcPlacementComponentManager::AddVisibleComponent(const ezComponentHandle& hComponent, const ezVec3& cameraPosition, const ezVec3& cameraDirection) const
{
  EZ_LOCK(m_VisibleComponentsMutex);

  for (auto& visibleComponent : m_VisibleComponents)
  {
    if (visibleComponent.m_hComponent == hComponent && visibleComponent.m_vCameraPosition == cameraPosition && visibleComponent.m_vCameraDirection == cameraDirection)
    {
      return;
    }
  }

  auto& visibleComponent = m_VisibleComponents.ExpandAndGetRef();
  visibleComponent.m_hComponent = hComponent;
  visibleComponent.m_vCameraPosition = cameraPosition;
  visibleComponent.m_vCameraDirection = cameraDirection;
}

void ezProcPlacementComponentManager::ClearVisibleComponents()
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
    new ezBoxManipulatorAttribute("Extents", 1.0f, false, "Offset", "Rotation"),
    new ezBoxVisualizerAttribute("Extents", 1.0f, ezColorScheme::LightUI(ezColorScheme::Blue), nullptr, ezVisualizerAnchor::Center, ezVec3(1.0f), "Offset", "Rotation"),
    new ezTransformManipulatorAttribute("Offset", "Rotation"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_COMPONENT_TYPE(ezProcPlacementComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Resource", GetResourceFile, SetResourceFile)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_ProcGen_Graph")),
    EZ_ARRAY_ACCESSOR_PROPERTY("BoxExtents", BoxExtents_GetCount, BoxExtents_GetValue, BoxExtents_SetValue, BoxExtents_Insert, BoxExtents_Remove),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnUpdateLocalBounds),
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnMsgExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Procedural Generation"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezProcPlacementComponent::ezProcPlacementComponent() = default;
ezProcPlacementComponent::~ezProcPlacementComponent() = default;
ezProcPlacementComponent& ezProcPlacementComponent::operator=(ezProcPlacementComponent&& other) = default;

void ezProcPlacementComponent::OnActivated()
{
  UpdateBoundsAndTiles();
}

void ezProcPlacementComponent::OnDeactivated()
{
  GetOwner()->UpdateLocalBounds();

  m_Bounds.Clear();
  m_OutputContexts.Clear();

  auto pManager = static_cast<ezProcPlacementComponentManager*>(GetOwningManager());
  pManager->RemoveComponent(this);
}

void ezProcPlacementComponent::SetResourceFile(const char* szFile)
{
  ezProcGenGraphResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezProcGenGraphResource>(szFile);
    ezResourceManager::PreloadResource(hResource);
  }

  SetResource(hResource);
}

const char* ezProcPlacementComponent::GetResourceFile() const
{
  if (!m_hResource.IsValid())
    return "";

  return m_hResource.GetResourceID();
}

void ezProcPlacementComponent::SetResource(const ezProcGenGraphResourceHandle& hResource)
{
  auto pManager = static_cast<ezProcPlacementComponentManager*>(GetOwningManager());

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

void ezProcPlacementComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& ref_msg)
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

  ref_msg.AddBounds(bounds, GetOwner()->IsDynamic() ? ezDefaultSpatialDataCategories::RenderDynamic : ezDefaultSpatialDataCategories::RenderStatic);
}

void ezProcPlacementComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& ref_msg) const
{
  // Don't extract render data for selection or in shadow views.
  if (ref_msg.m_OverrideCategory != ezInvalidRenderDataCategory)
    return;

  if (ref_msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::MainView || ref_msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::EditorView)
  {
    const ezCamera* pCamera = ref_msg.m_pView->GetCullingCamera();

    ezVec3 cameraPosition = pCamera->GetCenterPosition();
    ezVec3 cameraDirection = pCamera->GetCenterDirForwards();

    if (m_hResource.IsValid())
    {
      auto pManager = static_cast<const ezProcPlacementComponentManager*>(GetOwningManager());
      pManager->AddVisibleComponent(GetHandle(), cameraPosition, cameraDirection);
    }
  }
}

void ezProcPlacementComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  ezStreamWriter& s = inout_stream.GetStream();

  s << m_hResource;
  s.WriteArray(m_BoxExtents).IgnoreResult();
}

void ezProcPlacementComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = inout_stream.GetStream();

  s >> m_hResource;
  s.ReadArray(m_BoxExtents).IgnoreResult();
}

ezUInt32 ezProcPlacementComponent::BoxExtents_GetCount() const
{
  return m_BoxExtents.GetCount();
}

const ezProcGenBoxExtents& ezProcPlacementComponent::BoxExtents_GetValue(ezUInt32 uiIndex) const
{
  return m_BoxExtents[uiIndex];
}

void ezProcPlacementComponent::BoxExtents_SetValue(ezUInt32 uiIndex, const ezProcGenBoxExtents& value)
{
  m_BoxExtents.EnsureCount(uiIndex + 1);
  m_BoxExtents[uiIndex] = value;

  UpdateBoundsAndTiles();
}

void ezProcPlacementComponent::BoxExtents_Insert(ezUInt32 uiIndex, const ezProcGenBoxExtents& value)
{
  m_BoxExtents.Insert(value, uiIndex);

  UpdateBoundsAndTiles();
}

void ezProcPlacementComponent::BoxExtents_Remove(ezUInt32 uiIndex)
{
  m_BoxExtents.RemoveAtAndCopy(uiIndex);

  UpdateBoundsAndTiles();
}

void ezProcPlacementComponent::UpdateBoundsAndTiles()
{
  if (IsActiveAndInitialized())
  {
    auto pManager = static_cast<ezProcPlacementComponentManager*>(GetOwningManager());

    pManager->RemoveComponent(this);

    GetOwner()->UpdateLocalBounds();

    m_Bounds.Clear();
    m_OutputContexts.Clear();

    ezSimdTransform ownerTransform = GetOwner()->GetGlobalTransformSimd();
    for (auto& boxExtent : m_BoxExtents)
    {
      ezSimdTransform localBoxTransform;
      localBoxTransform.m_Position = ezSimdConversion::ToVec3(boxExtent.m_vOffset);
      localBoxTransform.m_Rotation = ezSimdConversion::ToQuat(boxExtent.m_Rotation);
      localBoxTransform.m_Scale = ezSimdConversion::ToVec3(boxExtent.m_vExtents * 0.5f);

      ezSimdTransform finalBoxTransform;
      finalBoxTransform.SetGlobalTransform(ownerTransform, localBoxTransform);

      ezSimdMat4f finalBoxMat = finalBoxTransform.GetAsMat4();

      ezSimdBBox globalBox(ezSimdVec4f(-1.0f), ezSimdVec4f(1.0f));
      globalBox.Transform(finalBoxMat);

      auto& bounds = m_Bounds.ExpandAndGetRef();
      bounds.m_GlobalBoundingBox = globalBox;
      bounds.m_GlobalToLocalBoxTransform = finalBoxMat.GetInverse();
    }

    pManager->AddComponent(this);
  }
}

//////////////////////////////////////////////////////////////////////////

ezResult ezProcGenBoxExtents::Serialize(ezStreamWriter& inout_stream) const
{
  inout_stream << m_vOffset;
  inout_stream << m_Rotation;
  inout_stream << m_vExtents;

  return EZ_SUCCESS;
}

ezResult ezProcGenBoxExtents::Deserialize(ezStreamReader& inout_stream)
{
  inout_stream >> m_vOffset;
  inout_stream >> m_Rotation;
  inout_stream >> m_vExtents;

  return EZ_SUCCESS;
}
