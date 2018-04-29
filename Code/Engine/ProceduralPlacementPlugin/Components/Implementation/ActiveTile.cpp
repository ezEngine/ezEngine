#include <PCH.h>
#include <ProceduralPlacementPlugin/Components/Implementation/ActiveTile.h>
#include <GameEngine/Components/PrefabReferenceComponent.h>
#include <GameEngine/Interfaces/PhysicsWorldModule.h>
#include <RendererCore/Messages/SetColorMessage.h>
#include <Core/World/World.h>
#include <Foundation/SimdMath/SimdConversion.h>

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

  m_Desc.m_uiResourceIdHash = 0;
  m_pLayer = nullptr;
  m_State = State::Invalid;
}

bool ActiveTile::IsValid() const
{
  return m_Desc.m_uiResourceIdHash != 0 && m_pLayer != nullptr;
}

const TileDesc& ActiveTile::GetDesc() const
{
  return m_Desc;
}

const Layer* ActiveTile::GetLayer() const
{
  return m_pLayer;
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

  ezBoundingBox bbox = GetBoundingBox();
  float fZStart = bbox.m_vMax.z;
  float fZRange = bbox.GetExtents().z;
  ezVec2 vXY = bbox.m_vMin.GetAsVec2();

  ezVec3 rayDir = ezVec3(0, 0, -1);

  auto& patternPoints = m_pLayer->m_pPattern->m_Points;

  for (ezUInt32 i = 0; i < patternPoints.GetCount(); ++i)
  {
    auto& patternPoint = patternPoints[i];

    ezVec3 rayStart = (vXY + patternPoint.m_Coordinates * m_pLayer->m_fFootprint).GetAsVec3(fZStart);

    ezPhysicsHitResult hitResult;
    if (!pPhysicsModule->CastRay(rayStart, rayDir, fZRange, 0, hitResult, false))
      continue;

    bool bInBoundingBox = false;
    ezSimdVec4f hitPosition = ezSimdConversion::ToVec3(hitResult.m_vPosition);
    ezSimdVec4f allOne = ezSimdVec4f(1.0f);
    for (auto& boundingBox : m_Desc.m_LocalBoundingBoxes)
    {
      ezSimdVec4f localHitPosition = boundingBox.TransformPosition(hitPosition).Abs();
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

ezUInt32 ezPPInternal::ActiveTile::PlaceObjects(ezWorld& world, const PlacementTask& placementTask)
{
  ezGameObjectDesc desc;
  auto& objectsToPlace = m_pLayer->m_ObjectsToPlace;

  auto objectTransforms = placementTask.GetOutputTransforms();
  for (auto& objectTransform : objectTransforms)
  {
    desc.m_LocalPosition = ezSimdConversion::ToVec3(objectTransform.m_Transform.m_Position);
    desc.m_LocalRotation = ezSimdConversion::ToQuat(objectTransform.m_Transform.m_Rotation);
    desc.m_LocalScaling = ezSimdConversion::ToVec3(objectTransform.m_Transform.m_Scale);

    ezGameObject* pObject = nullptr;
    ezGameObjectHandle hObject = world.CreateObject(desc, pObject);

    //pObject->GetTags().Set(tag);

    ezPrefabReferenceComponent* pPrefabReferenceComponent = nullptr;
    ezPrefabReferenceComponent::CreateComponent(pObject, pPrefabReferenceComponent);

    auto& objectToPlace = objectsToPlace[objectTransform.m_uiObjectIndex];
    pPrefabReferenceComponent->SetPrefabFile(objectToPlace);

    // Set the color
    ezMsgSetColor msg;
    msg.m_Color = objectTransform.m_Color;
    pObject->PostMessageRecursive(msg, ezObjectMsgQueueType::AfterInitialized);

    m_PlacedObjects.PushBack(hObject);
  }

  m_State = State::Finished;

  return m_PlacedObjects.GetCount();
}
