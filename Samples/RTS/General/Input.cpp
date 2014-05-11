#include <PCH.h>
#include <RTS/General/Application.h>
#include <RTS/General/Window.h>
#include <Core/Input/InputManager.h>

void SampleGameApp::UpdateInput(ezTime UpdateDiff)
{
  ezInputManager::Update(UpdateDiff);

  if (ezInputManager::GetInputActionState("Main", "ToggleConsole") == ezKeyState::Pressed)
    m_bConsoleActive = !m_bConsoleActive;

  m_Console.DoDefaultInputHandling(m_bConsoleActive);

  if (m_bConsoleActive)
    return;

  if (ezInputManager::GetInputActionState("Main", "CloseApp") == ezKeyState::Pressed)
    m_bActiveRenderLoop = false;

  if (ezInputManager::GetInputActionState("Main", "CaptureProfiling") == ezKeyState::Pressed)
  {
    ezFileWriter fileWriter;
    if (fileWriter.Open("profiling.json") == EZ_SUCCESS)
    {
      ezProfilingSystem::Capture(fileWriter);
    }
  }

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
    RevealerComponent::g_fDefaultRadius += 1.0f;

  if (ezInputManager::GetInputActionState("Game", "UnitSmaller", &f) == ezKeyState::Pressed)
    RevealerComponent::g_fDefaultRadius -= 1.0f;
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

  cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyF1;
  ezInputManager::SetInputActionConfig("Main", "ToggleConsole", cfg, true);

  cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyF12;
  ezInputManager::SetInputActionConfig("Main", "CaptureProfiling", cfg, true);

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

