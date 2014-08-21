#include <PCH.h>
#include <RTS/Components/AvoidObstacleSteeringComponent.h>
#include <RTS/Level.h>

EZ_BEGIN_COMPONENT_TYPE(AvoidObstacleSteeringComponent, SteeringBehaviorComponent, 1, AvoidObstacleSteeringComponentManager);
EZ_END_COMPONENT_TYPE();

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
  ezInt32 m_iThreat;
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

  if (fLength >= 5.0f)
    return ezCallbackResult::Continue;

  bool bDanger = false;

  const bool bThreat = (tcd->m_pGrid->GetCell(ezVec2I32(x, y)).m_iThreat != tcd->m_iThreat) && (tcd->m_pGrid->GetCell(ezVec2I32(x, y)).m_iThreat != 0);

  if (tcd->m_pGrid->GetCell(ezVec2I32(x, y)).m_iCellType == 1 || bThreat)
  {
    ezBoundingBox bb(vCellPos - tcd->m_pGrid->GetWorldSpaceCellSize() * 0.5f, vCellPos + tcd->m_pGrid->GetWorldSpaceCellSize() * 0.5f);

    for (ezInt32 i = 0; i < g_iSteeringDirections; ++i)
    {
      float fIntersection = 0.0f;
      if (bb.GetRayIntersection(tcd->m_vCenterPos + SteeringBehaviorComponent::g_vSteeringDirections[i] * 0.5f, SteeringBehaviorComponent::g_vSteeringDirections[i], &fIntersection))
      {
        if (tcd->m_pGrid->GetCell(ezVec2I32(x, y)).m_iCellType == 1)
          tcd->m_pComponent->m_fDirectionWhisker[i] = ezMath::Min(tcd->m_pComponent->m_fDirectionWhisker[i], fIntersection);

        if (bThreat)
          tcd->m_pComponent->m_fDirectionDesire[i] = ezMath::Min(tcd->m_pComponent->m_fDirectionDesire[i], fIntersection);
      }
    }
  }
  
  {
    ezComponentHandle hUnit = tcd->m_pGrid->GetCell(ezVec2I32(x, y)).m_hUnit;
    if (!hUnit.IsInvalidated() && hUnit != tcd->m_hSelf)
    {
      UnitComponent* pUnit;
      if (tcd->m_pLevel->GetWorld()->TryGetComponent<UnitComponent>(hUnit, pUnit))
      {
        ObstacleComponent* pObstacle;
        if (pUnit->GetOwner()->TryGetComponentOfBaseType<ObstacleComponent>(pObstacle))
        {
          const float fRadius = pObstacle->m_fRadius > 0 ? pObstacle->m_fRadius : ObstacleComponent::g_fDefaultRadius;

          ezBoundingSphere sphere(pObstacle->GetOwner()->GetLocalPosition(), fRadius);

          for (ezInt32 i = 0; i < g_iSteeringDirections; ++i)
          {
            float fIntersection = 0.0f;

            ezVec3 vStartPos = tcd->m_vCenterPos + SteeringBehaviorComponent::g_vSteeringDirections[i] * 0.5f;

            if (sphere.Contains(vStartPos) || sphere.GetRayIntersection(vStartPos, SteeringBehaviorComponent::g_vSteeringDirections[i], &fIntersection))
            {
              tcd->m_pComponent->m_fDirectionWhisker[i] = ezMath::Min(tcd->m_pComponent->m_fDirectionWhisker[i], fIntersection);
            }
          }
        }
      }
    }
  }

  return ezCallbackResult::Continue;
}

void AvoidObstacleSteeringComponent::Update()
{
  for (ezInt32 i = 0; i < g_iSteeringDirections; ++i)
  {
    m_fDirectionDesire[i] = 0;
    m_fDirectionWhisker[i] = 5.0f;
  }

  UnitComponent* pUnit;
  if (!GetOwner()->TryGetComponentOfBaseType<UnitComponent>(pUnit))
    return;

  

  TagCellData tcd;
  tcd.m_pComponent = this;
  tcd.m_pLevel = (Level*) GetWorld()->GetUserData();
  tcd.m_pGrid = &tcd.m_pLevel->GetGrid();
  tcd.m_iThreat = pUnit->m_iThreat;

  const ezVec2I32 iPos = tcd.m_pGrid->GetCellAtWorldPosition(GetOwner()->GetLocalPosition());

  tcd.m_vCenterPos = GetOwner()->GetLocalPosition();//tcd.m_pGrid->GetCellWorldSpaceCenter(vPos);
  tcd.m_hSelf = pUnit->GetHandle();

  if ((pUnit->m_Path.IsEmpty()) &&
      (tcd.m_pGrid->GetCell(iPos).m_iThreat != 0) &&
      (tcd.m_pGrid->GetCell(iPos).m_iThreat != pUnit->m_iThreat))
  {
    for (ezInt32 i = 0; i < g_iSteeringDirections; ++i)
    {
      m_fDirectionDesire[i] = 5.0f;
    }
  }

  ez2DGridUtils::RasterizeCircle(iPos.x, iPos.y, 5.0f, ComputeCellDanger, &tcd);
}

