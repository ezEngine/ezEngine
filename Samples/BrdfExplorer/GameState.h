#pragma once

#include <GameEngine/GameState/FallbackGameState.h>

class BrdfExplorerGameState : public ezFallbackGameState
{
  EZ_ADD_DYNAMIC_REFLECTION(BrdfExplorerGameState, ezFallbackGameState);

public:
  BrdfExplorerGameState();
  ~BrdfExplorerGameState();

private:
  virtual void OnActivation(ezWorld* pWorld) override;
  virtual void OnDeactivation() override;

  virtual float CanHandleThis(ezGameApplicationType AppType, ezWorld* pWorld) const override;

  void CreateGameLevel();
  void DestroyGameLevel();
};
