#include <PCH.h>
#include <RtsGamePlugin/GameState/RtsGameState.h>
#include <Foundation/Logging/Log.h>
#include <Core/World/World.h>
#include <GameEngine/DearImgui/DearImgui.h>
#include <System/Window/Window.h>
#include <RtsGamePlugin/GameMode/GameMode.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <GameEngine/Collection/CollectionResource.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(RtsGameState, 1, ezRTTIDefaultAllocator<RtsGameState>)
EZ_END_DYNAMIC_REFLECTED_TYPE

RtsGameState* RtsGameState::s_pSingleton = nullptr;

RtsGameState::RtsGameState()
{
  s_pSingleton = this;
}

float RtsGameState::GetCameraZoom() const
{
  return m_fCameraZoom;
}

float RtsGameState::SetCameraZoom(float zoom)
{
  m_fCameraZoom = ezMath::Clamp(zoom, 5.0f, 300.0f);

  return m_fCameraZoom;
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

  PreloadAssets();

  SwitchToGameMode(RtsActiveGameMode::EditLevelMode);
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

void RtsGameState::PreloadAssets()
{
  // Load all assets that are referenced in some Collections

  m_CollectionSpace = ezResourceManager::LoadResource<ezCollectionResource>("{ 7cd0dfa6-d2bb-433e-9fa2-b17bfae42b6b }");
  m_CollectionFederation = ezResourceManager::LoadResource<ezCollectionResource>("{ 1edd3af8-6d59-4825-b853-ee8d7a60cb03 }");
  m_CollectionKlingons = ezResourceManager::LoadResource<ezCollectionResource>("{ c683d049-0e54-4c42-9764-a122f9dbc69d }");

  // Register the loaded assets with the names defined in the collections
  // This allows to easily spawn those objects with human readable names instead of GUIDs
  {
    ezResourceLock<ezCollectionResource> pCollection(m_CollectionSpace, ezResourceAcquireMode::NoFallback);
    pCollection->RegisterNames();
  }
  {
    ezResourceLock<ezCollectionResource> pCollection(m_CollectionFederation, ezResourceAcquireMode::NoFallback);
    pCollection->RegisterNames();
  }
  {
    ezResourceLock<ezCollectionResource> pCollection(m_CollectionKlingons, ezResourceAcquireMode::NoFallback);
    pCollection->RegisterNames();
  }
}

void RtsGameState::BeforeWorldUpdate()
{
  EZ_LOCK(m_pMainWorld->GetWriteMarker());

  ActivateQueuedGameMode();

  if (m_pActiveGameMode)
  {
    m_pActiveGameMode->BeforeWorldUpdate();
  }
}

void RtsGameState::ActivateQueuedGameMode()
{
  switch (m_GameModeToSwitchTo)
  {
  case RtsActiveGameMode::None:
    SetActiveGameMode(nullptr);
    break;
  case RtsActiveGameMode::MainMenuMode:
    SetActiveGameMode(&m_MainMenuMode);
    break;
  case RtsActiveGameMode::BattleMode:
    SetActiveGameMode(&m_BattleMode);
    break;
  case RtsActiveGameMode::EditLevelMode:
    SetActiveGameMode(&m_EditLevelMode);
    break;

  default:
    EZ_ASSERT_NOT_IMPLEMENTED;
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
  // can't just switch game modes in the middle of a frame, so delay this to the next frame
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

