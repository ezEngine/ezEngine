#include <SampleApp_RTS/General/Application.h>
#include <SampleApp_RTS/General/Window.h>

#include <Foundation/Logging/Log.h>
#include <Core/Input/InputManager.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Time/Stopwatch.h>
#include <GameUtils/GridAlgorithms/Rasterization.h>
#include <GameUtils/PathFinding/GraphSearch.h>

void SampleGameApp::UpdateInput(ezTime UpdateDiff)
{
  ezInputManager::Update(UpdateDiff);

  if (ezInputManager::GetInputActionState("Main", "CloseApp") == ezKeyState::Pressed)
    m_bActiveRenderLoop = false;

  float f;
  float fCamSpeed = 15.0f;
  const float fCamRotSpeed = 140.0f;

  if (ezInputManager::GetInputActionState("Game", "CamMoveFast", &f) != ezKeyState::Up)
    fCamSpeed *= 3;

  if (ezInputManager::GetInputActionState("Game", "CamRotateLeft", &f) != ezKeyState::Up)
    m_Camera.RotateGlobally(ezAngle(), ezAngle::Degree(+f * fCamRotSpeed), ezAngle());

  if (ezInputManager::GetInputActionState("Game", "CamRotateRight", &f) != ezKeyState::Up)
    m_Camera.RotateGlobally(ezAngle(), ezAngle::Degree(-f * fCamRotSpeed), ezAngle());

  if (ezInputManager::GetInputActionState("Game", "CamRotateUp", &f) != ezKeyState::Up)
    m_Camera.RotateLocally(ezAngle::Degree(+f * fCamRotSpeed), ezAngle(), ezAngle());

  if (ezInputManager::GetInputActionState("Game", "CamRotateDown", &f) != ezKeyState::Up)
    m_Camera.RotateLocally(ezAngle::Degree(-f * fCamRotSpeed), ezAngle(), ezAngle());

  if (ezInputManager::GetInputActionState("Game", "CamMoveLeft", &f) != ezKeyState::Up)
    m_Camera.MoveLocally(ezVec3(-f * fCamSpeed, 0, 0));

  if (ezInputManager::GetInputActionState("Game", "CamMoveRight", &f) != ezKeyState::Up)
    m_Camera.MoveLocally(ezVec3(+f * fCamSpeed, 0, 0));

  if (ezInputManager::GetInputActionState("Game", "CamMoveUp", &f) != ezKeyState::Up)
    m_Camera.MoveGlobally(ezVec3(0, +f * fCamSpeed, 0));

  if (ezInputManager::GetInputActionState("Game", "CamMoveDown", &f) != ezKeyState::Up)
    m_Camera.MoveGlobally(ezVec3(0, -f * fCamSpeed, 0));

  if (ezInputManager::GetInputActionState("Game", "CamMoveForwards", &f) != ezKeyState::Up)
    m_Camera.MoveLocally(ezVec3(0, 0, -f * fCamSpeed));

  if (ezInputManager::GetInputActionState("Game", "CamMoveBackwards", &f) != ezKeyState::Up)
    m_Camera.MoveLocally(ezVec3(0, 0, +f * fCamSpeed));

  if (ezInputManager::GetInputActionState("Game", "SelectUnit", &f) == ezKeyState::Pressed)
    SelectUnit();

  if (ezInputManager::GetInputActionState("Game", "SendUnit", &f) == ezKeyState::Pressed)
    SendUnit();

  if (ezInputManager::GetInputActionState("Game", "UnitLarger", &f) == ezKeyState::Pressed)
    UnitComponent::g_fSize += 0.2f;

  if (ezInputManager::GetInputActionState("Game", "UnitSmaller", &f) == ezKeyState::Pressed)
    UnitComponent::g_fSize -= 0.2f;


  ezVec3 vTargetPos;
  ezVec2I32 iCell = GetPickedGridCell(&vTargetPos);

  if (m_pLevel->GetGrid().IsValidCellCoordinate(iCell))
  {
    UnitComponent::g_vTarget = vTargetPos;
  }

}

void SampleGameApp::SetupInput()
{
  m_pWindow->GetInputDevice()->SetClipMouseCursor(true);
  m_pWindow->GetInputDevice()->SetShowMouseCursor(false);
  m_pWindow->GetInputDevice()->SetMouseSpeed(ezVec2(0.002f));

  ezInputActionConfig cfg;
  cfg.m_bApplyTimeScaling = true;

  cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyEscape;
  ezInputManager::SetInputActionConfig("Main", "CloseApp", cfg, true);

  cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyLeftShift;
  ezInputManager::SetInputActionConfig("Game", "CamMoveFast", cfg, true);

  cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyA;
  ezInputManager::SetInputActionConfig("Game", "CamMoveLeft", cfg, true);

  cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyD;
  ezInputManager::SetInputActionConfig("Game", "CamMoveRight", cfg, true);

  cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyW;
  ezInputManager::SetInputActionConfig("Game", "CamMoveForwards", cfg, true);

  cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyS;
  ezInputManager::SetInputActionConfig("Game", "CamMoveBackwards", cfg, true);

  cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyQ;
  ezInputManager::SetInputActionConfig("Game", "CamMoveUp", cfg, true);

  cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyE;
  ezInputManager::SetInputActionConfig("Game", "CamMoveDown", cfg, true);

  cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyLeft;
  ezInputManager::SetInputActionConfig("Game", "CamRotateLeft", cfg, true);

  cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyRight;
  ezInputManager::SetInputActionConfig("Game", "CamRotateRight", cfg, true);

  cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyUp;
  ezInputManager::SetInputActionConfig("Game", "CamRotateUp", cfg, true);

  cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyDown;
  ezInputManager::SetInputActionConfig("Game", "CamRotateDown", cfg, true);

  cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseButton0;
  ezInputManager::SetInputActionConfig("Game", "SelectUnit", cfg, true);

  cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseButton1;
  ezInputManager::SetInputActionConfig("Game", "SendUnit", cfg, true);


  cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseWheelUp;
  ezInputManager::SetInputActionConfig("Game", "UnitLarger", cfg, true);

  cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseWheelDown;
  ezInputManager::SetInputActionConfig("Game", "UnitSmaller", cfg, true);
}

void SampleGameApp::SelectUnit()
{
  ezVec2I32 iCell = GetPickedGridCell();

  if (!m_pLevel->GetGrid().IsValidCellCoordinate(iCell))
    return;

  const GameCellData& Cell = m_pLevel->GetGrid().GetCell(iCell);

  UnitComponent* pUnit;
  if (m_pLevel->GetWorld()->TryGetComponent<UnitComponent>(Cell.m_hUnit, pUnit))
  {
    m_pSelectedUnits->Clear();
    m_pSelectedUnits->ToggleSelection(pUnit->GetOwner()->GetHandle());
  }
  else
    m_pSelectedUnits->Clear();
}

GameGrid* g_pGrid;

ezCallbackResult::Enum PointOnLine(ezInt32 x, ezInt32 y, void* pPassThrough)
{
  UnitComponent* pUnit = (UnitComponent*) pPassThrough;

  const ezVec2I32 coord = g_pGrid->GetCellAtWorldPosition(ezVec3((float) x, 1.0f, (float) y));

  if (!g_pGrid->IsValidCellCoordinate(coord))
    return ezCallbackResult::Stop;

  GameCellData& Cell = g_pGrid->GetCell(coord);

  if (Cell.m_uiVisited >= GameCellData::s_uiVisitCounter)
    return ezCallbackResult::Stop;

  Cell.m_uiVisited = GameCellData::s_uiVisitCounter;

  //if (Cell.m_iCellType != 0)
    //return ezCallbackResult::Stop;

  pUnit->m_Path.PushBack(ezVec3((float) x, 1, (float) y));
  EZ_NAN_ASSERT(&pUnit->m_Path.PeekBack());

  return ezCallbackResult::Continue;
}

struct MyPathState : ezPathStateBase
{
  EZ_DECLARE_POD_TYPE();

  bool m_bWentDiagonal;

};

class ezGridPathStateGenerator : public ezPathStateGenerator<MyPathState>
{
public:
  ezInt32 m_iGridWidth;
  ezInt32 m_iGridHeight;
  GameGrid* m_pGameGrid;

  ezInt32 m_iTargetX;
  ezInt32 m_iTargetY;

  virtual void StartSearch(ezInt64 iStartNodeIndex, const MyPathState* pStartState, ezInt64 iTargetNodeIndex) EZ_OVERRIDE
  {
    m_iTargetX = (ezInt32) iTargetNodeIndex / m_iGridWidth;
    m_iTargetY = (ezInt32) iTargetNodeIndex % m_iGridWidth;
  }

  virtual void GenerateAdjacentStates(ezInt64 iNodeIndex, const MyPathState& StartState, ezPathSearch<MyPathState>* pPathSearch) EZ_OVERRIDE
  {
    const ezInt32 iGridPosY = (ezInt32) iNodeIndex / m_iGridWidth;
    const ezInt32 iGridPosX = (ezInt32) iNodeIndex % m_iGridWidth;

    const ezInt32 iDX[8] = {   -1,    0,    1,    0,    1,   -1,    1,   -1 };
    const ezInt32 iDY[8] = {    0,    1,    0,   -1,    1,   -1,   -1,    1 };
    const float fCost[8] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.5f, 1.5f, 1.5f, 1.5f };

    const float fCurCost = StartState.m_fCostToNode;

    int iAllowedDirections = StartState.m_bWentDiagonal ? 4 : 8;
    iAllowedDirections = 8; // disabled

    for (ezInt32 i = 0; i < iAllowedDirections; ++i)
    {
      const ezInt32 iNextPosX = iGridPosX + iDX[i];
      const ezInt32 iNextPosY = iGridPosY + iDY[i];

      if (iNextPosX < 0 || iNextPosY < 0 || iNextPosX >= m_iGridWidth || iNextPosY >= m_iGridHeight)
        continue;

      if (m_pGameGrid->GetCell(ezVec2I32(iNextPosX, iNextPosY)).m_iCellType == 1)
        continue;

      ezVec2 vCurPos((float) iNextPosX, (float) iNextPosY);
      ezVec2 vTargetPos((float) m_iTargetX, (float) m_iTargetY);
      float fDistance = (vTargetPos - vCurPos).GetLength();

      MyPathState NextState;
      NextState.m_fCostToNode = fCurCost + fCost[i];
      NextState.m_fEstimatedCostToTarget = NextState.m_fCostToNode + fDistance;

      // this demonstrates how path-searches can have state along a path
      // the unit may only walk diagonally, if it did not do so in the previous step already
      // This can be used to search an optimal path that takes special (limited) actions into account
      // for example a unit might jump over an obstacle, as long as it doesn't do so all the time
      // (ie. the jump over obstacle action requires a cool-down)
      //
      // Or in a space game a ship could have a special action that lunges it forwards by ten tiles, but has a long cooldown
      // Now when a map has lots of nebulas, traversing those clouds may be slower, but with the special lunge forwards action
      // it is instantanious. The path search can now make use of that action to find the shortest path even through such clouds
      // by jumping through those where it saves the most time (e.g. a ship would go around a small cloud to save the action for
      // jumping through a large one later)
      NextState.m_bWentDiagonal = i >= 4;

      const ezInt64 iNextNodeIndex = m_pGameGrid->ConvertCellCoordinateToIndex(ezVec2I32(iNextPosX, iNextPosY));

      pPathSearch->AddPathNode(iNextNodeIndex, NextState);
    }

  }

};

class ezNavmeshPathStateGenerator : public ezPathStateGenerator<MyPathState>
{
public:
  ezGridNavmesh* m_pNavmesh;

  ezVec2 m_vTarget;

  virtual void StartSearch(ezInt64 iStartNodeIndex, const MyPathState* pStartState, ezInt64 iTargetNodeIndex) EZ_OVERRIDE
  {
  }

  virtual void GenerateAdjacentStates(ezInt64 iCurArea, const MyPathState& StartState, ezPathSearch<MyPathState>* pPathSearch) EZ_OVERRIDE
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

void SampleGameApp::SendUnit()
{
  ezVec2I32 iCell = GetPickedGridCell();

  if (!m_pLevel->GetGrid().IsValidCellCoordinate(iCell))
    return;

  g_pGrid = &m_pLevel->GetGrid();

  ezVec3 vTarget = m_pLevel->GetGrid().GetCellWorldSpaceCenter(iCell);

  //ezGridPathStateGenerator GridStateGenerator;
  //GridStateGenerator.m_pGameGrid   = g_pGrid;
  //GridStateGenerator.m_iGridWidth  = g_pGrid->GetWidth();
  //GridStateGenerator.m_iGridHeight = g_pGrid->GetDepth();

  ezNavmeshPathStateGenerator NavmeshStateGenerator;
  NavmeshStateGenerator.m_pNavmesh = &m_pLevel->GetNavmesh();
  NavmeshStateGenerator.m_vTarget = ezVec2(vTarget.x, vTarget.z);

  ezPathSearch<MyPathState> PathSearch;
  //PathSearch.SetPathStateGenerator(&GridStateGenerator);
  PathSearch.SetPathStateGenerator(&NavmeshStateGenerator);

  for (ezUInt32 i = 0; i < m_pSelectedUnits->GetCount();++i)
  {
    ezGameObjectHandle hObject = m_pSelectedUnits->GetObject(i);

    ezGameObject* pObject;
    if (m_pLevel->GetWorld()->TryGetObject(hObject, pObject))
    {
      const ezVec3 vPos = pObject->GetLocalPosition();

      UnitComponent* pUnit;
      if (pObject->TryGetComponentOfType<UnitComponent>(pUnit))
      {
        //pUnit->m_Path.Clear();

        ++GameCellData::s_uiVisitCounter;

        const ezVec2I32 StartCoord = g_pGrid->GetCellAtWorldPosition(vPos);
        const ezVec2I32 TargetCoord = g_pGrid->GetCellAtWorldPosition(vTarget);

        MyPathState StartState;
        StartState.m_fCostToNode = 0.0f;
        StartState.m_fEstimatedCostToTarget = 0.0f;
        StartState.m_bWentDiagonal = false;
        //StartState.m_iReachedThroughNode = g_pGrid->GetCellIndex(StartCoord);
        StartState.m_iReachedThroughNode = m_pLevel->GetNavmesh().GetAreaAt(StartCoord);

        const ezInt64 iTargetCell = g_pGrid->ConvertCellCoordinateToIndex(TargetCoord);
        const ezInt64 iTargetArea = m_pLevel->GetNavmesh().GetAreaAt(TargetCoord);

        ezDeque<ezPathSearch<MyPathState>::PathResultData> PathNodeIndices;

        ezStopwatch s;

        //if (PathSearch.FindPath(StartState.m_iReachedThroughNode, StartState, iTargetCell, PathNodeIndices) == EZ_SUCCESS)
        if (PathSearch.FindPath(StartState.m_iReachedThroughNode, StartState, iTargetArea, PathNodeIndices) == EZ_SUCCESS)
        {
          ezLog::Info("Found Path: %.2fms", s.Checkpoint().GetMilliseconds());

          s.Checkpoint();

          pUnit->m_Path.Clear();
          pUnit->m_Path.Reserve(PathNodeIndices.GetCount());

          for (ezInt32 i = 1; i < (ezInt32) PathNodeIndices.GetCount() - 1; ++i)
          {
            //const ezVec2I32 Coord = g_pGrid->GetCellCoordsByInex((ezUInt32) PathNodeIndices[i].m_iNodeIndex);
            const ezGridNavmesh::ConvexArea& Area = m_pLevel->GetNavmesh().GetConvexArea((ezInt32) PathNodeIndices[i].m_iNodeIndex);
            const ezVec2 vAreaCenter (Area.m_Rect.x + Area.m_Rect.width * 0.5f, Area.m_Rect.y + Area.m_Rect.height * 0.5f);
            const ezVec2I32 AreaCoord((ezInt32) vAreaCenter.x, (ezInt32) vAreaCenter.y);

            //pUnit->m_Path.PushBack(g_pGrid->GetCellOrigin(Coord) + g_pGrid->GetCellSize() * 0.5f);
            pUnit->m_Path.PushBack(g_pGrid->GetCellWorldSpaceCenter(AreaCoord));
          }

          pUnit->m_Path.PushBack(vTarget);
        }
        else
          ezLog::Info("No Path found: %.2fms", s.Checkpoint().GetMilliseconds());

        //ez2DGridUtils::ComputePointsOnLine((ezInt32) vPos.x, (ezInt32) vPos.z, (ezInt32) vTarget.x, (ezInt32) vTarget.z, PointOnLine, pUnit);
        //ez2DGridUtils::ComputePointsOnCircle((ezInt32) vPos.x, (ezInt32) vPos.z, 4, PointOnLine, pUnit);
        //ez2DGridUtils::FloodFill((ezInt32) vPos.x, (ezInt32) vPos.z, PointOnLine, pUnit);
      }
    }
  }
}



