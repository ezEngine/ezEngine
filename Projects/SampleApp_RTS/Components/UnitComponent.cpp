#include "UnitComponent.h"
#include <Foundation/Logging/Log.h>
#include <Foundation/Time/Clock.h>
#include <SampleApp_RTS/Level.h>

EZ_IMPLEMENT_COMPONENT_TYPE(UnitComponent, UnitComponentManager);

UnitComponent::UnitComponent()
{
  m_GridCoordinate.Slice = -1; // invalid
}

void UnitComponent::Update()
{
  Level* pLevel = (Level*) GetWorld()->GetUserData();

  GameGrid& Grid = pLevel->GetGrid();

  if (Grid.IsValidCellCoordinate(m_GridCoordinate))
  {
    Grid.GetCell(m_GridCoordinate).m_hUnit.Invalidate();
  }

  MoveAlongPath();

  m_GridCoordinate = Grid.GetCellAtPosition(GetOwner()->GetLocalPosition());

  if (Grid.IsValidCellCoordinate(m_GridCoordinate))
  {
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
    ezVec3 vDir = m_Path.PeekFront() - vCurPos;
    const float fLen = vDir.GetLengthAndNormalize();

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


