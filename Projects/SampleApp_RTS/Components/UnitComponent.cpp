#include <PCH.h>
#include <SampleApp_RTS/Components/UnitComponent.h>
#include <GameUtils/GridAlgorithms/Rasterization.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Time/Stopwatch.h>
#include <SampleApp_RTS/Level.h>

EZ_IMPLEMENT_COMPONENT_TYPE(UnitComponent, UnitComponentManager);

ezCVarFloat CVarUnitSpeed("ai_UnitSpeed", 10.0f, ezCVarFlags::Save, "How fast units move.");

UnitComponent::UnitComponent()
{
  m_GridCoordinate.Set(-1); // invalid
  m_iCurDirection = -1;
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
  //  return;

  ezHybridArray<SteeringBehaviorComponent*, 8> Steering;

  FollowPathSteeringComponent* pFollowSB;
  if (GetOwner()->TryGetComponentOfType<FollowPathSteeringComponent>(pFollowSB))
    Steering.PushBack(pFollowSB);

  AvoidObstacleSteeringComponent* pAvoidSB;
  if (GetOwner()->TryGetComponentOfType<AvoidObstacleSteeringComponent>(pAvoidSB))
    Steering.PushBack(pAvoidSB);

  ezVec3 vDesiredDir(0);

  float fDirectionDesire[SteeringBehaviorComponent::g_iSteeringDirections];
  float fDirectionWhisker[SteeringBehaviorComponent::g_iSteeringDirections];

  for (ezInt32 dd = 0; dd < SteeringBehaviorComponent::g_iSteeringDirections; ++dd)
  {
    fDirectionDesire[dd]  = 0.0f;
    fDirectionWhisker[dd] = 10.0f;
  }

  const ezInt32 iPrevDir = m_iCurDirection;

  m_iCurDirection = -1;

  if (Steering.IsEmpty())
    return;

  for (ezUInt32 i = 0; i < Steering.GetCount(); ++i)
  {
    for (ezInt32 dd = 0; dd < SteeringBehaviorComponent::g_iSteeringDirections; ++dd)
    {
      fDirectionDesire[dd]  = ezMath::Max(fDirectionDesire[dd],  Steering[i]->GetDirectionDesire()[dd]);
      fDirectionWhisker[dd] = ezMath::Min(fDirectionWhisker[dd], Steering[i]->GetDirectionWhisker()[dd]);
    }
  }

  if (iPrevDir != -1)
  {
    const ezInt32 n = SteeringBehaviorComponent::g_iSteeringDirections;

    fDirectionDesire[iPrevDir] *= 1.5f;
    fDirectionDesire[(iPrevDir + 1) % n] *= 1.3f;
    fDirectionDesire[(iPrevDir + n - 1) % n] *= 1.1f;
    fDirectionDesire[(iPrevDir + 2) % n] *= 1.3f;
    fDirectionDesire[(iPrevDir + n - 2) % n] *= 1.1f;
  }

  for (ezInt32 dd = 0; dd < SteeringBehaviorComponent::g_iSteeringDirections; ++dd)
    fDirectionDesire[dd] = ezMath::Min(fDirectionDesire[dd], fDirectionWhisker[dd]);

  
  float fDesire = 0;

  for (ezInt32 dd = 0; dd < SteeringBehaviorComponent::g_iSteeringDirections; ++dd)
  {
    if (fDirectionDesire[dd] > fDesire)
    {
      m_iCurDirection = dd;
      fDesire = fDirectionDesire[dd];
    }
  }

  if (m_iCurDirection == -1)
    return;

  vDesiredDir = SteeringBehaviorComponent::g_vSteeringDirections[m_iCurDirection] * fDesire;

  const float fSpeed = CVarUnitSpeed;
  float fDistance = fSpeed * (float) ezClock::Get()->GetTimeDiff().GetSeconds();

  ezVec3 vCurPos = GetOwner()->GetLocalPosition();

  if (fDistance > vDesiredDir.GetLength())
  {
    if (!m_Path.IsEmpty() && (vCurPos - m_Path.PeekBack()).GetLengthSquared() < 0.1f)
      m_Path.Clear();
  }
  else
  {
    vDesiredDir.SetLength(fDistance);
  }

  vCurPos += vDesiredDir;
  GetOwner()->SetLocalPosition(vCurPos);
}


