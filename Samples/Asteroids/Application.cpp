#include "Application.h"
#include "Window.h"
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Time/Clock.h>
#include <Core/ResourceManager/ResourceManager.h>

#include <InputXBox360/InputDeviceXBox.h>

#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/SimpleRenderPass.h>
#include <RendererCore/RenderLoop/RenderLoop.h>

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

SampleGameApp::SampleGameApp()
{
  m_pLevel = nullptr;
  m_pWindow = nullptr;
}

void SampleGameApp::AfterEngineInit()
{
  EZ_LOG_BLOCK("SampleGameApp::AfterEngineInit");

  ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
  ezFileSystem::AddDataDirectory(ezOSFile::GetApplicationDirectory());

  ezStringBuilder sReadDir = BUILDSYSTEM_OUTPUT_FOLDER;
  sReadDir.AppendPath("../../Shared/Samples/Asteroids/");

  ezFileSystem::AddDataDirectory(sReadDir.GetData(), ezFileSystem::AllowWrites, "Asteroids Content");

  if (ezPlugin::LoadPlugin("ezInspectorPlugin") == EZ_SUCCESS)
  {

  }

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  EZ_VERIFY(ezPlugin::LoadPlugin("ezShaderCompilerHLSL").Succeeded(), "Compiler Plugin not found");
#endif

  // Setup the logging system
  ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

  ezGameApplication::Initialize();

  m_pWindow = EZ_DEFAULT_NEW(GameWindow);
  ezGALSwapChainHandle hSwapChain = AddWindow(m_pWindow);

  ezStartup::StartupEngine();

  // Map the input keys to actions
  SetupInput();

  srand((ezUInt32)ezTime::Now().GetMicroseconds());

  const ezGALSwapChain* pSwapChain = ezGALDevice::GetDefaultDevice()->GetSwapChain(hSwapChain);
  CreateGameLevelAndRenderPipeline(pSwapChain->GetRenderTargetViewConfig());
}

void SampleGameApp::BeforeEngineShutdown()
{
  EZ_LOG_BLOCK("SampleGameApp::BeforeEngineShutdown");

  DestroyLevelAndRenderPipeline();

  ezStartup::ShutdownEngine();

  ezGameApplication::Deinitialize();

  EZ_DEFAULT_DELETE(m_pWindow);

  EZ_DEFAULT_DELETE(m_pThumbstick);
  EZ_DEFAULT_DELETE(m_pThumbstick2);
}

void SampleGameApp::UpdateInput()
{
  ezGameApplication::UpdateInput();

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

void SampleGameApp::SetupInput()
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

void SampleGameApp::CreateGameLevelAndRenderPipeline(ezGALRenderTargetConfigHandle hRTConfig)
{
  m_pLevel = EZ_DEFAULT_NEW(Level);
  m_pLevel->SetupLevel(EZ_DEFAULT_NEW(ezWorld)("Asteroids - World"));

  ezView* pView = ezRenderLoop::CreateView("Asteroids - View");
  ezRenderLoop::AddMainView(pView);

  ezRenderPipeline* pRenderPipeline = EZ_DEFAULT_NEW(ezRenderPipeline)();
  pRenderPipeline->AddPass(EZ_DEFAULT_NEW(ezSimpleRenderPass)(hRTConfig));
  pView->SetRenderPipeline(pRenderPipeline);

  ezSizeU32 size = m_pWindow->GetClientAreaSize();
  pView->SetViewport(ezRectFloat(0.0f, 0.0f, (float)size.width, (float)size.height));

  pView->SetWorld(m_pLevel->GetWorld());
  pView->SetLogicCamera(m_pLevel->GetCamera());
}

void SampleGameApp::DestroyLevelAndRenderPipeline()
{
  auto views = ezRenderLoop::GetMainViews();
  for (auto pView : views)
  {
    ezRenderPipeline* pRenderPipeline = pView->GetRenderPipeline();
    
    EZ_DEFAULT_DELETE(pRenderPipeline);

    EZ_DEFAULT_DELETE(pView);
  }

  ezRenderLoop::ClearMainViews();

  EZ_DEFAULT_DELETE(m_pLevel);
}

EZ_CONSOLEAPP_ENTRY_POINT(SampleGameApp);
