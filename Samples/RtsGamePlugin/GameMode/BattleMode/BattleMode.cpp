#include <PCH.h>
#include <RtsGamePlugin/GameMode/BattleMode/BattleMode.h>
#include <GameEngine/DearImgui/DearImgui.h>
#include <Core/Input/InputManager.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RtsGamePlugin/GameState/RtsGameState.h>

RtsBattleMode::RtsBattleMode() = default;
RtsBattleMode::~RtsBattleMode() = default;

void RtsBattleMode::OnActivateMode()
{
}

void RtsBattleMode::OnDeactivateMode()
{
}

void RtsBattleMode::OnBeforeWorldUpdate()
{
  DisplayMainUI();
}

void RtsBattleMode::UpdateCamera()
{
  //m_pMainCamera->LookAt(vCamPos, vCamTarget, ezVec3(1, 0, 0));
  //m_pMainCamera->SetCameraMode(ezCameraMode::OrthoFixedHeight, fCameraZoom, m_pMainCamera->GetNearPlane(), m_pMainCamera->GetFarPlane());

  ezView* pView = nullptr;
  if (!ezRenderWorld::TryGetView(m_hMainView, pView))
    return;

  const auto vp = pView->GetViewport();

  float movePosX, moveNegX, movePosY, moveNegY, zoomIn, zoomOut;
  ezInputManager::GetInputActionState("BattleMode", "CamMovePosX", &movePosX);
  ezInputManager::GetInputActionState("BattleMode", "CamMoveNegX", &moveNegX);
  ezInputManager::GetInputActionState("BattleMode", "CamMovePosY", &movePosY);
  ezInputManager::GetInputActionState("BattleMode", "CamMoveNegY", &moveNegY);
  ezInputManager::GetInputActionState("BattleMode", "CamZoomIn", &zoomIn);
  ezInputManager::GetInputActionState("BattleMode", "CamZoomOut", &zoomOut);

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
    float fZoom = RtsGameState::GetSingleton()->GetCameraZoom();

    fZoom += zoom * 5.0f;
    fZoom = RtsGameState::GetSingleton()->SetCameraZoom(fZoom);

    m_pMainCamera->SetCameraMode(ezCameraMode::OrthoFixedHeight, fZoom, -20.0f, 20.0f);
  }
}

void RtsBattleMode::DisplayMainUI()
{

}

void RtsBattleMode::RegisterInputActions()
{
  RtsGameMode::RegisterInputActions();

  // Camera
  {
    ezInputActionConfig cfg;

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseWheelUp;
    ezInputManager::SetInputActionConfig("BattleMode", "CamZoomIn", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseWheelDown;
    ezInputManager::SetInputActionConfig("BattleMode", "CamZoomOut", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseMovePosX;
    ezInputManager::SetInputActionConfig("BattleMode", "CamMovePosX", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseMoveNegX;
    ezInputManager::SetInputActionConfig("BattleMode", "CamMoveNegX", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseMovePosY;
    ezInputManager::SetInputActionConfig("BattleMode", "CamMovePosY", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseMoveNegY;
    ezInputManager::SetInputActionConfig("BattleMode", "CamMoveNegY", cfg, true);
  }
}

void RtsBattleMode::OnProcessInput()
{
  UpdateCamera();

}
