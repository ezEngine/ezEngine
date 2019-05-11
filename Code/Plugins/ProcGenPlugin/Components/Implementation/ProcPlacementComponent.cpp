#include <ProcGenPluginPCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Profiling/Profiling.h>
#include <GameEngine/Interfaces/PhysicsWorldModule.h>
#include <ProcGenPlugin/Components/Implementation/PlacementTile.h>
#include <ProcGenPlugin/Components/ProcPlacementComponent.h>
#include <ProcGenPlugin/Tasks/FindPlacementTilesTask.h>
#include <ProcGenPlugin/Tasks/PlacementTask.h>
#include <ProcGenPlugin/Tasks/PreparePlacementTask.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

using namespace ezProcGenInternal;

ezCVarInt CVarMaxProcessingTiles("pp_MaxProcessingTiles", 8, ezCVarFlags::Default, "Maximum number of tiles in process");
ezCVarInt CVarMaxPlacedObjects("pp_MaxPlacedObjects", 128, ezCVarFlags::Default, "Maximum number of objects placed per frame");
ezCVarBool CVarVisTiles("pp_VisTiles", false, ezCVarFlags::Default, "Enables debug visualization of procedural placement tiles");

ezProcPlacementComponentManager::ezProcPlacementComponentManager(ezWorld* pWorld)
  : ezComponentManager<ezProcPlacementComponent, ezBlockStorageType::Compact>(pWorld)
{
}

ezProcPlacementComponentManager::~ezProcPlacementComponentManager() {}

void ezProcPlacementComponentManager::Initialize()
{
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

  ezResourceManager::s_ResourceEvents.AddEventHandler(ezMakeDelegate(&ezProcPlacementComponentManager::OnResourceEvent, this));
}

void ezProcPlacementComponentManager::Deinitialize()
{
  ezResourceManager::s_ResourceEvents.RemoveEventHandler(ezMakeDelegate(&ezProcPlacementComponentManager::OnResourceEvent, this));

  for (auto& activeTile : m_ActiveTiles)
  {
    activeTile.Deinitialize(*GetWorld());
  }
  m_ActiveTiles.Clear();
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

    ezResourceLock<ezProcGenGraphResource> pResource(pComponent->m_hResource, ezResourceAcquireMode::NoFallback);
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
        ezTaskSystem::AddTaskToGroup(m_UpdateTilesTaskGroupID, outputContext.m_pUpdateTilesTask.Borrow());
      }
    }
  }

  ezTaskSystem::StartTaskGroup(m_UpdateTilesTaskGroupID);
}

void ezProcPlacementComponentManager::PreparePlace(const ezWorldModule::UpdateContext& context)
{
  // Find new active tiles and remove old ones
  {
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
        m_NewTiles.PushBackRange(outputContext.m_pUpdateTilesTask->GetNewTiles());

        auto oldTiles = outputContext.m_pUpdateTilesTask->GetOldTiles();
        for (ezUInt64 uiOldTileKey : oldTiles)
        {
          ezProcPlacementComponent::OutputContext::TileIndexAndAge tileIndex;
          if (outputContext.m_TileIndices.Remove(uiOldTileKey, &tileIndex))
          {
            DeallocateTile(tileIndex.m_uiIndex);
          }
        }
      }
    }

    // Sort new tiles
    {
      EZ_PROFILE_SCOPE("Sort new tiles");

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

          processingTask.m_uiScheduledFrame = ezRenderWorld::GetFrameCounter();
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

void ezProcPlacementComponentManager::PlaceObjects(const ezWorldModule::UpdateContext& context)
{
  EZ_PROFILE_SCOPE("Place objects");

  m_SortedProcessingTasks.Clear();
  for (ezUInt32 i = 0; i < m_ProcessingTasks.GetCount(); ++i)
  {
    auto& sortedTask = m_SortedProcessingTasks.ExpandAndGetRef();
    sortedTask.m_uiScheduledFrame = m_ProcessingTasks[i].m_uiScheduledFrame;
    sortedTask.m_uiTaskIndex = i;
  }

  m_SortedProcessingTasks.Sort([](auto& taskA, auto& taskB) { return taskA.m_uiScheduledFrame < taskB.m_uiScheduledFrame; });

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
        uiPlacedObjects = activeTile.PlaceObjects(*GetWorld(), task.m_pPlacementTask->GetOutputTransforms());
        if (uiPlacedObjects > 0)
        {
          auto& outputContext = pComponent->m_OutputContexts[tileDesc.m_uiOutputIndex];

          ezUInt64 uiTileKey = GetTileKey(tileDesc.m_iPosX, tileDesc.m_iPosY);
          auto& tile = outputContext.m_TileIndices[uiTileKey];
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
      DeallocateProcessingTask(sortedTask.m_uiTaskIndex);

      uiTotalNumPlacedObjects += uiPlacedObjects;
    }

    if (uiTotalNumPlacedObjects >= (ezUInt32)CVarMaxPlacedObjects)
    {
      break;
    }
  }
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

    ezStringBuilder sName;
    sName.Format("Prepare Task {}", uiNewTaskIndex);
    newTask.m_pPrepareTask = EZ_DEFAULT_NEW(PreparePlacementTask, sName);

    sName.Format("Placement Task {}", uiNewTaskIndex);
    newTask.m_pPlacementTask = EZ_DEFAULT_NEW(PlacementTask, sName);
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

  task.m_pPlacementTask->Clear();
  task.Invalidate();

  m_FreeProcessingTasks.PushBack(uiTaskIndex);
}

ezUInt32 ezProcPlacementComponentManager::GetNumAllocatedProcessingTasks() const
{
  return m_ProcessingTasks.GetCount() - m_FreeProcessingTasks.GetCount();
}

void ezProcPlacementComponentManager::RemoveTilesForComponent(
  ezProcPlacementComponent* pComponent, bool* out_bAnyObjectsRemoved /*= nullptr*/)
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

void ezProcPlacementComponentManager::AddVisibleComponent(
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
      new ezBoxManipulatorAttribute("Extents", "Offset", "Rotation"),
      new ezBoxVisualizerAttribute("Extents", "Offset", "Rotation", nullptr, ezColor::CornflowerBlue),
      new ezTransformManipulatorAttribute("Offset", "Rotation"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_COMPONENT_TYPE(ezProcPlacementComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
      EZ_ACCESSOR_PROPERTY("Resource", GetResourceFile, SetResourceFile)->AddAttributes(new ezAssetBrowserAttribute("ProcGen Graph")),
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

void ezProcPlacementComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg)
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

void ezProcPlacementComponent::OnExtractRenderData(ezMsgExtractRenderData& msg) const
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
      auto pManager = static_cast<const ezProcPlacementComponentManager*>(GetOwningManager());
      pManager->AddVisibleComponent(GetHandle(), cameraPosition, cameraDirection);
    }
  }
}

void ezProcPlacementComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  ezStreamWriter& s = stream.GetStream();

  s << m_hResource;
  s.WriteArray(m_BoxExtents);
}

void ezProcPlacementComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

  s >> m_hResource;
  s.ReadArray(m_BoxExtents);
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
