#include <RTSPlugin/RTSPluginPCH.h>

#include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#include <Foundation/System/Screen.h>
#include <RTSPlugin/GameMode/MainMenuMode/MainMenuMode.h>
#include <RTSPlugin/GameState/RTSGameState.h>
#include <RmlUiPlugin/Components/RmlUiCanvas2DComponent.h>
#include <RmlUiPlugin/RmlUiContext.h>

RtsMainMenuMode::RtsMainMenuMode() = default;
RtsMainMenuMode::~RtsMainMenuMode() = default;

void RtsMainMenuMode::OnActivateMode()
{
  EZ_LOCK(m_pMainWorld->GetWriteMarker());

  SetUiActive(m_pMainWorld, ezTempHashedString("app-menu"), true);

  ezGameObject* pUIObject = nullptr;
  if (m_pMainWorld->TryGetObjectWithGlobalKey(ezTempHashedString("app-menu"), pUIObject))
  {
    ezRmlUiCanvas2DComponent* pUiComponent = nullptr;
    if (pUIObject->TryGetComponentOfBaseType(pUiComponent))
    {
      m_hMainMenu = pUiComponent->GetHandle();

      ezRmlUiContext* pRmlContext = pUiComponent->GetOrCreateRmlContext();

      pRmlContext->RegisterEventHandler("Game-Resume", [this](Rml::Event& e)
        { m_pGameState->SwitchToGameMode(RtsActiveGameMode::EditLevelMode); });

      pRmlContext->RegisterEventHandler("Game-Settings", [this](Rml::Event& e)
        { m_pGameState->SwitchToGameMode(RtsActiveGameMode::SettingsMenuMode); });

      pRmlContext->RegisterEventHandler("Game-Exit", [this](Rml::Event& e)
        { m_pGameState->RequestQuit("game"); });
    }
  }
}

void RtsMainMenuMode::OnDeactivateMode()
{
  EZ_LOCK(m_pMainWorld->GetWriteMarker());

  SetUiActive(m_pMainWorld, ezTempHashedString("app-menu"), false);
}

void RtsMainMenuMode::OnBeforeWorldUpdate()
{
}

void RtsMainMenuMode::OnProcessInput(const RtsMouseInputState& MouseInput, bool bUiWantsInput)
{
  if (ezInputManager::GetInputSlotState(ezInputSlot_KeyEscape) == ezKeyState::Pressed)
  {
    m_pGameState->SwitchToGameMode(RtsActiveGameMode::EditLevelMode);
  }
}
