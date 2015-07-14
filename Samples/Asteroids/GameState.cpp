#include "GameState.h"
#include "Window.h"
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Time/Clock.h>
#include <Core/ResourceManager/ResourceManager.h>

#include <InputXBox360/InputDeviceXBox.h>

#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Pipeline/SimpleRenderPass.h>
#include <RendererCore/RenderLoop/RenderLoop.h>
#include <Core/Application/Config/ApplicationConfig.h>

#include <GameFoundation/GameApplication.h>

EZ_CONSOLEAPP_ENTRY_POINT(ezGameApplication, *EZ_DEFAULT_NEW(AsteroidGameState));

const char* szPlayerActions[MaxPlayerActions] = { "Forwards", "Backwards", "Left", "Right", "RotLeft", "RotRight", "Shoot" };
const char* szControlerKeys[MaxPlayerActions] = { "leftstick_posy", "leftstick_negy", "leftstick_negx", "leftstick_posx", "rightstick_negx", "rightstick_posx", "right_trigger" };

namespace
{
  ezCVarInt CVarInput("CVar_Input", 0, ezCVarFlags::Default, "Bla bla");

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

AsteroidGameState::AsteroidGameState()
{
  m_pLevel = nullptr;
  m_pWindow = nullptr;
}

void AsteroidGameState::Activate()
{
  EZ_LOG_BLOCK("AsteroidGameState::Activate");

  ezStringBuilder sBaseDir = BUILDSYSTEM_OUTPUT_FOLDER;
  sBaseDir.AppendPath("../../Shared/Data/");

  ezStringBuilder sSharedDir = BUILDSYSTEM_OUTPUT_FOLDER;
  sSharedDir.AppendPath("../../Shared/FreeContent/");

  ezStringBuilder sProjectDir = BUILDSYSTEM_OUTPUT_FOLDER;
  sProjectDir.AppendPath("../../Shared/Samples/Asteroids");

  // setup the 'asset management system'
  {
    // which redirection table to search
    ezDataDirectory::FolderType::s_sRedirectionFile = "AssetCache/LookupTable.ezAsset";
    // which platform assets to use
    ezDataDirectory::FolderType::s_sRedirectionPrefix = "AssetCache/PC/";
  }

  ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
  ezFileSystem::AddDataDirectory("");
  ezFileSystem::AddDataDirectory(sBaseDir.GetData(), ezFileSystem::ReadOnly, "Base");
  ezFileSystem::AddDataDirectory(sSharedDir.GetData(), ezFileSystem::ReadOnly, "Shared");
  ezFileSystem::AddDataDirectory(sProjectDir.GetData(), ezFileSystem::AllowWrites, "Project");

  GetApplication()->SetupProject(sProjectDir);

  m_pWindow = EZ_DEFAULT_NEW(GameWindow);
  ezGALSwapChainHandle hSwapChain = GetApplication()->AddWindow(m_pWindow);

  // Map the input keys to actions
  SetupInput();

  srand((ezUInt32)ezTime::Now().GetMicroseconds());

  const ezGALSwapChain* pSwapChain = ezGALDevice::GetDefaultDevice()->GetSwapChain(hSwapChain);
  CreateGameLevelAndRenderPipeline(pSwapChain->GetBackBufferRenderTargetView(), pSwapChain->GetDepthStencilTargetView());
}

void AsteroidGameState::Deactivate()
{
  EZ_LOG_BLOCK("AsteroidGameState::Deactivate");

  DestroyLevel();

  EZ_DEFAULT_DELETE(m_pWindow);

  EZ_DEFAULT_DELETE(m_pThumbstick);
  EZ_DEFAULT_DELETE(m_pThumbstick2);
}

void AsteroidGameState::BeforeWorldUpdate()
{
  if (ezInputManager::GetInputActionState("Main", "Assert") == ezKeyState::Pressed)
  {
    ezLog::Info("Asserting");

    EZ_ASSERT_DEV(false, "This is safe to ignore.");
  }

  if (ezInputManager::GetInputActionState("Main", "CVarUp") == ezKeyState::Pressed)
  {
    CVarInput = CVarInput + 1;
  }

  if (ezInputManager::GetInputActionState("Main", "CVarDown") == ezKeyState::Pressed)
  {
    CVarInput = CVarInput - 1;
  }

  if (ezInputManager::GetInputActionState("Main", "ToggleThumbstick") == ezKeyState::Pressed)
  {
    m_pThumbstick->SetEnabled(!m_pThumbstick->IsEnabled());
    m_pThumbstick2->SetEnabled(!m_pThumbstick2->IsEnabled());
  }

  if (ezInputManager::GetInputActionState("Main", "ToggleMouseShow") == ezKeyState::Pressed)
  {
    m_pWindow->GetInputDevice()->SetShowMouseCursor(!m_pWindow->GetInputDevice()->GetShowMouseCursor());
  }

  if (ezInputManager::GetInputActionState("Main", "ToggleMouseClip") == ezKeyState::Pressed)
  {
    m_pWindow->GetInputDevice()->SetClipMouseCursor(!m_pWindow->GetInputDevice()->GetClipMouseCursor());
  }
}

void AsteroidGameState::SetupInput()
{
  ezInputDeviceXBox360::GetDevice()->EnableVibration(0, true);
  ezInputDeviceXBox360::GetDevice()->EnableVibration(1, true);
  ezInputDeviceXBox360::GetDevice()->EnableVibration(2, true);
  ezInputDeviceXBox360::GetDevice()->EnableVibration(3, true);

  m_pWindow->GetInputDevice()->SetClipMouseCursor(true);
  m_pWindow->GetInputDevice()->SetShowMouseCursor(false);
  m_pWindow->GetInputDevice()->SetMouseSpeed(ezVec2(0.002f));

  RegisterInputAction("Main", "ResetLevel", ezInputSlot_KeyReturn);
  RegisterInputAction("Main", "ToggleThumbstick", ezInputSlot_KeyT);
  RegisterInputAction("Main", "Assert", ezInputSlot_KeyNumpadEnter);
  RegisterInputAction("Main", "CVarDown", ezInputSlot_KeyO);
  RegisterInputAction("Main", "CVarUp", ezInputSlot_KeyP);
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

void AsteroidGameState::CreateGameLevelAndRenderPipeline(ezGALRenderTargetViewHandle hBackBuffer, ezGALRenderTargetViewHandle hDSV)
{
  m_pLevel = EZ_DEFAULT_NEW(Level);
  m_pLevel->SetupLevel(EZ_DEFAULT_NEW(ezWorld, "Asteroids - World"));

  ezView* pView = ezRenderLoop::CreateView("Asteroids - View");
  ezRenderLoop::AddMainView(pView);

  ezGALRenderTagetSetup RTS;
  RTS.SetRenderTarget(0, hBackBuffer)
     .SetDepthStencilTarget(hDSV);

  ezUniquePtr<ezRenderPipeline> pRenderPipeline = EZ_DEFAULT_NEW(ezRenderPipeline);
  pRenderPipeline->AddPass(EZ_DEFAULT_NEW( ezSimpleRenderPass, RTS));
  pView->SetRenderPipeline(std::move(pRenderPipeline));

  ezSizeU32 size = m_pWindow->GetClientAreaSize();
  pView->SetViewport(ezRectFloat(0.0f, 0.0f, (float)size.width, (float)size.height));

  pView->SetWorld(m_pLevel->GetWorld());
  pView->SetLogicCamera(m_pLevel->GetCamera());
}

void AsteroidGameState::DestroyLevel()
{
  EZ_DEFAULT_DELETE(m_pLevel);
}
