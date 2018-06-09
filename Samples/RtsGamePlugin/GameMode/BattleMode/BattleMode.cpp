#include <PCH.h>
#include <RtsGamePlugin/GameMode/BattleMode/BattleMode.h>
#include <GameEngine/DearImgui/DearImgui.h>
#include <Core/Input/InputManager.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RtsGamePlugin/GameState/RtsGameState.h>

RtsBattleMode::RtsBattleMode() = default;
RtsBattleMode::~RtsBattleMode() = default;

void RtsBattleMode::OnActivateMode()
{
}

void RtsBattleMode::OnDeactivateMode()
{
}

void RtsBattleMode::OnBeforeWorldUpdate()
{
}

void RtsBattleMode::RegisterInputActions()
{
}

void RtsBattleMode::OnProcessInput()
{
  DoDefaultCameraInput();

}
