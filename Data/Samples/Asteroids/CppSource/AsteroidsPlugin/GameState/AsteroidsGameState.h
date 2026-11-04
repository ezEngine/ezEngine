#pragma once

#include <AsteroidsPlugin/AsteroidsPluginDLL.h>
#include <GameEngine/GameState/GameState.h>

/// Selects the scene to load when the game is started. The rules of the game are in AsteroidsGameComponent.
class AsteroidsGameState : public ezGameState
{
  EZ_ADD_DYNAMIC_REFLECTION(AsteroidsGameState, ezGameState);

public:
  AsteroidsGameState();
  ~AsteroidsGameState();

protected:
  virtual void GetStartupOptions(ezString& out_sScene, ezString& out_sPreloadCollection) override;
};
