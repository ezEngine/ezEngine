#include <PCH.h>
#include <RtsGamePlugin/GameMode/MainMenuMode/MainMenuMode.h>
#include <GameEngine/DearImgui/DearImgui.h>
#include <Core/Input/InputManager.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

RtsMainMenuMode::RtsMainMenuMode() = default;
RtsMainMenuMode::~RtsMainMenuMode() = default;

void RtsMainMenuMode::OnActivateMode()
{
}

void RtsMainMenuMode::OnDeactivateMode()
{
}

void RtsMainMenuMode::OnBeforeWorldUpdate()
{
  DisplayMainUI();
}

void RtsMainMenuMode::UpdateCamera()
{
  //m_pMainCamera->LookAt(vCamPos, vCamTarget, ezVec3(1, 0, 0));
  //m_pMainCamera->SetCameraMode(ezCameraMode::OrthoFixedHeight, fCameraZoom, m_pMainCamera->GetNearPlane(), m_pMainCamera->GetFarPlane());

  ezView* pView = nullptr;
  if (!ezRenderWorld::TryGetView(m_hMainView, pView))
    return;

  const auto vp = pView->GetViewport();

  float movePosX, moveNegX, movePosY, moveNegY, zoomIn, zoomOut;
  ezInputManager::GetInputActionState("MainMenuMode", "CamMovePosX", &movePosX);
  ezInputManager::GetInputActionState("MainMenuMode", "CamMoveNegX", &moveNegX);
  ezInputManager::GetInputActionState("MainMenuMode", "CamMovePosY", &movePosY);
  ezInputManager::GetInputActionState("MainMenuMode", "CamMoveNegY", &moveNegY);
  ezInputManager::GetInputActionState("MainMenuMode", "CamZoomIn", &zoomIn);
  ezInputManager::GetInputActionState("MainMenuMode", "CamZoomOut", &zoomOut);

  const float moveX = movePosX - moveNegX;
  const float moveY = movePosY - moveNegY;
  const float zoom = -zoomIn + zoomOut;

  const bool bMoveCamera = m_RightClickState != ezKeyState::Up;

  const float fDimY = m_pMainCamera->GetFovOrDim();
  const float fDimX = (fDimY / vp.height) * vp.width;

  if (bMoveCamera)
  {
    const float fMoveScale = 0.1f;

    const float fMoveX = fDimX * moveX * fMoveScale;
    const float fMoveY = fDimY * moveY * fMoveScale;

    m_pMainCamera->MoveGlobally(ezVec3(-fMoveY, fMoveX, 0));
  }

  if (zoom != 0.0f)
  {
    m_fCameraZoom += zoom * 5.0f;

    m_fCameraZoom = ezMath::Clamp(m_fCameraZoom, 5.0f, 300.0f);

    m_pMainCamera->SetCameraMode(ezCameraMode::OrthoFixedHeight, m_fCameraZoom, -20.0f, 20.0f);
  }
}

void RtsMainMenuMode::DisplayMainUI()
{

}

void RtsMainMenuMode::RegisterInputActions()
{
  RtsGameMode::RegisterInputActions();

  // Camera
  {
    ezInputActionConfig cfg;

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseWheelUp;
    ezInputManager::SetInputActionConfig("MainMenuMode", "CamZoomIn", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseWheelDown;
    ezInputManager::SetInputActionConfig("MainMenuMode", "CamZoomOut", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseMovePosX;
    ezInputManager::SetInputActionConfig("MainMenuMode", "CamMovePosX", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseMoveNegX;
    ezInputManager::SetInputActionConfig("MainMenuMode", "CamMoveNegX", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseMovePosY;
    ezInputManager::SetInputActionConfig("MainMenuMode", "CamMovePosY", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseMoveNegY;
    ezInputManager::SetInputActionConfig("MainMenuMode", "CamMoveNegY", cfg, true);
  }
}

void RtsMainMenuMode::OnProcessInput()
{
  UpdateCamera();

}
