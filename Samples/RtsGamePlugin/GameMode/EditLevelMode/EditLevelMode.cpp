#include <PCH.h>
#include <RtsGamePlugin/GameMode/EditLevelMode/EditLevelMode.h>
#include <GameEngine/DearImgui/DearImgui.h>
#include <Core/Input/InputManager.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RtsGamePlugin/GameState/RtsGameState.h>

RtsEditLevelMode::RtsEditLevelMode() = default;
RtsEditLevelMode::~RtsEditLevelMode() = default;

void RtsEditLevelMode::OnActivateMode()
{
}

void RtsEditLevelMode::OnDeactivateMode()
{
}

void RtsEditLevelMode::OnBeforeWorldUpdate()
{
  DisplayMainUI();
}

void RtsEditLevelMode::UpdateCamera()
{
  ezView* pView = nullptr;
  if (!ezRenderWorld::TryGetView(m_hMainView, pView))
    return;

  const auto vp = pView->GetViewport();

  float movePosX, moveNegX, movePosY, moveNegY, zoomIn, zoomOut;
  ezInputManager::GetInputActionState("EditLevelMode", "CamMovePosX", &movePosX);
  ezInputManager::GetInputActionState("EditLevelMode", "CamMoveNegX", &moveNegX);
  ezInputManager::GetInputActionState("EditLevelMode", "CamMovePosY", &movePosY);
  ezInputManager::GetInputActionState("EditLevelMode", "CamMoveNegY", &moveNegY);
  ezInputManager::GetInputActionState("EditLevelMode", "CamZoomIn", &zoomIn);
  ezInputManager::GetInputActionState("EditLevelMode", "CamZoomOut", &zoomOut);

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

void RtsEditLevelMode::DisplayMainUI()
{

}

void RtsEditLevelMode::RegisterInputActions()
{
  RtsGameMode::RegisterInputActions();

  ezInputActionConfig cfg;

  // Camera
  {
    cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseWheelUp;
    ezInputManager::SetInputActionConfig("EditLevelMode", "CamZoomIn", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseWheelDown;
    ezInputManager::SetInputActionConfig("EditLevelMode", "CamZoomOut", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseMovePosX;
    ezInputManager::SetInputActionConfig("EditLevelMode", "CamMovePosX", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseMoveNegX;
    ezInputManager::SetInputActionConfig("EditLevelMode", "CamMoveNegX", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseMovePosY;
    ezInputManager::SetInputActionConfig("EditLevelMode", "CamMovePosY", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_MouseMoveNegY;
    ezInputManager::SetInputActionConfig("EditLevelMode", "CamMoveNegY", cfg, true);
  }

  // Level Editing
  {
    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeySpace;
    ezInputManager::SetInputActionConfig("EditLevelMode", "PlaceObject", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyDelete;
    ezInputManager::SetInputActionConfig("EditLevelMode", "RemoveObject", cfg, true);
  }
}

void RtsEditLevelMode::OnProcessInput()
{
  UpdateCamera();

  ezVec3 vPickingRayStart, vPickingRayDir, vPickedGroundPlanePos;
  if (ComputePickingRay(vPickingRayStart, vPickingRayDir).Failed())
    return;

  if (PickGroundPlanePosition(vPickingRayStart, vPickingRayDir, vPickedGroundPlanePos).Failed())
    return;

  if (ezInputManager::GetInputActionState("EditLevelMode", "PlaceObject") == ezKeyState::Pressed)
  {
    SpawnNamedObjectAt(ezTransform(vPickedGroundPlanePos, ezQuat::IdentityQuaternion()), "FederationShip1", 0);
  }
}

