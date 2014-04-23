#include <PCH.h>
#include <RTS/General/Application.h>

#include <Foundation/Time/Stopwatch.h>
#include <GameUtils/PathFinding/GraphSearch.h>

ezCVarBool CVarLocalSteeringOnly("ai_LocalSteeringOnly", false, ezCVarFlags::Save, "Ignore global path finding and use local steering only.");

struct MyPathState : ezPathStateBase
{
  EZ_DECLARE_POD_TYPE();

};

class ezNavmeshPathStateGenerator : public ezPathStateGenerator<MyPathState>
{
public:
  ezGridNavmesh* m_pNavmesh;

  ezVec2 m_vTarget;
  
  virtual void GenerateAdjacentStates(ezInt64 iCurArea, const MyPathState& StartState, ezPathSearch<MyPathState>* pPathSearch) override
  {
    const ezGridNavmesh::ConvexArea& Area = m_pNavmesh->GetConvexArea((ezInt32) iCurArea);
    const ezVec2 vAreaCenter (Area.m_Rect.x + Area.m_Rect.width * 0.5f, Area.m_Rect.y + Area.m_Rect.height * 0.5f);

    for (ezUInt32 e = 0; e < Area.m_uiNumEdges; ++e)
    {
      const ezGridNavmesh::AreaEdge& Edge = m_pNavmesh->GetAreaEdge(Area.m_uiFirstEdge + e);
      const ezGridNavmesh::ConvexArea& NextArea = m_pNavmesh->GetConvexArea(Edge.m_iNeighborArea);

      const ezVec2 vNextAreaCenter (NextArea.m_Rect.x + NextArea.m_Rect.width * 0.5f, NextArea.m_Rect.y + NextArea.m_Rect.height * 0.5f);

      MyPathState NextState;
      NextState.m_fCostToNode = StartState.m_fCostToNode + (vNextAreaCenter - vAreaCenter).GetLength();
      NextState.m_fEstimatedCostToTarget = NextState.m_fCostToNode + (m_vTarget - vNextAreaCenter).GetLength();

      const ezInt64 iNextNodeIndex = Edge.m_iNeighborArea;

      pPathSearch->AddPathNode(iNextNodeIndex, NextState);
    }
  }

};

ezPathSearch<MyPathState> PathSearch;

void SampleGameApp::SendUnit()
{
  const ezVec2I32 iCell = GetPickedGridCell();

  if (!m_pLevel->GetGrid().IsValidCellCoordinate(iCell))
    return;

  const ezVec3 vTarget = m_pLevel->GetGrid().GetCellWorldSpaceCenter(iCell);

  ezNavmeshPathStateGenerator NavmeshStateGenerator;
  NavmeshStateGenerator.m_pNavmesh = &m_pLevel->GetNavmesh();
  NavmeshStateGenerator.m_vTarget = ezVec2(vTarget.x, vTarget.z);

  PathSearch.SetPathStateGenerator(&NavmeshStateGenerator);

  for (ezUInt32 i = 0; i < m_pSelectedUnits->GetCount();++i)
  {
    ezGameObjectHandle hObject = m_pSelectedUnits->GetObject(i);

    ezGameObject* pObject;
    if (m_pLevel->GetWorld()->TryGetObject(hObject, pObject))
    {
      const ezVec3 vPos = pObject->GetLocalPosition();

      UnitComponent* pUnit;
      if (pObject->TryGetComponentOfBaseType<UnitComponent>(pUnit))
      {
        const ezVec2I32 StartCoord  = m_pLevel->GetGrid().GetCellAtWorldPosition(vPos);
        const ezVec2I32 TargetCoord = m_pLevel->GetGrid().GetCellAtWorldPosition(vTarget);
        const ezInt64 iTargetArea = m_pLevel->GetNavmesh().GetAreaAt(TargetCoord);
        const ezInt64 iStartArea = m_pLevel->GetNavmesh().GetAreaAt(StartCoord);

        MyPathState StartState;
        ezDeque<ezPathSearch<MyPathState>::PathResultData> PathNodeIndices;

        ezStopwatch s;

        if (iStartArea >= 0 && iTargetArea >= 0 && PathSearch.FindPath(iStartArea, StartState, iTargetArea, PathNodeIndices) == EZ_SUCCESS)
        {
          ezLog::Info("Found Path: %.2fms", s.Checkpoint().GetMilliseconds());

          s.Checkpoint();

          pUnit->m_Path.Clear();

          if (!CVarLocalSteeringOnly)
          {
            pUnit->m_Path.Reserve(PathNodeIndices.GetCount());

            for (ezInt32 i = 1; i < (ezInt32) PathNodeIndices.GetCount() - 1; ++i)
            {
              const ezGridNavmesh::ConvexArea& Area = m_pLevel->GetNavmesh().GetConvexArea((ezInt32) PathNodeIndices[i].m_iNodeIndex);
              const ezVec2 vAreaCenter (Area.m_Rect.x + Area.m_Rect.width * 0.5f, Area.m_Rect.y + Area.m_Rect.height * 0.5f);
              const ezVec2I32 AreaCoord((ezInt32) vAreaCenter.x, (ezInt32) vAreaCenter.y);

              pUnit->m_Path.PushBack(m_pLevel->GetGrid().GetCellWorldSpaceCenter(AreaCoord));
            }
          }

          pUnit->m_Path.PushBack(vTarget);
        }
        else
          ezLog::Info("No Path found: %.2fms", s.Checkpoint().GetMilliseconds());
      }
    }
  }
}



