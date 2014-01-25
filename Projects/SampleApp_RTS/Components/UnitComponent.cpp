#include "UnitComponent.h"
#include <GameUtils/GridAlgorithms/Rasterization.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Time/Stopwatch.h>
#include <SampleApp_RTS/Level.h>

float UnitComponent::g_fSize = 5.0f;
ezVec3 UnitComponent::g_vTarget(0);

EZ_IMPLEMENT_COMPONENT_TYPE(UnitComponent, UnitComponentManager);

UnitComponent::UnitComponent()
{
  m_GridCoordinate.Slice = -1; // invalid
}

static ezInt32 iVisited = 1;

ezCallbackResult::Enum TagFootprint(ezInt32 x, ezInt32 y, void* pPassThrough)
{
  GameGrid* pGrid = (GameGrid*) pPassThrough;

  if (!pGrid->IsValidCellCoordinate(ezGridCoordinate(x, y)))
    return ezCallbackResult::Stop;

  if (pGrid->GetCell(ezGridCoordinate(x, y)).m_iCellType == 1)
    return ezCallbackResult::Stop;

  pGrid->GetCell(ezGridCoordinate(x, y)).m_bOccupied = 254;//iVisited;
  iVisited += 5;

  return ezCallbackResult::Continue;
}

ezCallbackResult::Enum UntagFootprint(ezInt32 x, ezInt32 y, void* pPassThrough)
{
  GameGrid* pGrid = (GameGrid*) pPassThrough;

  if (!pGrid->IsValidCellCoordinate(ezGridCoordinate(x, y)))
    return ezCallbackResult::Continue;

  pGrid->GetCell(ezGridCoordinate(x, y)).m_bOccupied = false;

  return ezCallbackResult::Continue;
}

void UnitComponent::Update()
{
  Level* pLevel = (Level*) GetWorld()->GetUserData();

  GameGrid& Grid = pLevel->GetGrid();

  if (Grid.IsValidCellCoordinate(m_GridCoordinate))
  {
    Grid.GetCell(m_GridCoordinate).m_hUnit.Invalidate();

    //ez2DGridUtils::RasterizeCircle(m_GridCoordinate.x, m_GridCoordinate.z, g_fSize + 2.0f, UntagFootprint, &Grid);
  }

  MoveAlongPath();

  m_GridCoordinate = Grid.GetCellAtPosition(GetOwner()->GetLocalPosition());

  if (Grid.IsValidCellCoordinate(m_GridCoordinate))
  {
    //ez2DGridUtils::RasterizeCircle(m_GridCoordinate.x, m_GridCoordinate.z, g_fSize, TagFootprint, &Grid);
    iVisited = 1;

    //static ezDynamicArray<ezUInt8> TempArray;

    const ezVec3 vDir = (g_vTarget - GetOwner()->GetLocalPosition()).GetNormalized();

    ezStopwatch s;
    ez2DGridUtils::ComputeVisibleAreaInCone(m_GridCoordinate.x, m_GridCoordinate.z, (ezUInt16) (g_fSize * 5.0f), ezVec2(vDir.x, vDir.z), ezAngle::Degree(45), Grid.GetWidth(), Grid.GetDepth(), TagFootprint, &Grid);//, &TempArray);

    //ezLog::Info("Time Taken: %.2ff microsec", s.Checkpoint().GetMicroseconds());

    Grid.GetCell(m_GridCoordinate).m_hUnit = GetHandle();
  }

}

void UnitComponent::MoveAlongPath()
{
  if (m_Path.IsEmpty())
    return;

  const float fSpeed = 10.5f;
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


