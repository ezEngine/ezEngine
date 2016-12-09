#include "GameState.h"
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Time/Clock.h>
#include <Core/ResourceManager/ResourceManager.h>

#include <InputXBox360/InputDeviceXBox.h>

#include <Core/Application/Config/ApplicationConfig.h>

#include <GameFoundation/GameApplication/GameApplication.h>
#include <System/Window/Window.h>

EZ_APPLICATION_ENTRY_POINT(ezGameApplication, "Asteroids", ezGameApplicationType::StandAlone, "Data/Samples/Asteroids");

const char* szPlayerActions[MaxPlayerActions] = { "Forwards", "Backwards", "Left", "Right", "RotLeft", "RotRight", "Shoot" };
const char* szControlerKeys[MaxPlayerActions] = { "leftstick_posy", "leftstick_negy", "leftstick_negx", "leftstick_posx", "rightstick_negx", "rightstick_posx", "right_trigger" };

namespace
{
  static void RegisterInputAction(const char* szInputSet, const char* szInputAction, const char* szKey1, const char* szKey2 = nullptr, const char* szKey3 = nullptr)
  {
    ezInputActionConfig cfg;

    cfg = ezInputManager::GetInputActionConfig(szInputSet, szInputAction);
    cfg.m_bApplyTimeScaling = true;

    if (szKey1 != nullptr)     cfg.m_sInputSlotTrigger[0] = szKey1;
    if (szKey2 != nullptr)     cfg.m_sInputSlotTrigger[1] = szKey2;
    if (szKey3 != nullptr)     cfg.m_sInputSlotTrigger[2] = szKey3;

    ezInputManager::SetInputActionConfig(szInputSet, szInputAction, cfg, true);
  }
}

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(AsteroidGameState, 1, ezRTTIDefaultAllocator<AsteroidGameState>);
EZ_END_DYNAMIC_REFLECTED_TYPE

AsteroidGameState::AsteroidGameState()
{
  m_pLevel = nullptr;
}

void AsteroidGameState::OnActivation(ezWorld* pWorld)
{
  EZ_LOG_BLOCK("AsteroidGameState::Activate");

  ezGameState::OnActivation(pWorld);

  CreateGameLevel();
}

void AsteroidGameState::OnDeactivation()
{
  EZ_LOG_BLOCK("AsteroidGameState::Deactivate");

  DestroyLevel();

  ezGameState::OnDeactivation();
}

void AsteroidGameState::BeforeWorldUpdate()
{
  m_MainCamera.SetCameraMode(ezCameraMode::OrthoFixedHeight, 40, -10, 10);
  m_MainCamera.LookAt(ezVec3::ZeroVector(), ezVec3(0, 0, 1), ezVec3(0, 1, 0));
}

void AsteroidGameState::ConfigureInputActions()
{
  ezInputDeviceXBox360::GetDevice()->EnableVibration(0, true);
  ezInputDeviceXBox360::GetDevice()->EnableVibration(1, true);
  ezInputDeviceXBox360::GetDevice()->EnableVibration(2, true);
  ezInputDeviceXBox360::GetDevice()->EnableVibration(3, true);

  RegisterInputAction("Main", "ResetLevel", ezInputSlot_KeyReturn);

  // setup all controllers
  for (ezInt32 iPlayer = 0; iPlayer < MaxPlayers; ++iPlayer)
  {
    for (ezInt32 iAction = 0; iAction < MaxPlayerActions; ++iAction)
    {
      ezStringBuilder sAction;
      sAction.Printf("Player%i_%s", iPlayer, szPlayerActions[iAction]);

      ezStringBuilder sKey;
      sKey.Printf("controller%i_%s", iPlayer, szControlerKeys[iAction]);

      RegisterInputAction("Game", sAction.GetData(), sKey.GetData());
    }
  }

  // some more keyboard key bindings

  RegisterInputAction("Game", "Player1_Forwards", nullptr, ezInputSlot_KeyW);
  RegisterInputAction("Game", "Player1_Backwards", nullptr, ezInputSlot_KeyS);
  RegisterInputAction("Game", "Player1_Left", nullptr, ezInputSlot_KeyA);
  RegisterInputAction("Game", "Player1_Right", nullptr, ezInputSlot_KeyD);
  RegisterInputAction("Game", "Player1_Shoot", nullptr, ezInputSlot_KeySpace);
  RegisterInputAction("Game", "Player1_RotLeft", nullptr, ezInputSlot_KeyLeft);
  RegisterInputAction("Game", "Player1_RotRight", nullptr, ezInputSlot_KeyRight);
}

void AsteroidGameState::CreateGameLevel()
{
  m_pLevel = EZ_DEFAULT_NEW(Level);
  m_pLevel->SetupLevel(GetApplication()->CreateWorld("Asteroids - World", true));

  ChangeMainWorld(m_pLevel->GetWorld());
}

void AsteroidGameState::DestroyLevel()
{
  GetApplication()->DestroyWorld(m_pLevel->GetWorld());
  EZ_DEFAULT_DELETE(m_pLevel);
}

float AsteroidGameState::CanHandleThis(ezGameApplicationType AppType, ezWorld* pWorld) const
{
  return 1.0f;
}

void AsteroidGameState::ProcessInput()
{
  EZ_LOCK(m_pMainWorld->GetWriteMarker());

  for (ezInt32 iPlayer = 0; iPlayer < MaxPlayers; ++iPlayer)
    m_pLevel->UpdatePlayerInput(iPlayer);
}

