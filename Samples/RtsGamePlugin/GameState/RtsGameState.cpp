#include <PCH.h>
#include <RtsGamePlugin/GameState/RtsGameState.h>
#include <Foundation/Logging/Log.h>
#include <Core/World/World.h>
#include <GameEngine/DearImgui/DearImgui.h>
#include <System/Window/Window.h>
#include <RtsGamePlugin/GameMode/GameMode.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(RtsGameState, 1, ezRTTIDefaultAllocator<RtsGameState>)
EZ_END_DYNAMIC_REFLECTED_TYPE

RtsGameState::RtsGameState()
{
}

float RtsGameState::CanHandleThis(ezGameApplicationType AppType, ezWorld* pWorld) const
{
  return 2.0f;
}

void RtsGameState::OnActivation(ezWorld* pWorld)
{
  EZ_LOG_BLOCK("GameState::Activate");

  SUPER::OnActivation(pWorld);

  if (ezImgui::GetSingleton() == nullptr)
  {
    EZ_DEFAULT_NEW(ezImgui);
  }

  SwitchToGameMode(RtsActiveGameMode::MainMenuMode);
}

void RtsGameState::OnDeactivation()
{
  EZ_LOG_BLOCK("GameState::Deactivate");

  SetActiveGameMode(nullptr);

  if (ezImgui::GetSingleton() != nullptr)
  {
    ezImgui* pImgui = ezImgui::GetSingleton();
    EZ_DEFAULT_DELETE(pImgui);
  }

  SUPER::OnDeactivation();
}

void RtsGameState::BeforeWorldUpdate()
{
  EZ_LOCK(m_pMainWorld->GetWriteMarker());

  switch (m_GameModeToSwitchTo)
  {
  case RtsActiveGameMode::None:
    SetActiveGameMode(nullptr);
    break;
  case RtsActiveGameMode::MainMenuMode:
    SetActiveGameMode(&m_MainMenuMode);
    break;

  default:
    EZ_ASSERT_NOT_IMPLEMENTED;
  }

  if (m_pActiveGameMode)
  {
    m_pActiveGameMode->BeforeWorldUpdate();
  }
}

void RtsGameState::ConfigureMainCamera()
{
  SUPER::ConfigureMainCamera();

  ezVec3 vCameraPos = ezVec3(0.0f, 0.0f, 10.0f);

  ezCoordinateSystem coordSys;

  if (m_pMainWorld)
  {
    m_pMainWorld->GetCoordinateSystem(vCameraPos, coordSys);
  }
  else
  {
    coordSys.m_vForwardDir.Set(1, 0, 0);
    coordSys.m_vRightDir.Set(0, 1, 0);
    coordSys.m_vUpDir.Set(0, 0, 1);
  }

  const float fCameraZoom = 10.0f;
  m_MainCamera.SetCameraMode(ezCameraMode::OrthoFixedHeight, fCameraZoom, -20.0f, 20.0f);
  m_MainCamera.LookAt(vCameraPos, vCameraPos - coordSys.m_vUpDir, coordSys.m_vForwardDir);
}

void RtsGameState::SwitchToGameMode(RtsActiveGameMode mode)
{
  m_GameModeToSwitchTo = mode;
}

void RtsGameState::SetActiveGameMode(RtsGameMode* pMode)
{
  if (m_pActiveGameMode == pMode)
    return;

  if (m_pActiveGameMode)
    m_pActiveGameMode->DeactivateMode();

  m_pActiveGameMode = pMode;

  if (m_pActiveGameMode)
    m_pActiveGameMode->ActivateMode(m_pMainWorld, m_hMainView, &m_MainCamera);
}

