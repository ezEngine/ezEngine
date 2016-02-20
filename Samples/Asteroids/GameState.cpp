#include "GameState.h"
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Time/Clock.h>
#include <Core/ResourceManager/ResourceManager.h>

#include <InputXBox360/InputDeviceXBox.h>

#include <RendererFoundation/Device/Device.h>

#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/SimpleRenderPass.h>
#include <RendererCore/Pipeline/TargetPass.h>
#include <RendererCore/RenderLoop/RenderLoop.h>

#include <Core/Application/Config/ApplicationConfig.h>

#include <GameFoundation/GameApplication/GameApplication.h>
#include <System/Window/Window.h>

EZ_CONSOLEAPP_ENTRY_POINT(ezGameApplication, ezGameApplicationType::StandAlone, "Shared/Samples/Asteroids");

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
EZ_END_DYNAMIC_REFLECTED_TYPE();

AsteroidGameState::AsteroidGameState()
{
  m_pLevel = nullptr;
}

void AsteroidGameState::OnActivation(ezWorld* pWorld)
{
  EZ_LOG_BLOCK("AsteroidGameState::Activate");

  ezGameState::OnActivation(pWorld);

  srand((ezUInt32)ezTime::Now().GetMicroseconds());

  CreateGameLevel();
}

void AsteroidGameState::OnDeactivation()
{
  EZ_LOG_BLOCK("AsteroidGameState::Deactivate");

  DestroyLevel();

  EZ_DEFAULT_DELETE(m_pThumbstick);
  EZ_DEFAULT_DELETE(m_pThumbstick2);

  ezGameState::OnDeactivation();
}

void AsteroidGameState::BeforeWorldUpdate()
{
  if (ezInputManager::GetInputActionState("Main", "ToggleThumbstick") == ezKeyState::Pressed)
  {
    m_pThumbstick->SetEnabled(!m_pThumbstick->IsEnabled());
    m_pThumbstick2->SetEnabled(!m_pThumbstick2->IsEnabled());
  }

  if (ezInputManager::GetInputActionState("Main", "ToggleMouseShow") == ezKeyState::Pressed)
  {
    m_pMainWindow->GetInputDevice()->SetShowMouseCursor(!m_pMainWindow->GetInputDevice()->GetShowMouseCursor());
  }

  if (ezInputManager::GetInputActionState("Main", "ToggleMouseClip") == ezKeyState::Pressed)
  {
    m_pMainWindow->GetInputDevice()->SetClipMouseCursor(!m_pMainWindow->GetInputDevice()->GetClipMouseCursor());
  }
}

void AsteroidGameState::ConfigureInputActions()
{
  ezInputDeviceXBox360::GetDevice()->EnableVibration(0, true);
  ezInputDeviceXBox360::GetDevice()->EnableVibration(1, true);
  ezInputDeviceXBox360::GetDevice()->EnableVibration(2, true);
  ezInputDeviceXBox360::GetDevice()->EnableVibration(3, true);

  RegisterInputAction("Main", "ResetLevel", ezInputSlot_KeyReturn);
  RegisterInputAction("Main", "ToggleThumbstick", ezInputSlot_KeyT);
  RegisterInputAction("Main", "ToggleMouseShow", ezInputSlot_KeyM);
  RegisterInputAction("Main", "ToggleMouseClip", ezInputSlot_KeyN);

  // setup all controllers
  for (ezInt32 iPlayer = 0; iPlayer < MaxPlayers; ++iPlayer)
  {
    for (ezInt32 iAction = 0; iAction < MaxPlayerActions; ++iAction)
    {
      ezStringBuilder sAction;
      sAction.Format("Player%i_%s", iPlayer, szPlayerActions[iAction]);

      ezStringBuilder sKey;
      sKey.Format("controller%i_%s", iPlayer, szControlerKeys[iAction]);

      RegisterInputAction("Game", sAction.GetData(), sKey.GetData());


    }
  }

  // some more keyboard key bindings

  RegisterInputAction("Game", "Player1_Forwards", nullptr, ezInputSlot_KeyW);
  RegisterInputAction("Game", "Player1_Backwards", nullptr, ezInputSlot_KeyS);
  RegisterInputAction("Game", "Player1_Left", nullptr, ezInputSlot_KeyA);
  RegisterInputAction("Game", "Player1_Right", nullptr, ezInputSlot_KeyD);
  RegisterInputAction("Game", "Player1_Shoot", nullptr, ezInputSlot_KeySpace);
  RegisterInputAction("Game", "Player1_RotLeft", nullptr, ezInputSlot_KeyLeft, ezInputSlot_MouseMoveNegX);
  RegisterInputAction("Game", "Player1_RotRight", nullptr, ezInputSlot_KeyRight, ezInputSlot_MouseMovePosX);

  //RegisterInputAction("Game", "Player3_Forwards",   nullptr, ezInputSlot_MouseMoveNegY);
  //RegisterInputAction("Game", "Player3_Backwards",  nullptr, ezInputSlot_MouseMovePosY);
  //RegisterInputAction("Game", "Player3_Left",       nullptr, ezInputSlot_MouseMoveNegX);
  //RegisterInputAction("Game", "Player3_Right",      nullptr, ezInputSlot_MouseMovePosX);
  //RegisterInputAction("Game", "Player3_Shoot",      nullptr, ezInputSlot_MouseButton2);
  //RegisterInputAction("Game", "Player3_RotLeft",    nullptr, ezInputSlot_MouseButton0);
  //RegisterInputAction("Game", "Player3_RotRight",   nullptr, ezInputSlot_MouseButton1);

  m_pThumbstick = EZ_DEFAULT_NEW(ezVirtualThumbStick);
  m_pThumbstick->SetInputArea(ezVec2(0.1f, 0.1f), ezVec2(0.3f, 0.3f), 0.1f, 0.0f);
  m_pThumbstick->SetTriggerInputSlot(ezVirtualThumbStick::Input::Touchpoint);
  m_pThumbstick->SetThumbstickOutput(ezVirtualThumbStick::Output::Controller0_LeftStick);
  m_pThumbstick->SetEnabled(false);

  m_pThumbstick2 = EZ_DEFAULT_NEW(ezVirtualThumbStick);
  m_pThumbstick2->SetInputArea(ezVec2(0.2f, 0.1f), ezVec2(0.4f, 0.4f), 0.1f, 0.0f);
  m_pThumbstick2->SetTriggerInputSlot(ezVirtualThumbStick::Input::Touchpoint);
  m_pThumbstick2->SetThumbstickOutput(ezVirtualThumbStick::Output::Controller0_RightStick);
  m_pThumbstick2->SetEnabled(false);
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


