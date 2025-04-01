#include <PacManPlugin/PacManPluginPCH.h>

#include <Core/Input/InputManager.h>
#include <Core/Interfaces/SoundInterface.h>
#include <Core/System/Window.h>
#include <Core/Utils/Blackboard.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Logging/Log.h>
#include <GameEngine/Input/InputDebugVis.h>
#include <PacManPlugin/GameState/PacManGameState.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/MeshComponent.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(PacManGameState, 1, ezRTTIDefaultAllocator<PacManGameState>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

PacManGameState::PacManGameState() = default;
PacManGameState::~PacManGameState() = default;

ezHashedString PacManGameState::s_sStats = ezMakeHashedString("Stat");
ezHashedString PacManGameState::s_sCoinsEaten = ezMakeHashedString("CoinsEaten");
ezHashedString PacManGameState::s_sPacManState = ezMakeHashedString("PacManState");

ezString PacManGameState::GetStartupSceneFile()
{
  // if we have a "-scene" command line argument, it was launched from the editor and we should load that
  // otherwise, we use the hardcoded 'Main.ezScene' of the PacMan project
  if (ezCommandLineUtils::GetGlobalInstance()->HasOption("-scene"))
  {
    return ezCommandLineUtils::GetGlobalInstance()->GetStringOption("-scene");
  }

  return "AssetCache/Common/Scenes/Main.ezBinScene";
}

void PacManGameState::OnActivation(ezWorld* pWorld, ezStringView sStartPosition, const ezTransform& startPositionOffset)
{
  // this is called shortly after the game state was created, and before the game starts to properly run
  // so here you could do general startup stuff

  EZ_LOG_BLOCK("GameState::Activate");

  SUPER::OnActivation(pWorld, sStartPosition, startPositionOffset);

  ResetState();

  {
    m_pLeftStick = EZ_DEFAULT_NEW(ezVirtualThumbStick);
    m_pLeftStick->SetInputArea(ezVec2(0, 0), ezVec2(0.3f, 1), 0.07f, 1.0f, ezVirtualThumbStick::CenterMode::Swipe);
    m_pLeftStick->SetFlags(ezVirtualThumbStick::Flags::OnlyMaxAxis);
    m_pLeftStick->SetTriggerInputSlot(ezVirtualThumbStick::Input::Touchpoint);
    m_pLeftStick->SetThumbstickOutput(ezVirtualThumbStick::Output::Controller0_LeftStick);
    m_pLeftStick->SetAreaFocusMode(ezInputActionConfig::OnEnterArea::ActivateImmediately, ezInputActionConfig::OnLeaveArea::KeepFocus);
    m_pLeftStick->SetEnabled(false);
  }
  {
    m_pRightStick = EZ_DEFAULT_NEW(ezVirtualThumbStick);
    m_pRightStick->SetInputArea(ezVec2(0.8f, 0), ezVec2(1.0f, 0.2f), 0.05f, 0.0f, ezVirtualThumbStick::CenterMode::InputArea);
    m_pRightStick->SetTriggerInputSlot(ezVirtualThumbStick::Input::Touchpoint);
    m_pRightStick->SetThumbstickOutput(ezVirtualThumbStick::Output::Controller0_RightStick);
    m_pRightStick->SetAreaFocusMode(ezInputActionConfig::OnEnterArea::RequireKeyUp, ezInputActionConfig::OnLeaveArea::LoseFocus);
    m_pRightStick->SetEnabled(false);
  }

  if (ezSoundInterface* pSoundInterface = ezSingletonRegistry::GetSingletonInstance<ezSoundInterface>())
  {
    // adjust the volume of the sound groups
    // this would usually be a user setting
    pSoundInterface->SetSoundGroupVolume("Music", 0.7f);
    pSoundInterface->SetSoundGroupVolume("Effects", 0.9f);
  }
}


void PacManGameState::OnDeactivation()
{
  // this is run when the game is shutting down

  EZ_LOG_BLOCK("GameState::Deactivate");

  SUPER::OnDeactivation();
}

void PacManGameState::AfterWorldUpdate()
{
  // this is called once each frame after the ezWorld got updated
  // here we use it to evaluate the current state and to also draw some text on screen
  // all of this could also be done in ProcessInput() instead, especially since the debug-drawing can be done at any time during the frame
  // but in a more complex game you may want to do some things right after the world update

  SUPER::AfterWorldUpdate();

  if (!m_pMainWorld)
    return;

  if (m_uiNumCoinsTotal == 0)
  {
    // we don't know the number of coins in the scene yet, so lets iterate over all objects and count how many coins we find

    EZ_LOCK(m_pMainWorld->GetWriteMarker());

    for (auto it = m_pMainWorld->GetObjects(); it.IsValid(); ++it)
    {
      // we just use the name of the objects to determine that this is a coin
      if (it->GetName() == "Coin")
      {
        ++m_uiNumCoinsTotal;
      }
    }
  }

  // get the global blackboard in which we track the state
  auto pBlackboard = ezBlackboard::GetOrCreateGlobal(s_sStats);

  const ezInt32 iNumCoinsFound = pBlackboard->GetEntryValue(s_sCoinsEaten, 0).Get<ezInt32>();
  const ezInt32 iPacManState = pBlackboard->GetEntryValue(s_sPacManState, 1).Get<ezInt32>();

  ezDebugRenderer::DrawInfoText(m_pMainWorld, ezDebugTextPlacement::TopCenter, "Stats", ezFmt("Coins: {} / {}", iNumCoinsFound, m_uiNumCoinsTotal));

  if (iPacManState == PacManState::Alive && m_uiNumCoinsTotal > 0 && iNumCoinsFound == m_uiNumCoinsTotal)
  {
    // let the ghosts and PacMan know when he ate all the coins
    pBlackboard->SetEntryValue(s_sPacManState, PacManState::WonGame);

    // play a sound, the GUID of the sound asset was copied from the editor
    // ezSoundInterface::PlaySound("{ a10b9065-0b4d-4eff-a9ac-2f712dc28c1c }", ezTransform::MakeIdentity()).IgnoreResult(); // FMOD
    ezSoundInterface::PlaySound(m_pMainWorld, "{ 2281d82a-cf87-4747-b664-a41ebc74c052 }", ezTransform::MakeIdentity()).IgnoreResult(); // MiniAudio
  }

  if (iPacManState == PacManState::EatenByGhost)
  {
    if (m_bTouchInput)
    {
      ezDebugRenderer::DrawInfoText(m_pMainWorld, ezDebugTextPlacement::TopCenter, "Stats", "YOU LOST!\n\nSwipe top-right screen to play again.", ezColor::LightPink);
    }
    else
    {
      ezDebugRenderer::DrawInfoText(m_pMainWorld, ezDebugTextPlacement::TopCenter, "Stats", "YOU LOST!\n\nPress SPACE to play again.", ezColor::Red);
    }
  }

  if (iPacManState == PacManState::WonGame)
  {
    if (m_bTouchInput)
    {
      ezDebugRenderer::DrawInfoText(m_pMainWorld, ezDebugTextPlacement::TopCenter, "Stats", "YOU WIN!\n\nSwipe top-right screen to play again.", ezColor::LightPink);
    }
    else
    {
      ezDebugRenderer::DrawInfoText(m_pMainWorld, ezDebugTextPlacement::TopCenter, "Stats", "YOU WIN!\n\nnPress SPACE to play again", ezColor::LightPink);
    }
  }

  {
    if (ezInputManager::GetInputSlotState(ezInputManager::GetInputSlotTouchPoint(0)) == ezKeyState::Down)
    {
      m_bTouchInput = true;
      m_pLeftStick->SetEnabled(true);
      m_pRightStick->SetEnabled(true);
    }

    if (m_bTouchInput)
    {
      ezDebugRenderer::DrawInfoText(m_pMainWorld, ezDebugTextPlacement::TopLeft, "Manual", "Swipe left screen area to steer.");
      ezDebugRenderer::DrawInfoText(m_pMainWorld, ezDebugTextPlacement::TopRight, "Manual", "Swipe top-right screen to reset.");
    }

    ezView* pView = nullptr;
    if (ezRenderWorld::TryGetView(m_hMainView, pView))
    {
      const ezRectFloat viewport = pView->GetViewport();
      const ezVec2 resolution = viewport.GetExtents();

      m_pLeftStick->SetInputCoordinateAspectRatio(resolution.x / resolution.y);
      m_pRightStick->SetInputCoordinateAspectRatio(resolution.x / resolution.y);

      ezInputDebugVis::DebugRender(m_pMainWorld, resolution, *m_pLeftStick);
      ezInputDebugVis::DebugRender(m_pMainWorld, resolution, *m_pRightStick);
    }
  }
}


ezResult PacManGameState::SpawnPlayer(ezStringView sStartPosition, const ezTransform& startPositionOffset)
{
  // this is called every time we switch to a new scene
  // some games may want to create the 'player object' here
  // since our game always already has a player object, we don't need to do anything like that here
  // but since it is also called when we reset the scene, it is a good point in time to reset the current state

  ResetState();
  return EZ_SUCCESS;
}

void PacManGameState::ResetState()
{
  // we use a global blackboard to store the overall state of the game (https://ezengine.net/pages/docs/Miscellaneous/blackboards.html)

  m_uiNumCoinsTotal = 0;

  auto pBlackboard = ezBlackboard::GetOrCreateGlobal(s_sStats);

  // 'reset' the state
  pBlackboard->SetEntryValue(s_sCoinsEaten, 0);
  pBlackboard->SetEntryValue(s_sPacManState, PacManState::Alive);
}

// a helper function to bind one or several keys to an input action
static void RegisterInputAction(const char* szInputSet, const char* szInputAction, const char* szKey1, const char* szKey2 = nullptr, const char* szKey3 = nullptr)
{
  ezInputActionConfig cfg;
  cfg.m_bApplyTimeScaling = true;
  cfg.m_sInputSlotTrigger[0] = szKey1;
  cfg.m_sInputSlotTrigger[1] = szKey2;
  cfg.m_sInputSlotTrigger[2] = szKey3;

  ezInputManager::SetInputActionConfig(szInputSet, szInputAction, cfg, true);
}

void PacManGameState::ConfigureInputActions()
{
  // this function is called once at startup
  // here we can add additional input actions that we want to handle on the game-state level
  // see https://ezengine.net/pages/docs/input/input-overview.html

  SUPER::ConfigureInputActions();

  // we want to be able to reset the game to the start state, using the spacebar
  RegisterInputAction("Game", "Reset", ezInputSlot_KeySpace, ezInputSlot_Controller0_ButtonStart, ezInputSlot_Controller0_RightStick_PosX);
}

void PacManGameState::ProcessInput()
{
  SUPER::ProcessInput();

  if (ezInputManager::GetInputActionState("Game", "Reset") == ezKeyState::Released)
  {
    ResetState();

    // We just kick off a scene load. The 'scene file' is the asset GUID of the 'Level1.ezScene' document.
    LoadScene(GetStartupSceneFile(), {}, {}, ezTransform::MakeIdentity());

    // scene loading happens in the background, and once it is ready, will switch automatically to the new scene
  }
}

void PacManGameState::ConfigureMainCamera()
{
  SUPER::ConfigureMainCamera();

  // we use a fixed camera from the level, so we don't need to setup a custom camera from code
  // but if we wanted, we could ignore the camera from the scene and create our own camera here
  // and then update it in ProcessInput()
}


EZ_STATICLINK_FILE(PacManPlugin, PacManPlugin_GameState_PacManGameState);
