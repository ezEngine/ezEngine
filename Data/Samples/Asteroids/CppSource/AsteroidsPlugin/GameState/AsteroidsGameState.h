#pragma once

#include <AsteroidsPlugin/AsteroidsPluginDLL.h>
#include <AsteroidsPlugin/GameState/Level.h>
#include <Core/Input/Declarations.h>
#include <Core/World/Declarations.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameState/FallbackGameState.h>
#include <GameEngine/GameState/GameState.h>

// the ezFallbackGameState adds a free flying camera and a scene switching menu, so can be useful in the very beginning
// but generally it's better to use ezGameState instead
// using AsteroidsGameStateBase = ezFallbackGameState;
using AsteroidsGameStateBase = ezGameState;

class AsteroidsGameState : public AsteroidsGameStateBase
{
  EZ_ADD_DYNAMIC_REFLECTION(AsteroidsGameState, AsteroidsGameStateBase);

public:
  AsteroidsGameState();
  ~AsteroidsGameState();

  virtual void ProcessInput() override;

protected:
  virtual void ConfigureInputActions() override;
  virtual void OnChangedMainWorld(ezWorld* pPrevWorld, ezWorld* pNewWorld, ezStringView sStartPosition, const ezTransform& startPositionOffset) override;
  virtual ezString GetStartupSceneFile() override;

private:
  virtual void OnActivation(ezWorld* pWorld, ezStringView sStartPosition, const ezTransform& startPositionOffset) override;
  virtual void OnDeactivation() override;
  virtual void BeforeWorldUpdate() override;

  void CreateGameLevel();
  void DestroyLevel();

  ezUniquePtr<Level> m_pLevel;
};
