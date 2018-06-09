#pragma once

#include <GameEngine/GameState/FallbackGameState.h>

class SimpleMeshRendererGameState : public ezFallbackGameState
{
  EZ_ADD_DYNAMIC_REFLECTION(SimpleMeshRendererGameState, ezFallbackGameState);

public:
  SimpleMeshRendererGameState();
  virtual ~SimpleMeshRendererGameState();

  virtual void ConfigureInputActions() override;
  virtual void ProcessInput() override;

private:
  virtual void OnActivation(ezWorld* pWorld) override;
  virtual void OnDeactivation() override;

  virtual ezGameState::Priority DeterminePriority(ezGameApplicationType AppType, ezWorld* pWorld) const override;
  
  void CreateGameLevel();
  void DestroyGameLevel();
  void MoveObjectToPosition(const ezVec3& pos);

  ezGameObjectHandle m_hSponza;
  ezGameObjectHandle m_hTree;
};
