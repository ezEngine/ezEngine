#include <PCH.h>
#include <ProceduralPlacementPlugin/Components/Implementation/ActiveTile.h>
#include <GameEngine/Interfaces/PhysicsWorldModule.h>
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

  m_pPlacementTask = std::move(other.m_pPlacementTask);

  m_State = other.m_State;
}

ActiveTile::~ActiveTile()
{
  Deinitialize();
}

void ActiveTile::Initialize(const TileDesc& desc, const Layer* pLayer)
{
  m_Desc = desc;
  m_pLayer = pLayer;

  if (m_pPlacementTask == nullptr)
  {
    m_pPlacementTask = EZ_DEFAULT_NEW(PlacementTask);
  }

  m_pPlacementTask->m_pLayer = pLayer;
  m_pPlacementTask->m_InputPoints.Clear();
  m_pPlacementTask->m_OutputTransforms.Clear();

  m_State = State::Initialized;
}

void ActiveTile::Deinitialize()
{
  if (m_State == State::Scheduled)
  {
    ezTaskSystem::WaitForGroup(m_TaskGroupId);
  }

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

ezArrayPtr<const PlacementTransform> ActiveTile::GetObjectTransforms() const
{
  return m_pPlacementTask->GetOutputTransforms();
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

void ActiveTile::Update(ezPhysicsWorldModuleInterface* pPhysicsModule)
{
  if (m_State == State::Initialized)
  {
    if (pPhysicsModule == nullptr)
      return;

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
        PlacementPoint& placementPoint = m_pPlacementTask->m_InputPoints.ExpandAndGetRef();
        placementPoint.m_vPosition = hitResult.m_vPosition;
        placementPoint.m_fScale = 1.0f;
        placementPoint.m_vNormal = hitResult.m_vNormal;
        placementPoint.m_uiColorIndex = 0;
        placementPoint.m_uiObjectIndex = 0;
        placementPoint.m_uiPointIndex = i;
      }
    }

    m_TaskGroupId = ezTaskSystem::StartSingleTask(m_pPlacementTask.Borrow(), ezTaskPriority::LongRunningHighPriority);

    m_State = State::Scheduled;
  }
  else if (m_State == State::Scheduled)
  {
    if (m_pPlacementTask->IsTaskFinished())
    {
      m_State = State::Finished;
    }
  }
}

bool ActiveTile::IsFinished() const
{
  return m_State == State::Finished;
}
