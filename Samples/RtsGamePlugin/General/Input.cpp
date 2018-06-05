#include <PCH.h>
#include <RtsGamePlugin/GameState/RtsGameState.h>
#include <Core/Input/InputManager.h>
#include <System/Window/Window.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Pipeline/View.h>
#include <GameEngine/DearImgui/DearImgui.h>
#include <Core/World/World.h>

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

  // Mouse Input
  {
    ezInputActionConfig cfg;

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_MousePositionX;
    ezInputManager::SetInputActionConfig("Game", "MousePosX", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_MousePositionY;
    ezInputManager::SetInputActionConfig("Game", "MousePosY", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseButton0;
    ezInputManager::SetInputActionConfig("Game", "MouseLeftClick", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseButton1;
    ezInputManager::SetInputActionConfig("Game", "MouseRightClick", cfg, true);
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
  m_LeftClickState = ezInputManager::GetInputActionState("Game", "MouseLeftClick");
  m_RightClickState = ezInputManager::GetInputActionState("Game", "MouseRightClick");

  m_uiMousePosX = (ezUInt32)(valueX * vp.width);
  m_uiMousePosY = (ezUInt32)((1.0f - valueY) * vp.height);
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
      m_pActiveGameMode->ProcessInput(m_uiMousePosX, m_uiMousePosY, m_LeftClickState, m_RightClickState);

    m_pActiveGameMode->AfterProcessInput();
  }
}
