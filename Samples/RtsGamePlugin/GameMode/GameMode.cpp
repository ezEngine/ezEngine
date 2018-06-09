#include <PCH.h>
#include <RtsGamePlugin/GameMode/GameMode.h>
#include <RtsGamePlugin/GameState/RtsGameState.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Pipeline/View.h>
#include <Core/Input/InputManager.h>

RtsGameMode::RtsGameMode() = default;
RtsGameMode::~RtsGameMode() = default;

void RtsGameMode::ActivateMode(ezWorld* pMainWorld, ezViewHandle hView, ezCamera* pMainCamera)
{
  if (!m_bInitialized)
  {
    m_pGameState = RtsGameState::GetSingleton();
    m_pMainWorld = pMainWorld;
    m_hMainView = hView;
    m_pMainCamera = pMainCamera;

    m_bInitialized = true;
    RegisterInputActions();
  }

  OnActivateMode();
}

void RtsGameMode::DeactivateMode()
{
  OnDeactivateMode();
}

void RtsGameMode::ProcessInput(ezUInt32 uiMousePosX, ezUInt32 uiMousePosY, ezKeyState::Enum LeftClickState, ezKeyState::Enum RightClickState)
{
  m_uiMousePosX = uiMousePosX;
  m_uiMousePosY = uiMousePosY;
  m_LeftClickState = LeftClickState;
  m_RightClickState = RightClickState;

  OnProcessInput();
}

void RtsGameMode::BeforeWorldUpdate()
{
  OnBeforeWorldUpdate();
}

void RtsGameMode::DoDefaultCameraInput()
{
  ezView* pView = nullptr;
  if (!ezRenderWorld::TryGetView(m_hMainView, pView))
    return;

  const auto vp = pView->GetViewport();

  float movePosX, moveNegX, movePosY, moveNegY, zoomIn, zoomOut;
  ezInputManager::GetInputActionState("Game", "CamMovePosX", &movePosX);
  ezInputManager::GetInputActionState("Game", "CamMoveNegX", &moveNegX);
  ezInputManager::GetInputActionState("Game", "CamMovePosY", &movePosY);
  ezInputManager::GetInputActionState("Game", "CamMoveNegY", &moveNegY);
  ezInputManager::GetInputActionState("Game", "CamZoomIn", &zoomIn);
  ezInputManager::GetInputActionState("Game", "CamZoomOut", &zoomOut);

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
