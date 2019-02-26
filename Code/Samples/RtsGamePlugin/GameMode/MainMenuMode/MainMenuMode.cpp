#include <RtsGamePluginPCH.h>

#include <RtsGamePlugin/GameMode/MainMenuMode/MainMenuMode.h>
#include <RtsGamePlugin/GameState/RtsGameState.h>

RtsMainMenuMode::RtsMainMenuMode() = default;
RtsMainMenuMode::~RtsMainMenuMode() = default;

void RtsMainMenuMode::OnActivateMode() {}

void RtsMainMenuMode::OnDeactivateMode() {}

void RtsMainMenuMode::OnBeforeWorldUpdate()
{
  DisplaySelectModeUI();
}

void RtsMainMenuMode::OnProcessInput(const RtsMouseInputState& MouseInput) {}
