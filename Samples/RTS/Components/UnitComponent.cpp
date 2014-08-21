#include <PCH.h>
#include <RTS/Components/UnitComponent.h>
#include <GameUtils/GridAlgorithms/Rasterization.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Time/Stopwatch.h>
#include <RTS/Level.h>

EZ_BEGIN_COMPONENT_TYPE(UnitComponent, ezComponent, 1, UnitComponentManager);
EZ_END_COMPONENT_TYPE();

ezCVarFloat CVarUnitSpeed("ai_UnitSpeed", 10.0f, ezCVarFlags::Save, "How fast units move.");

static ezInt32 g_iThreat = 1;

UnitComponent::UnitComponent()
{
  m_GridCoordinate.Set(-1); // invalid
  m_iCurDirection = -1;
  m_fSpeedBias = 0.0f;
  m_iThreat = g_iThreat++;
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

struct ThreatCellData
{
  Level* m_pLevel;
  GameGrid* m_pGrid;
  ezInt32 m_iThreat;
};

ezCallbackResult::Enum UnitComponent::TagCellThreat(ezInt32 x, ezInt32 y, void* pPassThrough)
{
  ThreatCellData* tcd = (ThreatCellData*) pPassThrough;

  if (!tcd->m_pGrid->IsValidCellCoordinate(ezVec2I32(x, y)))
    return ezCallbackResult::Stop;

  tcd->m_pGrid->GetCell(ezVec2I32(x, y)).m_iThreat = ezMath::Max(tcd->m_pGrid->GetCell(ezVec2I32(x, y)).m_iThreat, tcd->m_iThreat);

  if (tcd->m_pGrid->GetCell(ezVec2I32(x, y)).m_iCellType == 1)
    return ezCallbackResult::Stop;

  return ezCallbackResult::Continue;
}


void UnitComponent::MoveAlongPath()
{
  //if (m_Path.IsEmpty())
  //  return;

  ezHybridArray<SteeringBehaviorComponent*, 8> Steering;

  FollowPathSteeringComponent* pFollowSB;
  if (GetOwner()->TryGetComponentOfBaseType<FollowPathSteeringComponent>(pFollowSB))
    Steering.PushBack(pFollowSB);

  AvoidObstacleSteeringComponent* pAvoidSB;
  if (GetOwner()->TryGetComponentOfBaseType<AvoidObstacleSteeringComponent>(pAvoidSB))
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
  ezVec3 vCurPos = GetOwner()->GetLocalPosition();

  Level* pLevel = (Level*) GetWorld()->GetUserData();
  GameGrid* pGrid = &pLevel->GetGrid();
  const ezVec2I32 iPos = pGrid->GetCellAtWorldPosition(vCurPos);

  if (iPrevDir != -1)
  {
    const ezInt32 n = SteeringBehaviorComponent::g_iSteeringDirections;

    fDirectionDesire[iPrevDir] *= 1.5f;
    fDirectionDesire[(iPrevDir + 1) % n] *= 1.3f;
    fDirectionDesire[(iPrevDir + n - 1) % n] *= 1.1f;
    fDirectionDesire[(iPrevDir + 2) % n] *= 1.3f;
    fDirectionDesire[(iPrevDir + n - 2) % n] *= 1.1f;

    const ezVec3 vDir = SteeringBehaviorComponent::g_vSteeringDirections[iPrevDir];

    ThreatCellData tcd;
    tcd.m_pLevel = (Level*) GetWorld()->GetUserData();
    tcd.m_pGrid = &tcd.m_pLevel->GetGrid();
    tcd.m_iThreat = m_iThreat;

    const ezInt32 iPrevThreat = pGrid->GetCell(iPos).m_iThreat;

    ez2DGridUtils::ComputeVisibleAreaInCone(iPos.x, iPos.y, 4, ezVec2(vDir.x, vDir.z), ezAngle::Degree(30), 
      pGrid->GetGridWidth(), pGrid->GetGridHeight(), TagCellThreat, &tcd);

    pGrid->GetCell(iPos).m_iThreat = iPrevThreat;
  }

  for (ezInt32 dd = 0; dd < SteeringBehaviorComponent::g_iSteeringDirections; ++dd)
    fDirectionDesire[dd] = ezMath::Min(fDirectionDesire[dd], fDirectionWhisker[dd]);

  
  float fDesire = 0;

  const ezInt32 iOffset = 0;//rand();
  for (ezInt32 dd = 0; dd < SteeringBehaviorComponent::g_iSteeringDirections; ++dd)
  {
    const ezInt32 index = (dd + iOffset) % SteeringBehaviorComponent::g_iSteeringDirections;

    if (fDirectionDesire[index] > fDesire)
    {
      m_iCurDirection = index;
      fDesire = fDirectionDesire[index];
    }
  }

  if (m_iCurDirection == -1)
  {
    if (!m_Path.IsEmpty())
      m_iCurDirection = iPrevDir;

    return;
  }

  vDesiredDir = SteeringBehaviorComponent::g_vSteeringDirections[m_iCurDirection] * fDesire;

  const float fSpeed = CVarUnitSpeed + m_fSpeedBias;
  float fDistance = fSpeed * (float) ezClock::Get()->GetTimeDiff().GetSeconds();

  

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


