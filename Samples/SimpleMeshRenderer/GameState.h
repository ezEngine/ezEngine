#pragma once

#include <GameFoundation/GameState/FallbackGameState.h>

class SimpleMeshRendererGameState : public ezFallbackGameState
{
  EZ_ADD_DYNAMIC_REFLECTION(SimpleMeshRendererGameState, ezFallbackGameState);

public:
  SimpleMeshRendererGameState();
  ~SimpleMeshRendererGameState();

private:
  virtual void OnActivation(ezWorld* pWorld) override;
  virtual void OnDeactivation() override;

  virtual float CanHandleThis(ezGameApplicationType AppType, ezWorld* pWorld) const override;
  
  void CreateGameLevel();
  void DestroyGameLevel();
};