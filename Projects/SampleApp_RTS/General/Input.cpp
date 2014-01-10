#include <SampleApp_RTS/General/Application.h>
#include <SampleApp_RTS/General/Window.h>

#include <Foundation/Logging/Log.h>
#include <Core/Input/InputManager.h>
#include <Foundation/Configuration/CVar.h>
#include <GameUtils/GridAlgorithms/Rasterization.h>

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
  ezGridCoordinate iCell = GetPickedGridCell(&vTargetPos);

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
  ezGridCoordinate iCell = GetPickedGridCell();

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

  //if (x < -5 || y < -5 || x > 5 || y > 5)
    //return ezCallbackResult::Stop;

  const ezGridCoordinate coord = g_pGrid->GetCellAtPosition(ezVec3((float) x, 1.0f, (float) y));

  if (!g_pGrid->IsValidCellCoordinate(coord))
    return ezCallbackResult::Stop;

  GameCellData& Cell = g_pGrid->GetCell(coord);

  if (Cell.m_uiVisited >= GameCellData::s_uiVisitCounter)
    return ezCallbackResult::Stop;

  Cell.m_uiVisited = GameCellData::s_uiVisitCounter;

  //if (Cell.m_iCellType != 0)
    //return ezCallbackResult::Stop;

  pUnit->m_Path.PushBack(ezVec3((float) x, 1, (float) y));

  return ezCallbackResult::Continue;
}

void SampleGameApp::SendUnit()
{
  ezGridCoordinate iCell = GetPickedGridCell();

  if (!m_pLevel->GetGrid().IsValidCellCoordinate(iCell))
    return;

  g_pGrid = &m_pLevel->GetGrid();

  ezVec3 vTarget = m_pLevel->GetGrid().GetCellOrigin(iCell) + m_pLevel->GetGrid().GetCellSize() * 0.5f;

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
        pUnit->m_Path.Clear();

        ++GameCellData::s_uiVisitCounter;

        ez2DGridUtils::ComputePointsOnLine((ezInt32) vPos.x, (ezInt32) vPos.z, (ezInt32) vTarget.x, (ezInt32) vTarget.z, PointOnLine, pUnit);
        //ez2DGridUtils::ComputePointsOnCircle((ezInt32) vPos.x, (ezInt32) vPos.z, 4, PointOnLine, pUnit);
        //ez2DGridUtils::FloodFill((ezInt32) vPos.x, (ezInt32) vPos.z, PointOnLine, pUnit);
      }
    }
  }
}



