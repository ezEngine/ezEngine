#include <PCH.h>
#include <SampleApp_RTS/Components/AvoidObstacleSteeringComponent.h>
#include <SampleApp_RTS/Level.h>

EZ_IMPLEMENT_COMPONENT_TYPE(AvoidObstacleSteeringComponent, AvoidObstacleSteeringComponentManager);

AvoidObstacleSteeringComponent::AvoidObstacleSteeringComponent()
{
}

struct TagCellData
{
  AvoidObstacleSteeringComponent* m_pComponent;
  Level* m_pLevel;
  GameGrid* m_pGrid;
  ezVec3 m_vCenterPos;
  ezComponentHandle m_hSelf;
};

ezCallbackResult::Enum AvoidObstacleSteeringComponent::ComputeCellDanger(ezInt32 x, ezInt32 y, void* pPassThrough)
{
  TagCellData* tcd = (TagCellData*) pPassThrough;

  if (!tcd->m_pGrid->IsValidCellCoordinate(ezVec2I32(x, y)))
    return ezCallbackResult::Continue;

  const ezVec3 vCellPos = tcd->m_pGrid->GetCellWorldSpaceCenter(ezVec2I32(x, y));
  const ezVec3 vDir = vCellPos - tcd->m_vCenterPos;
  const float fLength = vDir.GetLength();
  const ezVec3 vDirNorm = vDir.GetNormalized();

  float fDanger = 0;

  if (tcd->m_pGrid->GetCell(ezVec2I32(x, y)).m_iCellType == 1)
    fDanger = ezMath::Max(0.0f, 2.0f - fLength);
  else
  {
    ezComponentHandle hUnit = tcd->m_pGrid->GetCell(ezVec2I32(x, y)).m_hUnit;
    if (!hUnit.IsInvalidated() && hUnit != tcd->m_hSelf)
      fDanger = ezMath::Max(0.0f, 3.0f - fLength);
  }


  for (ezInt32 i = 0; i < g_iSteeringDirections; ++i)
  {
    tcd->m_pComponent->m_fDirectionDanger[i] = ezMath::Max(SteeringBehaviorComponent::g_vSteeringDirections[i].Dot(vDirNorm) * fDanger, tcd->m_pComponent->m_fDirectionDanger[i]);

    if (fDanger > 0.1f)
      tcd->m_pComponent->m_fDirectionDesire[i] = ezMath::Max(-SteeringBehaviorComponent::g_vSteeringDirections[i].Dot(vDirNorm) * 0.1f, tcd->m_pComponent->m_fDirectionDesire[i]);
  }

  return ezCallbackResult::Continue;
}

void AvoidObstacleSteeringComponent::Update()
{
  for (ezInt32 i = 0; i < g_iSteeringDirections; ++i)
  {
    m_fDirectionDesire[i] = 0;
    m_fDirectionDanger[i] = 0;
  }

  UnitComponent* pUnit;
  if (!GetOwner()->TryGetComponentOfType<UnitComponent>(pUnit))
    return;

  TagCellData tcd;
  tcd.m_pComponent = this;
  tcd.m_pLevel = (Level*) GetWorld()->GetUserData();
  tcd.m_pGrid = &tcd.m_pLevel->GetGrid();
  tcd.m_vCenterPos = GetOwner()->GetLocalPosition();
  tcd.m_hSelf = pUnit->GetHandle();

  const ezVec2I32 vPos = tcd.m_pGrid->GetCellAtWorldPosition(GetOwner()->GetLocalPosition());

  ez2DGridUtils::RasterizeCircle(vPos.x, vPos.y, 3.0f, ComputeCellDanger, &tcd);
}

