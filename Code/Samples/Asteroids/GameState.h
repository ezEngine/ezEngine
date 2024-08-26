#pragma once

#include "Level.h"
#include <Core/Input/VirtualThumbStick.h>
#include <GameEngine/GameState/GameState.h>

class AsteroidGameState : public ezGameState
{
  EZ_ADD_DYNAMIC_REFLECTION(AsteroidGameState, ezGameState);

public:
  AsteroidGameState();

  virtual void ProcessInput() override;

protected:
  virtual void ConfigureInputActions() override;

  virtual void OnActivation(ezWorld* pWorld, ezStringView sStartPosition, const ezTransform& startPositionOffset) override;
  virtual void OnDeactivation() override;
  virtual void BeforeWorldUpdate() override;

  void CreateGameLevel();
  void DestroyLevel();

  ezUniquePtr<Level> m_pLevel;
};
