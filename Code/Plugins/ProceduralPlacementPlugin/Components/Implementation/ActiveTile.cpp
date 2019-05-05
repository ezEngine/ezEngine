#include <ProceduralPlacementPluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <GameEngine/Components/PrefabReferenceComponent.h>
#include <ProceduralPlacementPlugin/Components/Implementation/ActiveTile.h>
#include <ProceduralPlacementPlugin/Tasks/PlacementTask.h>
#include <RendererCore/Messages/SetColorMessage.h>

using namespace ezPPInternal;

ActiveTile::ActiveTile()
  : m_pLayer(nullptr)
  , m_State(State::Invalid)
{
}

ActiveTile::ActiveTile(ActiveTile&& other)
{
  m_Desc = other.m_Desc;
  m_pLayer = other.m_pLayer;

  m_State = other.m_State;
  other.m_State = State::Invalid;

  m_PlacedObjects = std::move(other.m_PlacedObjects);
}

ActiveTile::~ActiveTile()
{
  EZ_ASSERT_DEV(m_State == State::Invalid, "Implementation error");
}

void ActiveTile::Initialize(const TileDesc& desc, ezSharedPtr<const Layer>& pLayer)
{
  m_Desc = desc;
  m_pLayer = pLayer;

  m_State = State::Initialized;
}

void ActiveTile::Deinitialize(ezWorld& world)
{
  for (auto hObject : m_PlacedObjects)
  {
    world.DeleteObjectDelayed(hObject);
  }
  m_PlacedObjects.Clear();

  m_Desc.m_hComponent.Invalidate();
  m_pLayer = nullptr;
  m_State = State::Invalid;
}

bool ActiveTile::IsValid() const
{
  return !m_Desc.m_hComponent.IsInvalidated() && m_pLayer != nullptr;
}

const TileDesc& ActiveTile::GetDesc() const
{
  return m_Desc;
}

const Layer* ActiveTile::GetLayer() const
{
  return m_pLayer;
}

ezArrayPtr<const ezGameObjectHandle> ActiveTile::GetPlacedObjects() const
{
  return m_PlacedObjects;
}

ezBoundingBox ActiveTile::GetBoundingBox() const
{
  float fTileSize = m_pLayer->GetTileSize();
  ezVec2 vCenter = ezVec2(m_Desc.m_iPosX * fTileSize, m_Desc.m_iPosY * fTileSize);
  ezVec3 vMin = (vCenter - ezVec2(fTileSize * 0.5f)).GetAsVec3(m_Desc.m_fMinZ);
  ezVec3 vMax = (vCenter + ezVec2(fTileSize * 0.5f)).GetAsVec3(m_Desc.m_fMaxZ);

  return ezBoundingBox(vMin, vMax);
}

ezColor ActiveTile::GetDebugColor() const
{
  switch (m_State)
  {
    case State::Initialized:
      return ezColor::Orange;
    case State::Scheduled:
      return ezColor::Yellow;
    case State::Finished:
      return ezColor::Green;
  }

  return ezColor::DarkRed;
}

void ActiveTile::PrepareTask(const ezPhysicsWorldModuleInterface* pPhysicsModule, PlacementTask& placementTask)
{
  EZ_ASSERT_DEV(pPhysicsModule != nullptr, "Physics module must be valid");

  placementTask.m_pPhysicsModule = pPhysicsModule;
  placementTask.m_pLayer = m_pLayer;
  placementTask.m_iTileSeed = (m_Desc.m_iPosX << 11) ^ (m_Desc.m_iPosY * 17);
  placementTask.m_TileBoundingBox = GetBoundingBox();
  placementTask.m_GlobalToLocalBoxTransforms = m_Desc.m_GlobalToLocalBoxTransforms;

  m_State = State::Scheduled;
}

ezUInt32 ActiveTile::PlaceObjects(ezWorld& world, ezArrayPtr<const PlacementTransform> objectTransforms)
{
  ezGameObjectDesc desc;
  auto& objectsToPlace = m_pLayer->m_ObjectsToPlace;

  ezHybridArray<ezPrefabResource*, 4> prefabs;
  prefabs.SetCount(objectsToPlace.GetCount());

  for (auto& objectTransform : objectTransforms)
  {
    const ezUInt32 uiObjectIndex = objectTransform.m_uiObjectIndex;
    ezPrefabResource* pPrefab = prefabs[uiObjectIndex];

    if (pPrefab == nullptr)
    {
      pPrefab = ezResourceManager::BeginAcquireResource(objectsToPlace[uiObjectIndex], ezResourceAcquireMode::NoFallback);
      prefabs[uiObjectIndex] = pPrefab;
    }

    ezTransform transform = ezSimdConversion::ToTransform(objectTransform.m_Transform);
    ezHybridArray<ezGameObject*, 8> rootObjects;
    pPrefab->InstantiatePrefab(world, transform, ezGameObjectHandle(), &rootObjects, nullptr, nullptr);

    for (auto pRootObject : rootObjects)
    {
      // Set the color
      ezMsgSetColor msg;
      msg.m_Color = objectTransform.m_Color;
      pRootObject->PostMessageRecursive(msg, ezObjectMsgQueueType::AfterInitialized);

      m_PlacedObjects.PushBack(pRootObject->GetHandle());
    }
  }

  for (auto pPrefab : prefabs)
  {
    if (pPrefab != nullptr)
    {
      ezResourceManager::EndAcquireResource(pPrefab);
    }
  }

  m_State = State::Finished;

  return m_PlacedObjects.GetCount();
}
