#include <PCH.h>
#include "UnitComponent.h"
#include <GameUtils/GridAlgorithms/Rasterization.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Time/Stopwatch.h>
#include <SampleApp_RTS/Level.h>

EZ_IMPLEMENT_COMPONENT_TYPE(UnitComponent, UnitComponentManager);

UnitComponent::UnitComponent()
{
  m_GridCoordinate.Set(-1); // invalid
}

void UnitComponent::Update()
{
  Level* pLevel = (Level*) GetWorld()->GetUserData();

  GameGrid& Grid = pLevel->GetGrid();

  // deregister the unit from the cell that it is currently on
  if (Grid.IsValidCellCoordinate(m_GridCoordinate))
    Grid.GetCell(m_GridCoordinate).m_hUnit.Invalidate();

  MoveAlongPath();

  m_GridCoordinate = Grid.GetCellAtWorldPosition(GetOwner()->GetLocalPosition());

  if (Grid.IsValidCellCoordinate(m_GridCoordinate))
  {
    // register it at the new cell
    Grid.GetCell(m_GridCoordinate).m_hUnit = GetHandle();
  }

}

void UnitComponent::MoveAlongPath()
{
  //if (m_Path.IsEmpty())
    //return;

  ezHybridArray<SteeringBehaviorComponent*, 8> Steering;

  FollowPathSteeringComponent* pFollowSB;
  if (GetOwner()->TryGetComponentOfType<FollowPathSteeringComponent>(pFollowSB))
    Steering.PushBack(pFollowSB);

  AvoidObstacleSteeringComponent* pAvoidSB;
  if (GetOwner()->TryGetComponentOfType<AvoidObstacleSteeringComponent>(pAvoidSB))
    Steering.PushBack(pAvoidSB);

  ezVec3 vDesiredDir(0);

  float fDirectionDesire[SteeringBehaviorComponent::g_iSteeringDirections] = { 0 };
  float fDirectionDanger[SteeringBehaviorComponent::g_iSteeringDirections] = { 0 };

  if (Steering.IsEmpty())
    return;

  for (ezUInt32 i = 0; i < Steering.GetCount(); ++i)
  {
    for (ezInt32 dd = 0; dd < SteeringBehaviorComponent::g_iSteeringDirections; ++dd)
    {
      fDirectionDesire[dd] = ezMath::Max(fDirectionDesire[dd], Steering[i]->GetDirectionDesire()[dd]);
      fDirectionDanger[dd] = ezMath::Max(fDirectionDanger[dd], Steering[i]->GetDirectionDanger()[dd]);
    }
  }

  float fMinDanger = 1000;

  for (ezInt32 dd = 0; dd < SteeringBehaviorComponent::g_iSteeringDirections; ++dd)
    fMinDanger = ezMath::Min(fMinDanger, ezMath::Max(0.0f, fDirectionDanger[dd]));

  ezInt32 iDesiredDir = -1;
  float fDesire = 0;

  for (ezInt32 dd = 0; dd < SteeringBehaviorComponent::g_iSteeringDirections; ++dd)
  {
    if (fDirectionDanger[dd] <= fMinDanger && fDirectionDesire[dd] > fDesire)
    {
      iDesiredDir = dd;
      fDesire = fDirectionDesire[dd];
    }
  }

  if (iDesiredDir == -1)
    return;

  vDesiredDir = SteeringBehaviorComponent::g_vSteeringDirections[iDesiredDir] * fDesire;

  const float fSpeed = 10.0f;
  float fDistance = fSpeed * (float) ezClock::Get()->GetTimeDiff().GetSeconds();

  ezVec3 vCurPos = GetOwner()->GetLocalPosition();

  if (fDistance > vDesiredDir.GetLength())
  {
    vCurPos += vDesiredDir;

    if (!m_Path.IsEmpty() && (vCurPos - m_Path.PeekBack()).GetLengthSquared() < 0.1f)
      m_Path.Clear();
  }
  else
  {
    vDesiredDir.SetLength(fDistance);

    vCurPos += vDesiredDir;
  }

  GetOwner()->SetLocalPosition(vCurPos);
}


