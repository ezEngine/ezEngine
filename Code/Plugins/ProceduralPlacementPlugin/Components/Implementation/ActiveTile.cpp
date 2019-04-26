#include <ProceduralPlacementPluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/SimdMath/SimdRandom.h>
#include <GameEngine/Components/PrefabReferenceComponent.h>
#include <GameEngine/Interfaces/PhysicsWorldModule.h>
#include <GameEngine/Surfaces/SurfaceResource.h>
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
  ezVec3 vMin = ezVec3(m_Desc.m_iPosX * fTileSize, m_Desc.m_iPosY * fTileSize, m_Desc.m_fMinZ);
  ezVec3 vMax = (vMin.GetAsVec2() + ezVec2(fTileSize)).GetAsVec3(m_Desc.m_fMaxZ);

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

  placementTask.m_pLayer = m_pLayer;
  placementTask.m_iTileSeed = (m_Desc.m_iPosX << 11) ^ (m_Desc.m_iPosY * 17);

  ezSimdVec4i seed = ezSimdVec4i(placementTask.m_iTileSeed) + ezSimdVec4i(0, 3, 7, 11);

  ezBoundingBox bbox = GetBoundingBox();
  float fZRange = bbox.GetExtents().z;
  ezSimdFloat fZStart = bbox.m_vMax.z;
  ezSimdVec4f vXY = ezSimdConversion::ToVec3(bbox.m_vMin);
  ezSimdVec4f vMinOffset = ezSimdConversion::ToVec3(m_pLayer->m_vMinOffset);
  ezSimdVec4f vMaxOffset = ezSimdConversion::ToVec3(m_pLayer->m_vMaxOffset);

  ezVec3 rayDir = ezVec3(0, 0, -1);
  ezUInt32 uiCollisionLayer = m_pLayer->m_uiCollisionLayer;

  auto& patternPoints = m_pLayer->m_pPattern->m_Points;

  for (ezUInt32 i = 0; i < patternPoints.GetCount(); ++i)
  {
    auto& patternPoint = patternPoints[i];
    ezSimdVec4f patternCoords = ezSimdConversion::ToVec3(patternPoint.m_Coordinates.GetAsVec3(0.0f));

    ezSimdVec4f rayStart = (vXY + patternCoords * m_pLayer->m_fFootprint);
    rayStart += ezSimdRandom::FloatMinMax(seed + ezSimdVec4i(i), vMinOffset, vMaxOffset);
    rayStart.SetZ(fZStart);

    ezPhysicsHitResult hitResult;
    if (!pPhysicsModule->CastRay(
          ezSimdConversion::ToVec3(rayStart), rayDir, fZRange, uiCollisionLayer, hitResult, ezPhysicsShapeType::Static))
      continue;

    if (m_pLayer->m_hSurface.IsValid())
    {
      if (!hitResult.m_hSurface.IsValid())
        continue;

      ezResourceLock<ezSurfaceResource> hitSurface(hitResult.m_hSurface, ezResourceAcquireMode::NoFallbackAllowMissing);
      if (hitSurface.GetAcquireResult() == ezResourceAcquireResult::MissingFallback)
        continue;

      if (!hitSurface->IsBasedOn(m_pLayer->m_hSurface))
        continue;
    }

    bool bInBoundingBox = false;
    ezSimdVec4f hitPosition = ezSimdConversion::ToVec3(hitResult.m_vPosition);
    ezSimdVec4f allOne = ezSimdVec4f(1.0f);
    for (auto& globalToLocalBox : m_Desc.m_GlobalToLocalBoxTransforms)
    {
      ezSimdVec4f localHitPosition = globalToLocalBox.TransformPosition(hitPosition).Abs();
      if ((localHitPosition <= allOne).AllSet<3>())
      {
        bInBoundingBox = true;
        break;
      }
    }

    if (bInBoundingBox)
    {
      PlacementPoint& placementPoint = placementTask.m_InputPoints.ExpandAndGetRef();
      placementPoint.m_vPosition = hitResult.m_vPosition;
      placementPoint.m_fScale = 1.0f;
      placementPoint.m_vNormal = hitResult.m_vNormal;
      placementPoint.m_uiColorIndex = 0;
      placementPoint.m_uiObjectIndex = 0;
      placementPoint.m_uiPointIndex = i;
    }
  }

  m_State = State::Scheduled;
}

ezUInt32 ActiveTile::PlaceObjects(ezWorld& world, const PlacementTask& placementTask)
{
  ezGameObjectDesc desc;
  auto& objectsToPlace = m_pLayer->m_ObjectsToPlace;

  ezHybridArray<ezPrefabResource*, 4> prefabs;
  prefabs.SetCount(objectsToPlace.GetCount());

  auto objectTransforms = placementTask.GetOutputTransforms();
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
