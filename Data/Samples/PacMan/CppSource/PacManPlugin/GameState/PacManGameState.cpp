#include <PacManPlugin/PacManPluginPCH.h>

#include <Core/Input/InputManager.h>
#include <Core/System/Window.h>
#include <Core/Utils/Blackboard.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Logging/Log.h>
#include <PacManPlugin/GameState/PacManGameState.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/MeshComponent.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(PacManGameState, 1, ezRTTIDefaultAllocator<PacManGameState>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

PacManGameState::PacManGameState() = default;
PacManGameState::~PacManGameState() = default;

void PacManGameState::OnActivation(ezWorld* pWorld, const ezTransform* pStartPosition)
{
  // this is called shortly after the game state was created, and before the game starts to properly run
  // so here you could do general startup stuff

  EZ_LOG_BLOCK("GameState::Activate");

  SUPER::OnActivation(pWorld, pStartPosition);

  ResetState();
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
  ezHashedString hs;
  hs.Assign("Stats");
  auto pBlackboard = ezBlackboard::GetOrCreateGlobal(hs);

  const ezInt32 iNumCoinsFound = pBlackboard->GetEntryValue(ezTempHashedString("CoinsEaten"), 0).Get<ezInt32>();
  const ezInt32 iPacManState = pBlackboard->GetEntryValue(ezTempHashedString("PacManState"), 1).Get<ezInt32>();

  ezDebugRenderer::DrawInfoText(m_pMainWorld, ezDebugTextPlacement::TopCenter, "Stats", ezFmt("Coins: {} / {}", iNumCoinsFound, m_uiNumCoinsTotal));

  if (iPacManState == PacManState::Alive && m_uiNumCoinsTotal > 0 && iNumCoinsFound == m_uiNumCoinsTotal)
  {
    // let the ghosts and PacMan know when he ate all the coins
    pBlackboard->SetEntryValue(ezTempHashedString("PacManState"), PacManState::WonGame).AssertSuccess();

    // play a sound, the GUID of the sound asset was copied from the editor
    ezSoundInterface::PlaySound("{ a10b9065-0b4d-4eff-a9ac-2f712dc28c1c }", ezTransform::MakeIdentity()).IgnoreResult();
  }

  if (iPacManState == PacManState::EatenByGhost)
  {
    ezDebugRenderer::DrawInfoText(m_pMainWorld, ezDebugTextPlacement::TopCenter, "Stats", "YOU LOSE!\n\nPress SPACE to play again.", ezColor::Red);
  }

  if (iPacManState == PacManState::WonGame)
  {
    ezDebugRenderer::DrawInfoText(m_pMainWorld, ezDebugTextPlacement::TopCenter, "Stats", "YOU WIN!\n\nPress SPACE to play again.", ezColor::LightPink);
  }
}


ezResult PacManGameState::SpawnPlayer(const ezTransform* pStartPosition)
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

  ezHashedString hs;
  hs.Assign("Stats");
  auto pBlackboard = ezBlackboard::GetOrCreateGlobal(hs);

  hs.Assign("CoinsEaten");
  pBlackboard->RegisterEntry(hs, 0);

  hs.Assign("PacManState");
  pBlackboard->RegisterEntry(hs, PacManState::Alive);

  // 'reset' the state, if the values were already registered before
  pBlackboard->SetEntryValue(ezTempHashedString("CoinsEaten"), 0).AssertSuccess();
  pBlackboard->SetEntryValue(ezTempHashedString("PacManState"), PacManState::Alive).AssertSuccess();
}

ezGameStatePriority PacManGameState::DeterminePriority(ezWorld* pWorld) const
{
  // always return ezGameStatePriority::Default for 'normal' game states
  return ezGameStatePriority::Default;
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
  RegisterInputAction("Game", "Reset", ezInputSlot_KeySpace, ezInputSlot_Controller0_ButtonStart);
}

void PacManGameState::ProcessInput()
{
  SUPER::ProcessInput();

  if (ezInputManager::GetInputActionState("Game", "Reset") == ezKeyState::Released)
  {
    ResetState();

    // We just kick off a scene load. The 'scene file' is the asset GUID of the 'Level1.ezScene' document.
    StartSceneLoading("{ 92c25a40-7218-9708-2da5-4b75040dc3bd }", {}).IgnoreResult();

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
