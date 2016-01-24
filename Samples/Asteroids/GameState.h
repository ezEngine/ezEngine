#pragma once

#include "Level.h"
#include <Core/Input/VirtualThumbStick.h>
#include <GameFoundation/GameState.h>

class GameWindow;

class AsteroidGameState : public ezGameState
{
  EZ_ADD_DYNAMIC_REFLECTION(AsteroidGameState, ezGameState);

public:
  AsteroidGameState();

private:
  virtual void Activate() override;
  virtual void Deactivate() override;
  virtual void BeforeWorldUpdate() override;

  void SetupInput();
  void CreateGameLevelAndRenderPipeline(ezGALRenderTargetViewHandle hBackBuffer, ezGALRenderTargetViewHandle hDSV);
  void DestroyLevel();

  virtual ezGameStateCanHandleThis CanHandleThis(ezGameApplicationType AppType, ezWorld* pWorld) const override;

  Level* m_pLevel;
  GameWindow* m_pWindow;

  ezVirtualThumbStick* m_pThumbstick;
  ezVirtualThumbStick* m_pThumbstick2;
};