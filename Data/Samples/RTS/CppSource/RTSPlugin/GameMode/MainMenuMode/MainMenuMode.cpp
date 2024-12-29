#include <RTSPlugin/RTSPluginPCH.h>

#include <RTSPlugin/GameMode/MainMenuMode/MainMenuMode.h>
#include <RTSPlugin/GameState/RTSGameState.h>

RtsMainMenuMode::RtsMainMenuMode() = default;
RtsMainMenuMode::~RtsMainMenuMode() = default;

void RtsMainMenuMode::OnActivateMode() {}

void RtsMainMenuMode::OnDeactivateMode() {}

void RtsMainMenuMode::OnBeforeWorldUpdate()
{
  DisplaySelectModeUI();
}

void RtsMainMenuMode::OnProcessInput(const RtsMouseInputState& MouseInput) {}
