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
  m_pGameState->RenderUnitSelection();
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

  float fZoom = m_pGameState->GetCameraZoom();

  if (zoom != 0.0f)
  {
    if (zoom > 0)
      fZoom *= 1.1f;
    else
      fZoom *= 1.0f / 1.1f;

    fZoom = m_pGameState->SetCameraZoom(fZoom);

    ezVec3 pos = m_pMainCamera->GetCenterPosition();
    pos.z = fZoom;
    m_pMainCamera->LookAt(pos, pos + m_pMainCamera->GetCenterDirForwards(), m_pMainCamera->GetCenterDirUp());
  }

  if (bMoveCamera)
  {
    const float fMoveScale = 0.005f * fZoom;

    const float fMoveX = fDimX * moveX * fMoveScale;
    const float fMoveY = fDimY * moveY * fMoveScale;

    m_pMainCamera->MoveGlobally(ezVec3(-fMoveY, fMoveX, 0));
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

  ezVec3 vPickedGroundPlanePos;
  if (m_pGameState->PickGroundPlanePosition(vPickedGroundPlanePos).Failed())
    return;

  if (ezInputManager::GetInputActionState("EditLevelMode", "PlaceObject") == ezKeyState::Pressed)
  {
    m_pGameState->SpawnNamedObjectAt(ezTransform(vPickedGroundPlanePos, ezQuat::IdentityQuaternion()), "FederationShip1", 0);
    return;
  }

  auto& unitSelection = m_pGameState->m_SelectedUnits;

  if (ezInputManager::GetInputActionState("EditLevelMode", "RemoveObject") == ezKeyState::Pressed)
  {
    for (ezUInt32 i = 0; i < unitSelection.GetCount(); ++i)
    {
      ezGameObjectHandle hObject = unitSelection.GetObject(i);
      m_pMainWorld->DeleteObjectDelayed(hObject);
    }

    return;
  }

  if (m_LeftClickState == ezKeyState::Released)
  {
    m_pGameState->SelectUnits();
  }
}

