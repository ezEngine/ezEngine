#include <PCH.h>
#include "UnitComponent.h"
#include <GameUtils/GridAlgorithms/Rasterization.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Time/Stopwatch.h>
#include <SampleApp_RTS/Level.h>

float UnitComponent::g_fVisConeRadius = 10.0f;
ezVec3 UnitComponent::g_vUserPointsAt(0);

EZ_IMPLEMENT_COMPONENT_TYPE(UnitComponent, UnitComponentManager);

UnitComponent::UnitComponent()
{
  m_GridCoordinate.Set(-1); // invalid
}

ezCallbackResult::Enum TagCellVisible(ezInt32 x, ezInt32 y, void* pPassThrough)
{
  GameGrid* pGrid = (GameGrid*) pPassThrough;

  if (!pGrid->IsValidCellCoordinate(ezVec2I32(x, y)))
    return ezCallbackResult::Stop;

  pGrid->GetCell(ezVec2I32(x, y)).m_uiVisibility = 200;

  if (pGrid->GetCell(ezVec2I32(x, y)).m_iCellType == 1)
    return ezCallbackResult::Stop;

  return ezCallbackResult::Continue;
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
    const ezVec3 vDir = (g_vUserPointsAt - GetOwner()->GetLocalPosition()).GetNormalized();

    // update what we see
    ez2DGridUtils::ComputeVisibleArea(m_GridCoordinate.x, m_GridCoordinate.y, (ezUInt16) (g_fVisConeRadius), Grid.GetGridWidth(), Grid.GetGridHeight(), TagCellVisible, &Grid);

    // register it at the new cell
    Grid.GetCell(m_GridCoordinate).m_hUnit = GetHandle();
  }

}

void UnitComponent::MoveAlongPath()
{
  if (m_Path.IsEmpty())
    return;

  const float fSpeed = 50.0f;
  float fDistance = fSpeed * (float) ezClock::Get()->GetTimeDiff().GetSeconds();

  ezVec3 vCurPos = GetOwner()->GetLocalPosition();

  while (fDistance > 0.0f && !m_Path.IsEmpty())
  {
    ezVec3 vPathPos = m_Path.PeekFront();

    float fLen = 0.0f;
    ezVec3 vDir = vPathPos - vCurPos;

    if (vPathPos != vCurPos)
    {
      fLen = vDir.GetLengthAndNormalize();
    }

    if (fLen <= fDistance)
    {
      vCurPos = m_Path.PeekFront();
      m_Path.PopFront();

      fDistance -= fLen;
    }
    else
    {
      vCurPos += vDir * fDistance;
      break;
    }
  }

  GetOwner()->SetLocalPosition(vCurPos);
}


