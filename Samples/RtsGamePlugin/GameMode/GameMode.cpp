#include <PCH.h>
#include <RtsGamePlugin/GameMode/GameMode.h>

RtsGameMode::RtsGameMode() = default;
RtsGameMode::~RtsGameMode() = default;

void RtsGameMode::ActivateMode(ezWorld* pMainWorld, ezViewHandle hView, ezCamera* pMainCamera)
{
  if (!m_bInitialized)
  {
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

