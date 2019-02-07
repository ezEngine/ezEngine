#include <RtsGamePluginPCH.h>

#include <RtsGamePlugin/GameState/RtsGameState.h>

void RtsGameState::ConfigureInputDevices()
{
  SUPER::ConfigureInputDevices();

  m_pMainWindow->GetInputDevice()->SetClipMouseCursor(true);
  m_pMainWindow->GetInputDevice()->SetShowMouseCursor(true);
  m_pMainWindow->GetInputDevice()->SetMouseSpeed(ezVec2(0.002f));
}

void RtsGameState::ConfigureInputActions()
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

void RtsGameState::UpdateMousePosition()
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
  m_MouseInputState.m_MousePos.y = (ezUInt32)((1.0f - valueY) * vp.height);

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

  m_MouseInputState.m_bLeftMouseMoved =
      m_MouseInputState.m_bLeftMouseMoved ||
      RtsMouseInputState::HasMouseMoved(m_MouseInputState.m_MousePosLeftClick, m_MouseInputState.m_MousePos);
  m_MouseInputState.m_bRightMouseMoved =
      m_MouseInputState.m_bRightMouseMoved ||
      RtsMouseInputState::HasMouseMoved(m_MouseInputState.m_MousePosRightClick, m_MouseInputState.m_MousePos);

  ComputePickingRay();
}

void RtsGameState::ProcessInput()
{
  EZ_LOCK(m_pMainWorld->GetWriteMarker());

  bool bProcessInput = true;

  if (ezImgui::GetSingleton() != nullptr)
  {
    // we got input (ie. this function was called), so let Imgui know that it can use the input
    ezImgui::GetSingleton()->SetPassInputToImgui(true);

    // do not process input, when Imgui already wants to work with it
    if (ezImgui::GetSingleton()->WantsInput())
      bProcessInput = false;
  }

  UpdateMousePosition();

  if (m_pActiveGameMode)
  {
    if (bProcessInput)
      m_pActiveGameMode->ProcessInput(m_MouseInputState);

    m_pActiveGameMode->AfterProcessInput();
  }
}
