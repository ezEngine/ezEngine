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
  EZ_LOG_BLOCK("GameState::Activate");

  SUPER::OnActivation(pWorld, pStartPosition);

  ResetState();
}

void PacManGameState::OnDeactivation()
{
  EZ_LOG_BLOCK("GameState::Deactivate");

  SUPER::OnDeactivation();
}

void PacManGameState::AfterWorldUpdate()
{
  SUPER::AfterWorldUpdate();

  if (m_pMainWorld)
  {
    if (m_uiNumCoinsTotal == 0)
    {
      EZ_LOCK(m_pMainWorld->GetWriteMarker());

      for (auto it = m_pMainWorld->GetObjects(); it.IsValid(); ++it)
      {
        if (it->GetName() == "Coin")
        {
          ++m_uiNumCoinsTotal;
        }
      }
    }

    ezHashedString hs;
    hs.Assign("Stats");
    auto pBlackboard = ezBlackboard::GetOrCreateGlobal(hs);

    const ezInt32 iNumCoinsFound = pBlackboard->GetEntryValue(ezTempHashedString("CoinsEaten"), 0).Get<ezInt32>();
    const ezInt32 iPacManState = pBlackboard->GetEntryValue(ezTempHashedString("PacManState"), 1).Get<ezInt32>();

    ezDebugRenderer::DrawInfoText(m_pMainWorld, ezDebugRenderer::ScreenPlacement::TopCenter, "Stats", ezFmt("Coins: {} / {}", iNumCoinsFound, m_uiNumCoinsTotal));

    if (iPacManState == 1 && m_uiNumCoinsTotal > 0 && iNumCoinsFound == m_uiNumCoinsTotal)
    {
      pBlackboard->SetEntryValue(ezTempHashedString("PacManState"), 2).AssertSuccess();
    }

    if (iPacManState == 0)
    {
      ezDebugRenderer::DrawInfoText(m_pMainWorld, ezDebugRenderer::ScreenPlacement::TopCenter, "Stats", "YOU LOSE!\n\nPress SPACE to play again.", ezColor::Red);
    }

    if (iPacManState >= 2)
    {
      ezDebugRenderer::DrawInfoText(m_pMainWorld, ezDebugRenderer::ScreenPlacement::TopCenter, "Stats", "YOU WIN!\n\nPress SPACE to play again.", ezColor::LightPink);
    }
  }
}


ezResult PacManGameState::SpawnPlayer(const ezTransform* pStartPosition)
{
  ResetState();
  return EZ_SUCCESS;
}

void PacManGameState::ResetState()
{
  m_uiNumCoinsTotal = 0;

  ezHashedString hs;
  hs.Assign("Stats");
  auto pBlackboard = ezBlackboard::GetOrCreateGlobal(hs);

  hs.Assign("CoinsEaten");
  pBlackboard->RegisterEntry(hs, 0);

  hs.Assign("PacManState");
  pBlackboard->RegisterEntry(hs, 1);

  // 'reset' the state, if the values were already registered before
  pBlackboard->SetEntryValue(ezTempHashedString("CoinsEaten"), 0).AssertSuccess();
  pBlackboard->SetEntryValue(ezTempHashedString("PacManState"), 1).AssertSuccess();
}

void PacManGameState::BeforeWorldUpdate()
{
  EZ_LOCK(m_pMainWorld->GetWriteMarker());
}

ezGameStatePriority PacManGameState::DeterminePriority(ezWorld* pWorld) const
{
  return ezGameStatePriority::Default;
}

void PacManGameState::ConfigureMainWindowInputDevices(ezWindow* pWindow)
{
  SUPER::ConfigureMainWindowInputDevices(pWindow);

  // setup devices here
}

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
  SUPER::ConfigureInputActions();

  RegisterInputAction("Game", "Reset", ezInputSlot_KeySpace);
}

void PacManGameState::ProcessInput()
{
  SUPER::ProcessInput();

  if (ezInputManager::GetInputActionState("Game", "Reset") == ezKeyState::Released)
  {
    ResetState();
    StartSceneLoading("{ 92c25a40-7218-9708-2da5-4b75040dc3bd }", {}).IgnoreResult();

    // scene loading happens in the background, and once it is ready, will switch automatically to the new scene
  }
}

void PacManGameState::ConfigureMainCamera()
{
  SUPER::ConfigureMainCamera();

  // do custom camera setup here
}
