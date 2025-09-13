#include <RTSPlugin/RTSPluginPCH.h>

#include <Core/ActorSystem/Actor.h>
#include <Core/ActorSystem/ActorManager.h>
#include <RTSPlugin/GameState/RTSGameState.h>

void RTSGameState::ConfigureMainWindowInputDevices(ezWindow* pWindow)
{
  SUPER::ConfigureMainWindowInputDevices(pWindow);

  // pWindow->GetInputDevice()->SetClipMouseCursor(ezMouseCursorClipMode::NoClip);
  pWindow->GetInputDevice()->SetShowMouseCursor(true);
  pWindow->GetInputDevice()->SetMouseSpeed(ezVec2(0.002f));
}

void RTSGameState::ConfigureInputActions()
{
  SUPER::ConfigureInputActions();

  ezInputActionConfig cfg;

  // Mouse Input
  {
    cfg.m_sInputSlotTrigger[0] = ezInputSlot_MousePositionX;
    ezInputManager::SetInputActionConfig("Game", "MousePosX", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_MousePositionY;
    ezInputManager::SetInputActionConfig("Game", "MousePosY", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseButton0;
    ezInputManager::SetInputActionConfig("Game", "MouseLeftClick", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseButton1;
    ezInputManager::SetInputActionConfig("Game", "MouseRightClick", cfg, true);
  }

  // Default Camera Navigation
  {
    cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseWheelUp;
    ezInputManager::SetInputActionConfig("Game", "CamZoomIn", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseWheelDown;
    ezInputManager::SetInputActionConfig("Game", "CamZoomOut", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseMovePosX;
    ezInputManager::SetInputActionConfig("Game", "CamMovePosX", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseMoveNegX;
    ezInputManager::SetInputActionConfig("Game", "CamMoveNegX", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseMovePosY;
    ezInputManager::SetInputActionConfig("Game", "CamMovePosY", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseMoveNegY;
    ezInputManager::SetInputActionConfig("Game", "CamMoveNegY", cfg, true);
  }
}

void RTSGameState::UpdateMousePosition()
{
  ezView* pView = nullptr;
  if (!ezRenderWorld::TryGetView(m_hMainView, pView))
    return;

  const auto vp = pView->GetViewport();

  float valueX, valueY;
  ezInputManager::GetInputActionState("Game", "MousePosX", &valueX);
  ezInputManager::GetInputActionState("Game", "MousePosY", &valueY);
  m_MouseInputState.m_LeftClickState = ezInputManager::GetInputActionState("Game", "MouseLeftClick");
  m_MouseInputState.m_RightClickState = ezInputManager::GetInputActionState("Game", "MouseRightClick");

  m_MouseInputState.m_MousePos.x = (ezUInt32)(valueX * vp.width);
  m_MouseInputState.m_MousePos.y = (ezUInt32)(valueY * vp.height);

  if (m_MouseInputState.m_LeftClickState == ezKeyState::Pressed)
  {
    m_MouseInputState.m_MousePosLeftClick = m_MouseInputState.m_MousePos;
    m_MouseInputState.m_bLeftMouseMoved = false;
  }

  if (m_MouseInputState.m_RightClickState == ezKeyState::Pressed)
  {
    m_MouseInputState.m_MousePosRightClick = m_MouseInputState.m_MousePos;
    m_MouseInputState.m_bRightMouseMoved = false;
  }

  m_MouseInputState.m_bLeftMouseMoved = m_MouseInputState.m_bLeftMouseMoved || RtsMouseInputState::HasMouseMoved(m_MouseInputState.m_MousePosLeftClick, m_MouseInputState.m_MousePos);
  m_MouseInputState.m_bRightMouseMoved = m_MouseInputState.m_bRightMouseMoved || RtsMouseInputState::HasMouseMoved(m_MouseInputState.m_MousePosRightClick, m_MouseInputState.m_MousePos);

  ComputePickingRay().IgnoreResult();
}

void RTSGameState::ProcessInput()
{
  SUPER::ProcessInput();

  EZ_LOCK(m_pMainWorld->GetWriteMarker());

  UpdateMousePosition();

  if (m_pActiveGameMode)
  {
    m_pActiveGameMode->ProcessInput(m_MouseInputState);

    m_pActiveGameMode->AfterProcessInput();
  }
}
